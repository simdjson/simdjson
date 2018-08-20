#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>

#include "jsonparser/common_defs.h"
#include "jsonparser/simdjson_internal.h"

// the ape machine consists of two parts:
//
// 1) The "state machine", which is a multiple channel per-level state machine
//    It is a conventional DFA except in that it 'changes track' on {}[]
//    characters
//
// 2) The "tape machine": this records offsets of various structures as they go
// by
//    These structures are either u32 offsets of other tapes or u32 offsets into
//    our input or structures.
//
// The state machine doesn't record ouput.
// The tape machine doesn't validate.
//
// The output of the tape machine is meaningful only if the state machine is in
// non-error states.

// depth adjustment is strictly based on whether we are {[ or }]

// depth adjustment is a pre-increment which, in effect, means that a {[
// contained in an object is in the level one deeper, while the corresponding }]
// is at the level

// TAPE MACHINE DEFINITIONS

const u32 DEPTH_PLUS_ONE = 0x01000000;
const u32 DEPTH_ZERO = 0x00000000;
const u32 DEPTH_MINUS_ONE = 0xff000000;
const u32 WRITE_ZERO = 0x0;
const u32 WRITE_FOUR = 0x1;

const u32 CDF = DEPTH_ZERO | WRITE_ZERO; // default 'control'
const u32 C04 = DEPTH_ZERO | WRITE_FOUR;
const u32 CP4 = DEPTH_PLUS_ONE | WRITE_FOUR;
const u32 CM4 = DEPTH_MINUS_ONE | WRITE_FOUR;

inline s8 get_depth_adjust(u32 control) { return (s8)(((s32)control) >> 24); }
inline size_t get_write_size(u32 control) { return control & 0xff; }

const u32 char_control[256] = {
    // nothing interesting from 0x00-0x20
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF,

    // " is 0x22, - is 0x2d
    CDF, CDF, C04, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, C04, CDF,
    CDF,

    // numbers are 0x30-0x39
    C04, C04, C04, C04, C04, C04, C04, C04, C04, C04, CDF, CDF, CDF, CDF, CDF,
    CDF,

    // nothing interesting from 0x40-0x49
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF,

    // 0x5b/5d are []
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CP4, CDF, CM4, CDF,
    CDF,

    // f is 0x66 n is 0x6e
    CDF, CDF, CDF, CDF, CDF, CDF, C04, CDF, CDF, CDF, CDF, CDF, CDF, CDF, C04,
    CDF,

    // 0x7b/7d are {}, 74 is t
    CDF, CDF, CDF, CDF, C04, CDF, CDF, CDF, CDF, CDF, CDF, CP4, CDF, CM4, CDF,
    CDF,

    // nothing interesting from 0x80-0xff
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF,
    CDF, CDF, CDF, CDF, CDF, CDF, CDF, CDF};

// all of this stuff needs to get moved somewhere reasonable
// like our ParsedJson structure
/*
u64 tape[MAX_TAPE];
u32 tape_locs[MAX_DEPTH];
u8 string_buf[512*1024];
u8 * current_string_buf_loc;
u8 number_buf[512*1024]; // holds either doubles or longs, really
u8 * current_number_buf_loc;
*/

// STATE MACHINE DECLARATIONS
const u32 MAX_STATES = 16;

/**
 * It is annoying to have to call init_state_machine each time.
 * Better to precompute the (small) result into a header file.
 */
// u32 trans[MAX_STATES][256];
#include "jsonparser/transitions.h"

u32 states[MAX_DEPTH];
const int START_STATE = 1;

u32 valid_end_states[MAX_STATES] = {
    0, // 0 state is by definition an error
    1, // ok to still be in start state
    1, // state 2: we've seen an { - if we left this level it's ok
    0, // state 3 is abolished, we shouldn't be in it

    0, // state 4 means we saw a string inside an object. We can't end like
       // this!
    0, // similarly state 5 means we saw a string followed by a colon.
    0, // state 6 is abolished
    1, // it's ok to finish on 7

    0, // state 8 we've seen a comma inside an object - can't finish here
    1, // state 9 is like state 2 only for arrays, so ok
    0, // state 10 abolished
    1, // state 11 is ok to finish on, we just saw a unary inside a array

    0, // state 12 we've just seen a comma inside an array - can't finish
    0, // state 13 is our weird start state. I think we shouldn't end on it as
       // we need to see something
    1, // state 14 is ok. Its an error to see something *more* here but not to
       // be in this state
    0, // we don't use state 15
};

// weird sub-machine for starting depth only
// we start at 13 and go to 14 on a single UNARY
// 14 doesn't have to have any transitions. Anything
// else arrives after the single thing it's an error
const int START_DEPTH_START_STATE = 13;

// ANYTHING_IS_ERROR_STATE is useful both as a target
// for a transition at the start depth and also as
// a good initial value for "red line" depths; that
// is, depths that are maintained strictly to avoid
// undefined behavior (e.g. depths below the starting
// depth).
const int ANYTHING_IS_ERROR_STATE = 14;

void init_state_machine() {
  // states 10 and 6 eliminated

  trans[1][(int)'{'] = 2;
  trans[2][(int)'"'] = 4;
  trans[4][(int)':'] = 5;
  // 5->7 on all values ftn0123456789-"
  trans[7][(int)','] = 8;
  trans[8][(int)'"'] = 4;

  trans[1][(int)'['] = 9;
  // 9->11 on all values ftn0123456789-"
  trans[11][(int)','] = 12;
  // 12->11 on all values ftn0123456789-"

  const char *UNARIES = "}]ftn0123456789-\"";
  for (u32 i = 0; i < strlen(UNARIES); i++) {
    trans[5][(u32)UNARIES[i]] = 7;
    trans[9][(u32)UNARIES[i]] = 11;
    trans[12][(u32)UNARIES[i]] = 11;
#ifdef PERMIT_RANDOM_UNARIES_AT_TOP_LEVEL
    // NOTE: if we permit JSON documents that
    // contain a single number or string, then we
    // allow all the unaries at the top level
    trans[13][(u32)UNARIES[i]] = 14;
#endif
  }

#ifndef PERMIT_RANDOM_UNARIES_AT_TOP_LEVEL
  // NOTE: if we don't permit JSON documents that
  // that contain a single number or string, we must
  // make sure we accept the top-level closing braces
  // that are delivered to the start depth only
  trans[13][(int)'}'] = 14;
  trans[13][(int)']'] = 14;
#endif

  // back transitions when new things are open
  trans[2][(int)'{'] = 2;
  trans[7][(int)'{'] = 2;
  trans[9][(int)'{'] = 2;
  trans[11][(int)'{'] = 2;
  trans[2][(int)'['] = 9;
  trans[7][(int)'['] = 9;
  trans[9][(int)'['] = 9;
  trans[11][(int)'['] = 9;
}

bool ape_machine(const u8 *buf, UNUSED size_t len, ParsedJson &pj) {

  // NOTE - our depth is used by both the tape machine and the state machine
  // Further, in production we will set it to a largish value in a generous
  // buffer as a rogue input could consist of many {[ characters or many }]
  // characters. We aren't busily checking errors (and in fact, a aggressive
  // sequence of [ characters is actually valid input!) so something that blows
  // out maximum depth will need to be periodically checked for, as will
  // something that tries to set depth very low. If we set our starting depth,
  // say, to 256, we can tolerate 256 bogus close brace characters without
  // aggressively going wrong and writing to bad memory Note that any specious
  // depth can have a specious tape associated with and all these specious
  // depths can share a region of the tape - it's harmless. Since tape is
  // one-way, any movement in a specious tape is an error (so we can detect
  // max_depth violations by making sure that specious tape locations haven't
  // moved from their starting values)

  u32 depth = START_DEPTH;

  for (u32 i = 0; i < MAX_DEPTH; i++) {
    pj.tape_locs[i] = i * MAX_TAPE_ENTRIES;
    if (i == START_DEPTH) {
      states[i] = START_DEPTH_START_STATE;
    } else if ((i < START_DEPTH) || (i >= REDLINE_DEPTH)) {
      states[i] = ANYTHING_IS_ERROR_STATE;
    } else {
      states[i] = START_STATE;
    }
  }

  pj.current_string_buf_loc = pj.string_buf;
  pj.current_number_buf_loc = pj.number_buf;

  u32 error_sump = 0;
  u32 old_tape_loc = pj.tape_locs[depth]; // need to initialize for first write

  u32 next_idx = pj.structural_indexes[0];
  u8 next_c = buf[next_idx];
  u32 next_control = char_control[next_c];

  for (u32 i = 0; i < pj.n_structural_indexes; i++) {

    // very periodic safety checking. This does NOT guarantee that we
    // haven't been in our dangerous zones above or below our normal
    // depths. It ONLY checks to be sure that we don't manage to leave
    // these zones and write completely off our tape.
    if (!(i % DEPTH_SAFETY_MARGIN)) {
      if (depth < START_DEPTH || depth >= REDLINE_DEPTH) {
        error_sump |= 1;
        break;
      }
    }

    u32 idx = next_idx;
    u8 c = next_c;
    u32 control = next_control;

    next_idx = pj.structural_indexes[i + 1];
    next_c = buf[next_idx];
    next_control = char_control[next_c];

    // TAPE MACHINE
    s8 depth_adjust = get_depth_adjust(control);
    u8 write_size = get_write_size(control);
    u32 write_val = (depth_adjust != 0) ? old_tape_loc : idx;
    depth += depth_adjust;
#ifdef DEBUG
    cout << "i: " << i << " idx: " << idx << " c " << c << "\n";
    cout << "TAPE MACHINE: depth change " << (s32)depth_adjust << " write_size "
         << (u32)write_size << " current_depth: " << depth << "\n";
#endif

    // STATE MACHINE - hoisted here to fill in during the tape machine's
    // latencies
#ifdef DEBUG
    cout << "STATE MACHINE: state[depth] pre " << states[depth] << " ";
#endif
    states[depth] = trans[states[depth]][c];
#ifdef DEBUG
    cout << "post " << states[depth] << "\n";
#endif
    // TAPE MACHINE, again
    pj.tape[pj.tape_locs[depth]] = write_val | (((u64)c) << 56);
    old_tape_loc = pj.tape_locs[depth] += write_size;
  }

  if (depth != START_DEPTH) {
    // We haven't returned to our start depth, so our braces can't possibly
    // match Note this doesn't exclude the possibility that we have improperly
    // matched { } or [] pairs
    return false;
  }

  for (u32 i = 0; i < MAX_DEPTH; i++) {
    if (!valid_end_states[states[i]]) {
#ifdef DEBUG
      printf("Invalid ending state: states[%d] == %d\n", states[i]);
#endif
      return false;
    }
  }

#define DUMP_TAPES
#ifdef DEBUG
  for (u32 i = 0; i < MAX_DEPTH; i++) {
    u32 start_loc = i * MAX_TAPE_ENTRIES;
    cout << " tape section i " << i;
    if (i == START_DEPTH) {
      cout << "   (START) ";
    } else if ((i < START_DEPTH) || (i >= REDLINE_DEPTH)) {
      cout << " (REDLINE) ";
    } else {
      cout << "  (NORMAL) ";
    }

    cout << " from: " << start_loc << " to: " << tape_locs[i] << " "
         << " size: " << (tape_locs[i] - start_loc) << "\n";
    cout << " state: " << states[i] << "\n";
#ifdef DUMP_TAPES
    for (u32 j = start_loc; j < tape_locs[i]; j++) {
      if (tape[j]) {
        cout << "j: " << j << " tape[j] char " << (char)(tape[j] >> 56)
             << " tape[j][0..55]: " << (tape[j] & 0xffffffffffffffULL) << "\n";
      }
    }
#endif
  }
#endif
  if (error_sump) {
    return false;
  }
  return true;
}
