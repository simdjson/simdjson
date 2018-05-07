#include "linux-perf-events.h"
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

using namespace std;

//#define DEBUG

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
#else
#define dump256(a,b) ;
#define dumpbits(a,b) ;
#endif

// get a corpus; pad out to cache line so we can always use SIMD
pair<u8 *, size_t> get_corpus(string filename) {
ifstream is(filename, ios::binary);
    if (is) {
        stringstream buffer;
        buffer << is.rdbuf();
        size_t length = buffer.str().size();
        char * aligned_buffer;
        if (posix_memalign( (void **)&aligned_buffer, 64, ROUNDUP_N(length, 64))) {
            cerr << "Could not allocate memory\n";
            exit(1);
        };
        memset(aligned_buffer, 0x20, ROUNDUP_N(length, 64));
        memcpy(aligned_buffer, buffer.str().c_str(), length);
        is.close();
        return make_pair((u8 *)aligned_buffer, length);
    }
    throw "No corpus";
    return make_pair((u8 *)0, (size_t)0);
}

struct JsonNode {
    u32 next;
    u32 next_type;
    u64 payload; // a freeform 'payload' holding a parsed representation of *something*
};

struct ParsedJson {
    u8 * structurals;
    u32 n_structural_indexes;
    u32 * structural_indexes;
    JsonNode * nodes;
};

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
    u32 base = NUM_RESERVED_NODES;
    u32 * base_ptr = pj.structural_indexes;
    base_ptr[DUMMY_NODE] = base_ptr[ROOT_NODE] = 0; // really shouldn't matter
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
u32 tape[MAX_TAPE]; 
u32 tape_locs[MAX_DEPTH];

// STATE MACHINE DECLARATIONS

const u32 MAX_STATES = 16;


u32 trans[MAX_STATES][256];

u32 states[MAX_DEPTH];
const int START_STATE = 1;
never_inline void init_state_machine() {
    // states 10 and 6 eliminated

    trans[ 1]['{'] = 2;
    trans[ 2]['"'] = 4;
    trans[ 4][':'] = 5;
    // 5->7 on all unary values ftn0123456789-"
    trans[ 7][','] = 8;
    trans[ 8]['"'] = 4;

    trans[ 1]['['] = 9;
    // 9->11 on all unary values ftn0123456789-"
    trans[11][','] = 12;
    // 12->11 on all unary values ftn0123456789-"

    const char * UNARIES = "}]ftn0123456789-\"";
    for (u32 i = 0; i < strlen(UNARIES); i++) {
        trans[ 5][(u32)UNARIES[i]] = 7;
        trans[ 9][(u32)UNARIES[i]] = 11;
        trans[12][(u32)UNARIES[i]] = 11;
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

    u32 depth = 1;

    for (u32 i = 0; i < MAX_DEPTH; i++) {
        tape_locs[i] = i*MAX_TAPE_ENTRIES;
        states[i] = START_STATE;
    }

    u32 error_sump = 0;
    u32 old_tape_loc = tape_locs[depth]; // need to initialize for first write

    u32 next_idx = pj.structural_indexes[0];
    u8 next_c = buf[next_idx];
    u32 next_control = char_control[next_c];

    // To try: figure out idx, c, depth adjust, write size, write val and maybe even depth in One Giant Pass
    // then do the remainder of the loop. The interesting question is whether this loop is best pulled apart
    // into different passes

    for (u32 i = NUM_RESERVED_NODES; i < pj.n_structural_indexes; i++) {
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
        tape[tape_locs[depth]] = write_val | (c << 24); // hack. Assumes no more than 2^24 tape items and buffer size for now
        old_tape_loc = tape_locs[depth] += write_size;
    }

#define DUMP_TAPES
#ifdef DEBUG
    for (u32 i = 0; i < MAX_DEPTH; i++) {
        u32 start_loc = i*MAX_TAPE_ENTRIES;
        cout << " tape section i " << i << " from: " << start_loc 
             << " to: " << tape_locs[i] << " "
             << " size: " << (tape_locs[i]-start_loc) << "\n";
        cout << " state: " << states[i] << "\n"; 
#ifdef DUMP_TAPES
        for (u32 j = start_loc; j < tape_locs[i]; j++) {
            if (tape[j]) {
                cout << "j: " << j << " tape[j] char " << (char)(tape[j]>>24) 
                     << " tape[j][0..23]: " << (tape[j]&0xffffff) << "\n";
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


u32 count_tapes;
u32 count_opens;
u32 count_strings;
u32 count_non_zeros;
u32 count_leading_zeros;
u32 count_minus;
u32 count_true;
u32 count_false;
u32 count_null;

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

never_inline bool shovel_machine(UNUSED const u8 * buf, UNUSED size_t len, UNUSED ParsedJson & pj) {
    // fixup the mess made by the ape_machine
    // as such it does a bunch of miscellaneous things on the tapes
    
    u32 error_sump = 0;

    u64 tv = *(const u64 *)"true    ";
    u64 nv = *(const u64 *)"null    ";
    u64 fv = *(const u64 *)"false   ";

    u64 mask4 = 0x00000000ffffffff;
    u64 mask5 = 0x000000ffffffffff;

    // walk over each tape
    for (u32 i = 0; i < MAX_DEPTH; i++) {
        u32 start_loc = i*MAX_TAPE_ENTRIES;
        u32 end_loc = tape_locs[i];
        for (u32 j = start_loc; j < end_loc; j++) {
            count_tapes++;
            switch (tape[j]>>24) {
            case '{': case '[': {
                count_opens++;
                // TODO: pivot our tapes
                // point the enclosing structural char (}]) to the head marker ({[) and
                // put the end of the sequence on the tape at the head marker
                // we start with head marker pointing at the enclosing structural char
                // and the enclosing structural char pointing at the end. Just swap them.
                // also check the balanced-{} or [] property here
                u8 head_marker_c = tape[j] >> 24;
                u32 head_marker_loc = tape[j] & 0xffffff;    
                u32 tape_enclosing = tape[head_marker_loc];
                u8 enclosing_c = tape_enclosing >> 24;
                tape[head_marker_loc] = tape[j]; 
                tape[j] = tape_enclosing;
                //cout << "head_marker_c: " << head_marker_c << " enclosing_c " << enclosing_c << "\n";
                error_sump |= (enclosing_c - head_marker_c - 2); // [] and {} only differ by 2 chars
                break;
            }
            case '"':
                count_strings++;
                // TODO: normalize strings
                break;
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                count_non_zeros++;
                // TODO: read in a number
                break;
            case '0':
                count_leading_zeros++;
                // TODO: read in a number. Must be float so we can skip some stuff.
                break;
            case '-': 
                count_minus++;
                // TODO: read in a number 
                break;
            case 't':  {
                count_true++;
                u32 offset = tape[j] & 0xffffff;    
                const u8 * loc = buf + offset;
                error_sump |= ((*(const u64 *)loc) & mask4) ^ tv;
                error_sump |= is_not_structural_or_whitespace(loc[4]);
                break;
            }
            case 'f':  {
                count_false++;
                u32 offset = tape[j] & 0xffffff;    
                const u8 * loc = buf + offset;
                error_sump |= ((*(const u64 *)loc) & mask5) ^ fv;
                error_sump |= is_not_structural_or_whitespace(loc[5]);
                break;
            }
            case 'n':  {
                count_null++;
                u32 offset = tape[j] & 0xffffff;    
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

// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
namespace Color {
    enum Code {
        FG_DEFAULT = 39, FG_BLACK = 30, FG_RED = 31, FG_GREEN = 32,
        FG_YELLOW = 33, FG_BLUE = 34, FG_MAGENTA = 35, FG_CYAN = 36,
        FG_LIGHT_GRAY = 37, FG_DARK_GRAY = 90, FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92, FG_LIGHT_YELLOW = 93, FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95, FG_LIGHT_CYAN = 96, FG_WHITE = 97,
        BG_RED = 41, BG_GREEN = 42, BG_BLUE = 44, BG_DEFAULT = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}

void colorfuldisplay(ParsedJson & pj, const u8 * buf) {
    Color::Modifier greenfg(Color::FG_GREEN);
    Color::Modifier yellowfg(Color::FG_YELLOW);
    Color::Modifier deffg(Color::FG_DEFAULT);
    size_t i = 0;
    // skip initial fluff
    while((i+1< pj.n_structural_indexes) && (pj.structural_indexes[i]==pj.structural_indexes[i+1])){
      i++;
    }
    for (; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        u8 c = buf[idx];
        if (((c & 0xdf) == 0x5b)) { // meaning 7b or 5b, { or [
            std::cout << greenfg <<  buf[idx] << deffg;
        } else if (((c & 0xdf) == 0x5d)) { // meaning 7d or 5d, } or ]
            std::cout << greenfg <<  buf[idx] << deffg;
        } else {
            std::cout << yellowfg <<  buf[idx] << deffg;
        }
        if(i + 1 < pj.n_structural_indexes) {
          u32 nextidx = pj.structural_indexes[i + 1];
          for(u32 pos = idx + 1 ; pos < nextidx; pos++) {
            std::cout << buf[pos];
          }
        }
    }
    std::cout << std::endl;
}
int main(int argc, char * argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <jsonfile>\n";
        exit(1);
    }
    pair<u8 *, size_t> p = get_corpus(argv[1]);
    ParsedJson pj;

    if (posix_memalign( (void **)&pj.structurals, 8, ROUNDUP_N(p.second, 64)/8)) {
        cerr << "Could not allocate memory\n";
        exit(1);
    };

    if (p.second > 0xffffff) {
        cerr << "Currently only support JSON files < 16MB\n";
        exit(1);
    }
    init_state_machine();

    pj.n_structural_indexes = 0;
    // we have potentially 1 structure per byte of input
    // as well as a dummy structure and a root structure
    // we also potentially write up to 7 iterations beyond
    // in our 'cheesy flatten', so make some worst-case
    // space for that too
    u32 max_structures = ROUNDUP_N(p.second, 64) + 2 + 7;
    pj.structural_indexes = new u32[max_structures];
    pj.nodes = new JsonNode[max_structures];

#if defined(DEBUG) 
    const u32 iterations = 1;
#else
    const u32 iterations = 1000;
#endif
    vector<double> res;
    res.resize(iterations);

#if !defined(__linux__)
#define SQUASH_COUNTERS
#endif

#ifndef SQUASH_COUNTERS
    LinuxEvents<PERF_TYPE_HARDWARE> cycles(PERF_COUNT_HW_CPU_CYCLES);
    LinuxEvents<PERF_TYPE_HARDWARE> instructions(PERF_COUNT_HW_INSTRUCTIONS);
    unsigned long cy1 = 0, cy2 = 0, cy3 = 0, cy4 = 0;
    unsigned long cl1 = 0, cl2 = 0, cl3 = 0, cl4 = 0;
#endif
    for (u32 i = 0; i < iterations; i++) {
        auto start = std::chrono::steady_clock::now();
#ifndef SQUASH_COUNTERS
        cycles.start(); instructions.start();
#endif
        find_structural_bits(p.first, p.second, pj);
#ifndef SQUASH_COUNTERS
        cl1 += instructions.end(); cy1 += cycles.end();
        cycles.start(); instructions.start();
#endif
        flatten_indexes(p.second, pj);
#ifndef SQUASH_COUNTERS
        cl2 += instructions.end(); cy2 += cycles.end();
        cycles.start(); instructions.start();
#endif
        ape_machine(p.first, p.second, pj);
#ifndef SQUASH_COUNTERS
        cl3 += instructions.end(); cy3 += cycles.end();
        cycles.start(); instructions.start();
#endif
        shovel_machine(p.first, p.second, pj);
#ifndef SQUASH_COUNTERS
        cl4 += instructions.end(); cy4 += cycles.end();
#endif
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> secs = end - start;
        res[i] = secs.count();
    }
#ifndef SQUASH_COUNTERS
    printf("number of bytes %ld number of structural chars %d ratio %.3f\n", p.second, pj.n_structural_indexes,
           (double) pj.n_structural_indexes / p.second);
    unsigned long total = cy1 + cy2  + cy3  + cy4;

    printf("stage 1 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f \n",
         cl1, cy1, 100. *  cy1 / total, (double) cl1 / cy1);
    printf(" stage 1 runs at %.2f cycles per input byte.\n", (double) cy1 / (iterations * p.second));

    printf("stage 2 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f \n",
         cl2, cy2, 100. *  cy2 / total, (double) cl2 / cy2);
    printf(" stage 2 runs at %.2f cycles per input byte and ", (double) cy2 / (iterations * p.second));
    printf("%.2f cycles per structural character.\n", (double) cy2 / (iterations * pj.n_structural_indexes));

    printf("stage 3 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f \n",
         cl3, cy3, 100. * cy3 / total, (double) cl3 / cy3);
    printf(" stage 3 runs at %.2f cycles per input byte and ", (double) cy3 / (iterations * p.second));
    printf("%.2f cycles per structural character.\n", (double) cy3 / (iterations * pj.n_structural_indexes));

    printf("stage 4 instructions: %10lu cycles: %10lu (%.2f %%) ins/cycles: %.2f \n",
         cl4, cy4, 100. * cy4 / total, (double) cl4 / cy4);
    printf(" stage 4 runs at %.2f cycles per input byte and ", (double) cy4 / (iterations * p.second));
    printf("%.2f cycles per structural character.\n", (double) cy4 / (iterations * pj.n_structural_indexes));

    printf("There were %d elements on our tapes.\n", count_tapes);
    printf("Opens %d strings %d non_zeros %d leading_zeros %d minus %d, true %d false %d null %d\n", 
        count_opens, count_strings, count_non_zeros, count_leading_zeros, count_minus, count_true, count_false, count_null);

    printf(" all stages: %.2f cycles per input byte.\n", (double) total / (iterations * p.second));
#endif
//    colorfuldisplay(pj, p.first);
	double min_result = *min_element(res.begin(), res.end());
	cout << "Min:  " << min_result << " bytes read: " << p.second  << " Gigabytes/second: " << (p.second) / (min_result * 1000000000.0) << "\n";
    return 0;
}
