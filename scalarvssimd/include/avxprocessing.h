#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <x86intrin.h>
#include <assert.h>
#include "common_defs.h"
#include "jsonstruct.h"
using namespace std;

#ifdef DEBUG
inline void dump256(m256 d, string msg) {
	for (u32 i = 0; i < 32; i++) {
		cout << setw(3) << (int)*(((u8 *)(&d)) + i);
        if (!((i+1)%8))
            cout << "|";
        else if (!((i+1)%4))
            cout << ":";
        else
            cout << " ";
	}
    cout << " " << msg << "\n";
}

// dump bits low to high
void dumpbits(u64 v, string msg) {
	for (u32 i = 0; i < 64; i++) {
        std::cout << (((v>>(u64)i) & 0x1ULL) ? "1" : "_");
    }
    cout << " " << msg << "\n";
}

void dumpbits32(u32 v, string msg) {
	for (u32 i = 0; i < 32; i++) {
        std::cout << (((v>>(u32)i) & 0x1ULL) ? "1" : "_");
    }
    cout << " " << msg << "\n";
}
#else
#define dump256(a,b) ;
#define dumpbits(a,b) ;
#define dumpbits32(a,b) ;
#endif
// a straightforward comparison of a mask against input. 5 uops; would be cheaper in AVX512.
really_inline u64 cmp_mask_against_input(m256 input_lo, m256 input_hi, m256 mask) {
    m256 cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
    u64 res_0 = (u32)_mm256_movemask_epi8(cmp_res_0);
    m256 cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
    u64 res_1 = _mm256_movemask_epi8(cmp_res_1);
    return res_0 | (res_1 << 32);
}

never_inline bool find_structural_bits(const u8 * buf, size_t len, ParsedJson & pj) {
    // Useful constant masks
    const u64 even_bits = 0x5555555555555555ULL;
    const u64 odd_bits = ~even_bits;

    // for now, just work in 64-byte chunks
    // we have padded the input out to 64 byte multiple with the remainder being zeros

    // persistent state across loop
    u64 prev_iter_ends_odd_backslash = 0ULL; // either 0 or 1, but a 64-bit value
    u64 prev_iter_inside_quote = 0ULL; // either all zeros or all ones
    u64 prev_iter_ends_pseudo_pred = 0ULL;

    for (size_t idx = 0; idx < len; idx+=64) {
        __builtin_prefetch(buf + idx + 128);
#ifdef DEBUG
        cout << "Idx is " << idx << "\n";
        for (u32 j = 0; j < 64; j++) {
            char c = *(buf+idx+j);
            if (isprint(c)) {
                cout << c;
            } else {
                cout << '_';
            }
        }
        cout << "|  ... input\n";
#endif
        m256 input_lo = _mm256_load_si256((const m256 *)(buf + idx + 0));
        m256 input_hi = _mm256_load_si256((const m256 *)(buf + idx + 32));

        ////////////////////////////////////////////////////////////////////////////////////////////
        //     Step 1: detect odd sequences of backslashes
        ////////////////////////////////////////////////////////////////////////////////////////////

        u64 bs_bits = cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
        dumpbits(bs_bits, "backslash bits");
        u64 start_edges = bs_bits & ~(bs_bits << 1);
        dumpbits(start_edges, "start_edges");

        // flip lowest if we have an odd-length run at the end of the prior iteration
        u64 even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
        u64 even_starts = start_edges & even_start_mask;
        u64 odd_starts = start_edges & ~even_start_mask;

        dumpbits(even_starts, "even_starts");
        dumpbits(odd_starts, "odd_starts");

        u64 even_carries = bs_bits + even_starts;

        u64 odd_carries;
        // must record the carry-out of our odd-carries out of bit 63; this indicates whether the
        // sense of any edge going to the next iteration should be flipped
        bool iter_ends_odd_backslash = __builtin_uaddll_overflow(bs_bits, odd_starts, &odd_carries);

        odd_carries |= prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                                     // if we had an odd-numbered run at the end of
                                                     // the previous iteration
        prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;

        dumpbits(even_carries, "even_carries");
        dumpbits(odd_carries, "odd_carries");

        u64 even_carry_ends = even_carries & ~bs_bits;
        u64 odd_carry_ends = odd_carries & ~bs_bits;
        dumpbits(even_carry_ends, "even_carry_ends");
        dumpbits(odd_carry_ends, "odd_carry_ends");

        u64 even_start_odd_end = even_carry_ends & odd_bits;
        u64 odd_start_even_end = odd_carry_ends & even_bits;
        dumpbits(even_start_odd_end, "esoe");
        dumpbits(odd_start_even_end, "osee");

        u64 odd_ends = even_start_odd_end | odd_start_even_end;
        dumpbits(odd_ends, "odd_ends");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //     Step 2: detect insides of quote pairs
        ////////////////////////////////////////////////////////////////////////////////////////////

        u64 quote_bits = cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('"'));
        quote_bits = quote_bits & ~odd_ends;
        dumpbits(quote_bits, "quote_bits");
        u64 quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(_mm_set_epi64x(0ULL, quote_bits),
                                                                _mm_set1_epi8(0xFF), 0));
        quote_mask ^= prev_iter_inside_quote;
        prev_iter_inside_quote = (u64)((s64)quote_mask>>63);
        dumpbits(quote_mask, "quote_mask");

        // How do we build up a user traversable data structure
        // first, do a 'shufti' to detect structural JSON characters
        // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
        // these go into the first 3 buckets of the comparison (1/2/4)

        // we are also interested in the four whitespace characters
        // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
        // these go into the next 2 buckets of the comparison (8/16)
        const m256 low_nibble_mask = _mm256_setr_epi8(
        //  0                           9  a   b  c  d
            16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0,
            16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0
        );
        const m256 high_nibble_mask = _mm256_setr_epi8(
        //  0     2   3     5     7
            8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0,
            8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0
        );

        m256 structural_shufti_mask = _mm256_set1_epi8(0x7);
        m256 whitespace_shufti_mask = _mm256_set1_epi8(0x18);

        m256 v_lo = _mm256_and_si256(
                        _mm256_shuffle_epi8(low_nibble_mask, input_lo),
                        _mm256_shuffle_epi8(high_nibble_mask,
                           _mm256_and_si256(_mm256_srli_epi32(input_lo, 4), _mm256_set1_epi8(0x7f))));

        m256 v_hi = _mm256_and_si256(
                        _mm256_shuffle_epi8(low_nibble_mask, input_hi),
                        _mm256_shuffle_epi8(high_nibble_mask,
                           _mm256_and_si256(_mm256_srli_epi32(input_hi, 4), _mm256_set1_epi8(0x7f))));
        m256 tmp_lo = _mm256_cmpeq_epi8(_mm256_and_si256(v_lo, structural_shufti_mask),
                                        _mm256_set1_epi8(0));
        m256 tmp_hi = _mm256_cmpeq_epi8(_mm256_and_si256(v_hi, structural_shufti_mask),
                                        _mm256_set1_epi8(0));

        u64 structural_res_0 = (u32)_mm256_movemask_epi8(tmp_lo);
        u64 structural_res_1 = _mm256_movemask_epi8(tmp_hi);
        u64 structurals =  ~(structural_res_0 | (structural_res_1 << 32));

        // this additional mask and transfer is non-trivially expensive, unfortunately
        m256 tmp_ws_lo = _mm256_cmpeq_epi8(_mm256_and_si256(v_lo, whitespace_shufti_mask),
                                        _mm256_set1_epi8(0));
        m256 tmp_ws_hi = _mm256_cmpeq_epi8(_mm256_and_si256(v_hi, whitespace_shufti_mask),
                                        _mm256_set1_epi8(0));

        u64 ws_res_0 = (u32)_mm256_movemask_epi8(tmp_ws_lo);
        u64 ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
        u64 whitespace =  ~(ws_res_0 | (ws_res_1 << 32));

        dumpbits(structurals, "structurals");
        dumpbits(whitespace, "whitespace");

        // mask off anything inside quotes
        structurals &= ~quote_mask;

        // add the real quote bits back into our bitmask as well, so we can
        // quickly traverse the strings we've spent all this trouble gathering
        structurals |= quote_bits;

        // Now, establish "pseudo-structural characters". These are non-whitespace characters
        // that are (a) outside quotes and (b) have a predecessor that's either whitespace or a structural
        // character. This means that subsequent passes will get a chance to encounter the first character
        // of every string of non-whitespace and, if we're parsing an atom like true/false/null or a number
        // we can stop at the first whitespace or structural character following it.

        // a qualified predecessor is something that can happen 1 position before an
        // psuedo-structural character
        u64 pseudo_pred = structurals | whitespace;
        dumpbits(pseudo_pred, "pseudo_pred");
        u64 shifted_pseudo_pred = (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
        dumpbits(shifted_pseudo_pred, "shifted_pseudo_pred");
        prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
        u64 pseudo_structurals = shifted_pseudo_pred & (~whitespace) & (~quote_mask);
        dumpbits(pseudo_structurals, "pseudo_structurals");
        dumpbits(structurals, "final structurals without pseudos");
        structurals |= pseudo_structurals;
        dumpbits(structurals, "final structurals and pseudo structurals");

        // now, we've used our close quotes all we need to. So let's switch them off
        // they will be off in the quote mask and on in quote bits.
        structurals &= ~(quote_bits & ~quote_mask);
        dumpbits(structurals, "final structurals and pseudo structurals after close quote removal");
        *(u64 *)(pj.structurals + idx/8) = structurals;
    }
    return true;
}

const u32 NUM_RESERVED_NODES = 2;
const u32 DUMMY_NODE = 0;
const u32 ROOT_NODE = 1;

// just transform the bitmask to a big list of 32-bit integers for now
// that's all; the type of character the offset points to will
// tell us exactly what we need to know. Naive but straightforward implementation
never_inline bool flatten_indexes(size_t len, ParsedJson & pj) {
    u32 * base_ptr = pj.structural_indexes;
    base_ptr[DUMMY_NODE] = base_ptr[ROOT_NODE] = 0; // really shouldn't matter
    u32 base = NUM_RESERVED_NODES;
    for (size_t idx = 0; idx < len; idx+=64) {
        u64 s = *(u64 *)(pj.structurals + idx/8);
#ifdef SUPPRESS_CHEESY_FLATTEN
        while (s) {
            base_ptr[base++] = (u32)idx + __builtin_ctzll(s); s &= s - 1ULL;
        }
#else
        u32 cnt = __builtin_popcountll(s);
        u32 next_base = base + cnt;
        while (s) {
            // spoil the suspense by reducing dependency chains; actually a win even with cost of pdep
            u64 s3 = _pdep_u64(~0x7ULL, s); // s3 will have bottom 3 1-bits unset
            u64 s5 = _pdep_u64(~0x1fULL, s); // s5 will have bottom 5 1-bits unset

            base_ptr[base+0] = (u32)idx + __builtin_ctzll(s);  u64 s1 = s  & (s  - 1ULL);
            base_ptr[base+1] = (u32)idx + __builtin_ctzll(s1); u64 s2 = s1 & (s1 - 1ULL);
            base_ptr[base+2] = (u32)idx + __builtin_ctzll(s2); //u64 s3 = s2 & (s2 - 1ULL);
            base_ptr[base+3] = (u32)idx + __builtin_ctzll(s3); u64 s4 = s3 & (s3 - 1ULL);

            base_ptr[base+4] = (u32)idx + __builtin_ctzll(s4); //u64 s5 = s4 & (s4 - 1ULL);
            base_ptr[base+5] = (u32)idx + __builtin_ctzll(s5); u64 s6 = s5 & (s5 - 1ULL);
            s = s6;
            base += 6;
        }
        base = next_base;
#endif
    }
    pj.n_structural_indexes = base;
    base_ptr[pj.n_structural_indexes] = 0; // make it safe to dereference one beyond this array
    return true;
}


const u32 MAX_DEPTH = 256;
const u32 DEPTH_SAFETY_MARGIN = 32; // should be power-of-2 as we check this with a modulo in our
                                    // hot stage 3 loop
const u32 START_DEPTH = DEPTH_SAFETY_MARGIN;
const u32 REDLINE_DEPTH = MAX_DEPTH - DEPTH_SAFETY_MARGIN;

// the ape machine consists of two parts:
//
// 1) The "state machine", which is a multiple channel per-level state machine
//    It is a conventional DFA except in that it 'changes track' on {}[] characters
//
// 2) The "tape machine": this records offsets of various structures as they go by
//    These structures are either u32 offsets of other tapes or u32 offsets into our input
//    or structures.
//
// The state machine doesn't record ouput.
// The tape machine doesn't validate.
//
// The output of the tape machine is meaningful only if the state machine is in non-error states.

// depth adjustment is strictly based on whether we are {[ or }]

// depth adjustment is a pre-increment which, in effect, means that a {[ contained in an object
// is in the level one deeper, while the corresponding }] is at the level


// TAPE MACHINE DEFINITIONS

const u32 DEPTH_PLUS_ONE =  0x01000000;
const u32 DEPTH_ZERO =      0x00000000;
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
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,

    // " is 0x22, - is 0x2d
    CDF,CDF,C04,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,C04,CDF,CDF,

    // numbers are 0x30-0x39
    C04,C04,C04,C04, C04,C04,C04,C04, C04,C04,CDF,CDF, CDF,CDF,CDF,CDF,

    // nothing interesting from 0x40-0x49
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,

    // 0x5b/5d are []
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CP4, CDF,CM4,CDF,CDF,

    // f is 0x66 n is 0x6e
    CDF,CDF,CDF,CDF, CDF,CDF,C04,CDF, CDF,CDF,CDF,CDF, CDF,CDF,C04,CDF,

    // 0x7b/7d are {}, 74 is t
    CDF,CDF,CDF,CDF, C04,CDF,CDF,CDF, CDF,CDF,CDF,CP4, CDF,CM4,CDF,CDF,

    // nothing interesting from 0x80-0xff
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF,
    CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF, CDF,CDF,CDF,CDF
};

const size_t MAX_TAPE_ENTRIES = 127*1024;
const size_t MAX_TAPE = MAX_DEPTH * MAX_TAPE_ENTRIES;

// all of this stuff needs to get moved somewhere reasonable
// like our ParsedJson structure
u64 tape[MAX_TAPE];
u32 tape_locs[MAX_DEPTH];
u8 string_buf[512*1024];
u8 * current_string_buf_loc;
u8 number_buf[512*1024]; // holds either doubles or longs, really
u8 * current_number_buf_loc;

// STATE MACHINE DECLARATIONS
const u32 MAX_STATES = 16;
u32 trans[MAX_STATES][256];
u32 states[MAX_DEPTH];
const int START_STATE = 1;

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

never_inline void init_state_machine() {
    // states 10 and 6 eliminated

    trans[ 1]['{'] = 2;
    trans[ 2]['"'] = 4;
    trans[ 4][':'] = 5;
    // 5->7 on all values ftn0123456789-"
    trans[ 7][','] = 8;
    trans[ 8]['"'] = 4;

    trans[ 1]['['] = 9;
    // 9->11 on all values ftn0123456789-"
    trans[11][','] = 12;
    // 12->11 on all values ftn0123456789-"

    const char * UNARIES = "}]ftn0123456789-\"";
    for (u32 i = 0; i < strlen(UNARIES); i++) {
        trans[ 5][(u32)UNARIES[i]] = 7;
        trans[ 9][(u32)UNARIES[i]] = 11;
        trans[12][(u32)UNARIES[i]] = 11;
        trans[13][(u32)UNARIES[i]] = 14;
    }

    // back transitions when new things are open
    trans[2]['{'] = 2;
    trans[7]['{'] = 2;
    trans[9]['{'] = 2;
    trans[11]['{'] = 2;
    trans[2]['['] = 9;
    trans[7]['['] = 9;
    trans[9]['['] = 9;
    trans[11]['['] = 9;

}

never_inline bool ape_machine(const u8 * buf, UNUSED size_t len, ParsedJson & pj) {
    // NOTE - our depth is used by both the tape machine and the state machine
    // Further, in production we will set it to a largish value in a generous buffer as a rogue input
    // could consist of many {[ characters or many }] characters. We aren't busily checking errors
    // (and in fact, a aggressive sequence of [ characters is actually valid input!) so something that
    // blows out maximum depth will need to be periodically checked for, as will something that tries
    // to set depth very low. If we set our starting depth, say, to 256, we can tolerate 256 bogus close brace
    // characters without aggressively going wrong and writing to bad memory
    // Note that any specious depth can have a specious tape associated with and all these specious depths
    // can share a region of the tape - it's harmless. Since tape is one-way, any movement in a specious tape
    // is an error (so we can detect max_depth violations by making sure that specious tape locations haven't
    // moved from their starting values)

    u32 depth = START_DEPTH;

    for (u32 i = 0; i < MAX_DEPTH; i++) {
        tape_locs[i] = i*MAX_TAPE_ENTRIES;
        if (i == START_DEPTH) {
            states[i] = START_DEPTH_START_STATE;
        } else if ((i < START_DEPTH) || (i >= REDLINE_DEPTH)) {
            states[i] = ANYTHING_IS_ERROR_STATE;
        } else {
            states[i] = START_STATE;
        }
    }

    current_string_buf_loc = string_buf;
    current_number_buf_loc = number_buf;

    u32 error_sump = 0;
    u32 old_tape_loc = tape_locs[depth]; // need to initialize for first write

    u32 next_idx = pj.structural_indexes[0];
    u8 next_c = buf[next_idx];
    u32 next_control = char_control[next_c];

    for (u32 i = NUM_RESERVED_NODES; i < pj.n_structural_indexes; i++) {

        // very periodic safety checking. This does NOT guarantee that we
        // haven't been in our dangerous zones above or below our normal
        // depths. It ONLY checks to be sure that we don't manage to leave
        // these zones and write completely off our tape.
        if (!(i%DEPTH_SAFETY_MARGIN)) {
            if (depth < START_DEPTH || depth >= REDLINE_DEPTH) {
                error_sump |= 1;
                break;
            }
        }

        u32 idx = next_idx;
        u8 c = next_c;
        u32 control = next_control;

        next_idx = pj.structural_indexes[i+1];
        next_c = buf[next_idx];
        next_control = char_control[next_c];

        // TAPE MACHINE
        s8 depth_adjust = get_depth_adjust(control);
        u8 write_size = get_write_size(control);
        u32 write_val = (depth_adjust != 0) ? old_tape_loc : idx;
        depth += depth_adjust;
#ifdef DEBUG
        cout << "i: " << i << " idx: " << idx << " c " << c << "\n";
        cout << "TAPE MACHINE: depth change " << (s32)depth_adjust
             << " write_size " << (u32)write_size << " current_depth: " << depth << "\n";
#endif

        // STATE MACHINE - hoisted here to fill in during the tape machine's latencies
#ifdef DEBUG
        cout << "STATE MACHINE: state[depth] pre " << states[depth] << " ";
#endif
        states[depth] = trans[states[depth]][c];
#ifdef DEBUG
        cout << "post " << states[depth] << "\n";
#endif
        // TAPE MACHINE, again
        tape[tape_locs[depth]] = write_val | (((u64)c) << 56);
        old_tape_loc = tape_locs[depth] += write_size;
    }
/*
    for (u32 i = 0; i < MAX_DEPTH; i++) {
        if (states[i] == 0) {
          printf("duuh\n");
            return false;
        }
    }*/

#define DUMP_TAPES
#ifdef DEBUG
    for (u32 i = 0; i < MAX_DEPTH; i++) {
        u32 start_loc = i*MAX_TAPE_ENTRIES;
        cout << " tape section i " << i;
        if (i == START_DEPTH) {
            cout << "   (START) ";
        } else if ((i < START_DEPTH) || (i >= REDLINE_DEPTH)) {
            cout << " (REDLINE) ";
        } else {
            cout << "  (NORMAL) ";
        }

        cout << " from: " << start_loc
             << " to: " << tape_locs[i] << " "
             << " size: " << (tape_locs[i]-start_loc) << "\n";
        cout << " state: " << states[i] << "\n";
#ifdef DUMP_TAPES
        for (u32 j = start_loc; j < tape_locs[i]; j++) {
            if (tape[j]) {
                cout << "j: " << j << " tape[j] char " << (char)(tape[j]>>56)
                     << " tape[j][0..55]: " << (tape[j]&0xffffffffffffffULL ) << "\n";
            }
        }
#endif
    }
#endif
    if (error_sump) {
      printf("error_sump\n");
        return false;
    }
    return true;
}


// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
// these go into the first 3 buckets of the comparison (1/2/4)

// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d

const u32 structural_or_whitespace_negated[256] = {
    1,1,1,1, 1,1,1,1, 1,0,0,1, 1,0,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    0,1,1,1, 1,1,1,1, 1,1,1,1, 0,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,0,1, 1,1,1,1,

    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,0, 1,0,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,0, 1,0,1,1,

    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,

    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1
};

// return non-zero if not a structural or whitespace char
// zero otherwise
really_inline u32 is_not_structural_or_whitespace(u8 c) {
    return structural_or_whitespace_negated[c];
}

// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
const u8 escape_map[256] = {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //0x0.
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0x22,0, 0,0,0,0, 0,0,0,0, 0,0,0,0x2f,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //0x4.
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0x5c,0,0,0, //0x5.
    0,0,0x08,0, 0,0,0x12,0, 0,0,0,0, 0,0,0x0a,0, //0x6.
    0,0,0x0d,0, 0x09,0,0,0, 0,0,0,0, 0,0,0,0, //0x7.

    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
};


const u32 leading_zeros_to_utf_bytes[33] = {
    1,
    1, 1, 1, 1, 1, 1, 1, // 7 bits for first one
    2, 2, 2, 2, // 11 bits for next
    3, 3, 3, 3, 3, // 16 bits for next
    4, 4, 4, 4, 4, // 21 bits for next
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // error


const u32 UTF_PDEP_MASK[5] = {
    0x00, // error
    0x7f,
    0x1f3f,
    0x0f3f3f,
    0x073f3f3f
};

const u32 UTF_OR_MASK[5] = {
    0x00, // error
    0x00,
    0xc080,
    0xe08080,
    0xf0808080
};

bool is_hex_digit(u8 v) {
    if (v >= '0' && v <= '9')
        return true;
    v &= 0xdf;
    if (v >= 'A' && v <= 'F')
        return true;
    return false;
}

u8 digit_to_val(u8 v) {
    if (v >= '0' && v <= '9')
        return v - '0';
    v &= 0xdf;
    return v - 'A' + 10;
}

bool hex_to_u32(const u8 * src, u32 * res) {
    u8 v1 = src[0];
    u8 v2 = src[1];
    u8 v3 = src[2];
    u8 v4 = src[3];
    if (!is_hex_digit(v1) || !is_hex_digit(v2) || !is_hex_digit(v3) || !is_hex_digit(v4)) {
        return false;
    }
    *res = digit_to_val(v1) << 24 | digit_to_val(v2) << 16 | digit_to_val(v3) << 8 | digit_to_val(v4);
    return true;
}

// handle a unicode codepoint
// write appropriate values into dest
// src will always advance 6 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
really_inline bool handle_unicode_codepoint(const u8 ** src_ptr, u8 ** dst_ptr) {
    u32 code_point = 0; // read the hex, potentially reading another \u beyond if it's a // wacky one
    if (!hex_to_u32(*src_ptr + 2, &code_point)) {
        return false;
    }
    *src_ptr += 6;
    // check for the weirdo double-UTF-16 nonsense for things outside Basic Multilingual Plane.
    if (code_point >= 0xd800 && code_point < 0xdc00) {
        // TODO: sanity check and clean up; snippeted from RapidJSON and poorly understood at the moment
        if (( (*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
            return false;
        }
        u32 code_point_2 = 0;
        if (!hex_to_u32(*src_ptr + 2, &code_point_2)) {
            return false;
        }
        if (code_point_2 < 0xdc00 || code_point_2 > 0xdfff) {
            return false;
        }
        code_point = (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
        *src_ptr += 6;
    }
    // TODO: check to see whether the below code is nonsense (it's really only a sketch at this point)
    u32 lz = __builtin_clz(code_point);
    u32 utf_bytes = leading_zeros_to_utf_bytes[lz];
    u32 tmp = _pdep_u32(code_point, UTF_PDEP_MASK[utf_bytes]) | UTF_OR_MASK[utf_bytes];
    // swap and move to the other side of the register
    tmp = __builtin_bswap32(tmp);
    tmp >>= (4 - utf_bytes) * 8;
    **(u32 **)dst_ptr = tmp;
    *dst_ptr += utf_bytes;
    return true;
}

really_inline bool parse_string(const u8 * buf, UNUSED size_t len, UNUSED ParsedJson & pj, u32 tape_loc) {
    u32 offset = tape[tape_loc] & 0xffffff;
    const u8 * src = &buf[offset+1]; // we know that buf at offset is a "
    u8 * dst = current_string_buf_loc;
#ifdef DEBUG
    cout << "Entering parse string with offset " << offset << "\n";
#endif
    // basic non-sexy parsing code
    while (1) {
#ifdef DEBUG
        for (u32 j = 0; j < 32; j++) {
            char c = *(src+j);
            if (isprint(c)) {
                cout << c;
            } else {
                cout << '_';
            }
        }
        cout << "|  ... string handling input\n";
#endif
        m256 v = _mm256_loadu_si256((const m256 *)(src));
        u32 bs_bits = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')));
        dumpbits32(bs_bits, "backslash bits 2");
        u32 quote_bits = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('"')));
        dumpbits32(quote_bits, "quote_bits");
        u32 quote_dist = __builtin_ctz(quote_bits);
        u32 bs_dist = __builtin_ctz(bs_bits);
        // store to dest unconditionally - we can overwrite the bits we don't like later
        _mm256_storeu_si256((m256 *)(dst), v);
#ifdef DEBUG
            cout << "quote dist: " << quote_dist << " bs dist: " << bs_dist << "\n";
#endif

        if (quote_dist < bs_dist) {
#ifdef DEBUG
            cout << "Found end, leaving!\n";
#endif
            // we encountered quotes first. Move dst to point to quotes and exit
            dst[quote_dist] = 0; // null terminate and get out
            current_string_buf_loc = dst + quote_dist + 1;
            tape[tape_loc] = ((u32)'"') << 24 | (current_string_buf_loc - string_buf); // assume 2^24 will hold all strings for now
            return true;
        } else if (quote_dist > bs_dist) {
            u8 escape_char = src[bs_dist+1];
#ifdef DEBUG
            cout << "Found escape char: " << escape_char << "\n";
#endif
            // we encountered backslash first. Handle backslash
            if (escape_char == 'u') {
                // move src/dst up to the start; they will be further adjusted
                // within the unicode codepoint handling code.
                src += bs_dist;
                dst += bs_dist;
                if (!handle_unicode_codepoint(&src, &dst)) {
                    return false;
                }
                return true;
            } else {
                // simple 1:1 conversion. Will eat bs_dist+2 characters in input and
                // write bs_dist+1 characters to output
                // note this may reach beyond the part of the buffer we've actually seen.
                // I think this is ok
                u8 escape_result = escape_map[escape_char];
                if (!escape_result)
                    return false; // bogus escape value is an error
                dst[bs_dist] = escape_result;
                src += bs_dist+2;
                dst += bs_dist+1;
            }
        } else {
            // they are the same. Since they can't co-occur, it means we encountered neither.
            src+=32;
            dst+=32;
        }
        return true;
    }
    // later extensions -
    // if \\ we could detect whether it's a substantial run of \ or just eat 2 chars and write 1
    // handle anything short of \u or \\\ (as a prefix) with clever PSHUFB stuff and don't leave SIMD
    return true;
}

// put a parsed version of number (either as a double or a signed long) into the number buffer,
// put a 'tag' indicating which type and where it is back onto the tape at that location
// return false if we can't parse the number which means either
// (a) the number isn't valid, or (b) the number is followed by something that isn't whitespace, comma or a close }] character
// which are the only things that should follow a number at this stage
// bools to detect what we found in our initial character already here - we are already
// switching on 0 vs 1-9 vs - so we may as well keep separate paths where that's useful

// TODO: see if we really need a separate number_buf or whether we should just
//       have a generic scratch - would need to align before using for this
really_inline bool parse_number(const u8 * buf, UNUSED size_t len, UNUSED ParsedJson & pj, u32 tape_loc, UNUSED bool found_zero, bool found_minus) {
    u32 offset = tape[tape_loc] & 0xffffff;
    if (found_minus) {
        offset++;
    }
    const u8 * src = &buf[offset];
    m256 v = _mm256_loadu_si256((const m256 *)(src));
    u64 error_sump = 0;
#ifdef DEBUG
        for (u32 j = 0; j < 32; j++) {
            char c = *(src+j);
            if (isprint(c)) {
                cout << c;
            } else {
                cout << '_';
            }
        }
        cout << "|  ... number handling input\n";
#endif

    // categories to extract
    // Digits:
        // 0 (0x30) - bucket 0
        // 1-9 (never any distinction except if we didn't get the free kick at 0 due to the leading minus) (0x31-0x39) - bucket 1
    // . (0x2e) - bucket 2
    // E or e - no distinction (0x45/0x65) - bucket 3
    // + (0x2b) - bucket 4
    // - (0x2d) - bucket 4
    // Terminators
        // Whitespace: 0x20, 0x09, 0x0a, 0x0d - bucket 5+6
        // Comma and the closes: 0x2c is comma, } is 0x5d, ] is 0x7d - bucket 5+7

    // Another shufti - also a bit hand-hacked. Need to make a better construction
    const m256 low_nibble_mask = _mm256_setr_epi8(
    //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
       33,  2,  2,  2,  2, 10,  2,  2,  2, 66, 64, 16, 32,208,  4,  0,
       33,  2,  2,  2,  2, 10,  2,  2,  2, 66, 64, 16, 32,208,  4,  0
    );
    const m256 high_nibble_mask = _mm256_setr_epi8(
    //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
       64,  0, 52,  3,  8,128,  8,128,  0,  0,  0,  0,  0,  0,  0,  0,
       64,  0, 52,  3,  8,128,  8,128,  0,  0,  0,  0,  0,  0,  0,  0
    );

    m256 tmp = _mm256_and_si256(
                    _mm256_shuffle_epi8(low_nibble_mask, v),
                    _mm256_shuffle_epi8(high_nibble_mask,
                       _mm256_and_si256(_mm256_srli_epi32(v, 4), _mm256_set1_epi8(0x7f))));

    m256 enders_mask = _mm256_set1_epi8(0xe0);
    m256 tmp_enders = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, enders_mask),
                                    _mm256_set1_epi8(0));
    u32 enders = ~(u32)_mm256_movemask_epi8(tmp_enders);
    dumpbits32(enders, "ender characters");

    if (enders == 0) {
        // TODO: scream for help if enders == 0 which means we have
        // a heroically long number string or some garbage
    }
    // TODO: make a mask that indicates where our digits are
    u32 number_mask = ~enders & (enders-1);
    dumpbits32(number_mask, "number mask");

    m256 n_mask = _mm256_set1_epi8(0x1f);
    m256 tmp_n = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, n_mask),
                                    _mm256_set1_epi8(0));
    u32 number_characters = ~(u32)_mm256_movemask_epi8(tmp_n);

    // put something into our error sump if we have something
    // before our ending characters that isn't a valid character
    // for the inside of our JSON
    number_characters &= number_mask;
    error_sump |= number_characters ^ number_mask;
    dumpbits32(number_characters, "number characters");

    m256 d_mask = _mm256_set1_epi8(0x03);
    m256 tmp_d = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, d_mask),
                                    _mm256_set1_epi8(0));
    u32 digit_characters = ~(u32)_mm256_movemask_epi8(tmp_d);
    digit_characters &= number_mask;
    dumpbits32(digit_characters, "digit characters");

    m256 p_mask = _mm256_set1_epi8(0x04);
    m256 tmp_p = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, p_mask),
                                    _mm256_set1_epi8(0));
    u32 decimal_characters = ~(u32)_mm256_movemask_epi8(tmp_p);
    decimal_characters &= number_mask;
    dumpbits32(decimal_characters, "decimal characters");

    m256 e_mask = _mm256_set1_epi8(0x08);
    m256 tmp_e = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, e_mask),
                                    _mm256_set1_epi8(0));
    u32 exponent_characters = ~(u32)_mm256_movemask_epi8(tmp_e);
    exponent_characters &= number_mask;
    dumpbits32(exponent_characters, "exponent characters");

    m256 s_mask = _mm256_set1_epi8(0x10);
    m256 tmp_s = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, s_mask),
                                    _mm256_set1_epi8(0));
    u32 sign_characters = ~(u32)_mm256_movemask_epi8(tmp_s);
    sign_characters &= number_mask;
    dumpbits32(sign_characters, "sign characters");

    u32 digit_edges = ~(digit_characters << 1) & digit_characters;
    dumpbits32(digit_edges, "digit_edges");

    // check that we have 1-3 'edges' only
    u32 t = digit_edges;
    t &= t-1; t &= t-1; t &= t-1;
    error_sump |= t;

    // check that we start with a digit
    error_sump |= ~digit_characters & 0x1;

    // having done some checks, get lazy and fall back
    // to strtoll or strtod
    // TODO: handle the easy cases ourselves; these are
    // expensive and we've done a lot of the prepwork.
    // return errors if strto* fail, otherwise fill in a code on the tape
    // 'd' for floating point and 'l' for long and put a pointer to the
    // spot in the buffer.
    if (__builtin_popcount(digit_edges) == 1) {
        // try a strtoll
        char * end;
        u64 result = strtoll((const char *)src, &end, 10);
        if ((errno != 0) || (end == (const char *)src)) {
            error_sump |= 1;
        }
        error_sump |= is_not_structural_or_whitespace(*end);
        if (found_minus) {
            result = -result;
        }
#ifdef DEBUG
        cout << "Found number " << result << "\n";
#endif
        *((u64 *)current_number_buf_loc) = result;
        tape[tape_loc] = ((u32)'l') << 24 | (current_number_buf_loc - number_buf); // assume 2^24 will hold all numbers for now
        current_number_buf_loc += 8;
    } else {
        // try a strtod
        char * end;
        double result = strtod((const char *)src, &end);
        if ((errno != 0) || (end == (const char *)src)) {
            error_sump |= 1;
        }
        error_sump |= is_not_structural_or_whitespace(*end);
        if (found_minus) {
            result = -result;
        }
#ifdef DEBUG
        cout << "Found number " << result << "\n";
#endif
        *((double *)current_number_buf_loc) = result;
        tape[tape_loc] = ((u32)'d') << 24 | (current_number_buf_loc - number_buf); // assume 2^24 will hold all numbers for now
        current_number_buf_loc += 8;
    }
    // TODO: check the MSB element is a digit

    // TODO: a whole bunch of checks

    // TODO:  <=1 decimal point, eE mark, +- construct

    // TODO: first and last character in mask region must be
    // digit

    // TODO: if it exists,
    // Decimal point is after the first cluster of numbers only
    // and before the second cluster of numbers only. It must
    // be digit_or_zero . digit_or_zero strictly

    // TODO: eE mark and +- construct are adjacent with eE first
    // eE mark preceeds final cluster of numbers only
    // and immediately follows second-last cluster of numbers only (not
    // necessarily second, as we may have 4e10).
    // it may suffice to insist that eE is preceeded immediately
    // by a digit of any kind and that it's followed locally by
    // a digit immediately or a +- construct then a digit.

    // TODO: if we have both . and the eE mark then the . must
    // precede the eE mark

    // TODO: if first character is a zero (we know in advance except for -0)
    // second char must be . or eE.

    if (error_sump)
        return true;
    return true;
}

bool tape_disturbed(u32 i) {
    u32 start_loc = i*MAX_TAPE_ENTRIES;
    u32 end_loc = tape_locs[i];
    return start_loc != end_loc;
}

never_inline bool shovel_machine(const u8 * buf, size_t len, ParsedJson & pj) {
    // fixup the mess made by the ape_machine
    // as such it does a bunch of miscellaneous things on the tapes
    u32 error_sump = 0;
    u64 tv = *(const u64 *)"true    ";
    u64 nv = *(const u64 *)"null    ";
    u64 fv = *(const u64 *)"false   ";
    u64 mask4 = 0x00000000ffffffff;
    u64 mask5 = 0x000000ffffffffff;

    // if the tape has been touched at all at the depths outside the safe
    // zone we need to quit. Note that our periodic checks to see that we're
    // inside our safe zone in stage 3 don't guarantee that the system did
    // not get into the danger area briefly.
    if (tape_disturbed(START_DEPTH - 1) || tape_disturbed(REDLINE_DEPTH)) {
        return false;
    }

    // walk over each tape
    for (u32 i = START_DEPTH; i < MAX_DEPTH; i++) {
        u32 start_loc = i*MAX_TAPE_ENTRIES;
        u32 end_loc = tape_locs[i];
        if (start_loc == end_loc) {
            break;
        }
        for (u32 j = start_loc; j < end_loc; j++) {
            switch (tape[j]>>56) {
            case '{': case '[': {
                // pivot our tapes
                // point the enclosing structural char (}]) to the head marker ({[) and
                // put the end of the sequence on the tape at the head marker
                // we start with head marker pointing at the enclosing structural char
                // and the enclosing structural char pointing at the end. Just swap them.
                // also check the balanced-{} or [] property here
                u8 head_marker_c = tape[j] >> 56;
                u32 head_marker_loc = tape[j] & 0xffffffffffffffULL;
                u64 tape_enclosing = tape[head_marker_loc];
                u8 enclosing_c = tape_enclosing >> 56;
                tape[head_marker_loc] = tape[j];
                tape[j] = tape_enclosing;
                error_sump |= (enclosing_c - head_marker_c - 2); // [] and {} only differ by 2 chars
                break;
            }
            case '"': {
                error_sump |= !parse_string(buf, len, pj, j);
                break;
            }
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                error_sump |= !parse_number(buf, len, pj, j, false, false);
                break;
            case '0':
                error_sump |= !parse_number(buf, len, pj, j, true, false);
                break;
            case '-':
                error_sump |= !parse_number(buf, len, pj, j, false, true);
                break;
            case 't':  {
                u32 offset = tape[j] & 0xffffffffffffffULL;
                const u8 * loc = buf + offset;
                error_sump |= ((*(const u64 *)loc) & mask4) ^ tv;
                error_sump |= is_not_structural_or_whitespace(loc[4]);
                break;
            }
            case 'f':  {
                u32 offset = tape[j] & 0xffffffffffffffULL;
                const u8 * loc = buf + offset;
                error_sump |= ((*(const u64 *)loc) & mask5) ^ fv;
                error_sump |= is_not_structural_or_whitespace(loc[5]);
                break;
            }
            case 'n':  {
                u32 offset = tape[j] & 0xffffffffffffffULL;
                const u8 * loc = buf + offset;
                error_sump |= ((*(const u64 *)loc) & mask4) ^ nv;
                error_sump |= is_not_structural_or_whitespace(loc[4]);
                break;
            }
            default:
                break;
            }
        }
    }
    if (error_sump) {
        cerr << "Ugh!\n";
        return false;
    }
    return true;
}






static bool avx_json_parse(const u8 * buf,  size_t len, ParsedJson & pj) {
          find_structural_bits(buf, len, pj);
          flatten_indexes(len, pj);
          bool apeok = ape_machine(buf, len, pj);
          if(!apeok) {
            return false;
          }
          return  shovel_machine(buf, len, pj);
}
