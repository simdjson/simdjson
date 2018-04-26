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
            throw "Allocation failed";
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
            // spoil the suspense
            u64 s3 = _pdep_u64(~0x7ULL, s); // s3 will have bottom 3 1-bits unset
            u64 s5 = _pdep_u64(~0x1fULL, s); // s5 will have bottom 5 1-bits unset

            base_ptr[base+0] = (u32)idx + __builtin_ctzll(s);  u64 s1 = s  & (s  - 1ULL);
            base_ptr[base+1] = (u32)idx + __builtin_ctzll(s1); u64 s2 = s1 & (s1 - 1ULL);
            base_ptr[base+2] = (u32)idx + __builtin_ctzll(s2); //u64 s3 = s2 & (s2 - 1ULL);
            base_ptr[base+3] = (u32)idx + __builtin_ctzll(s3); u64 s4 = s3 & (s3 - 1ULL);

            base_ptr[base+4] = (u32)idx + __builtin_ctzll(s4); //u64 s5 = s4 & (s4 - 1ULL);
            base_ptr[base+5] = (u32)idx + __builtin_ctzll(s5); u64 s6 = s5 & (s5 - 1ULL);
            base_ptr[base+6] = (u32)idx + __builtin_ctzll(s6); u64 s7 = s6 & (s6 - 1ULL);
            s = s7;
            base += 7;
        }
        base = next_base;
#endif
    }
    pj.n_structural_indexes = base;
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

const u32 DEPTH_PLUS_ONE = 0x2;
const u32 DEPTH_ZERO = 0x1;
const u32 DEPTH_MINUS_ONE = 0x0;
const u32 TAKE_UPTAPE = 0x80000000;
const u32 TAKE_INDEX = 0x0;
const u32 WRITE_ZERO = 0x0;
const u32 WRITE_FOUR = 0x4;
const u32 WRITE_EIGHT = 0x8;

const u32 CDEF = DEPTH_ZERO | TAKE_INDEX | WRITE_ZERO;
const u32 C0I4 = DEPTH_ZERO | TAKE_INDEX | WRITE_FOUR;
const u32 C0I8 = DEPTH_ZERO | TAKE_INDEX | WRITE_FOUR;
const u32 CPI0 = DEPTH_PLUS_ONE | TAKE_INDEX | WRITE_ZERO;
const u32 CMU8 = DEPTH_MINUS_ONE | TAKE_UPTAPE | WRITE_EIGHT;

inline s8 get_depth_adjust(u32 control) { return (s8)(control&0x3) - 1; }
inline bool is_uptape(u32 control) { return (control & TAKE_UPTAPE); }
inline size_t get_write_size(u32 control) { return control & 12; }

const u32 char_control[256] = {
    // nothing interesting from 0x00-0x20
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF,
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 

    // " is 0x22, - is 0x2d
    CDEF,CDEF,C0I4,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,C0I8,CDEF,CDEF,

    // numbers are 0x30-0x39
    C0I8,C0I8,C0I8,C0I8, C0I8,C0I8,C0I8,C0I8, C0I8,C0I8,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF,

    // nothing interesting from 0x40-0x49
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 

    // 0x5b/5d are []
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CPI0, CDEF,CMU8,CDEF,CDEF, 

    // nothing interesting from 0x60-0x69
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 

    // 0x7b/7d are {}
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CPI0, CDEF,CMU8,CDEF,CDEF, 

    // nothing interesting from 0x80-0xff
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF,
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF,
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, 
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF,
    CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF, CDEF,CDEF,CDEF,CDEF
};

const size_t MAX_TAPE_ENTRIES = 1024*1024;
const size_t MAX_TAPE = MAX_DEPTH * MAX_TAPE_ENTRIES;
u32 tape[MAX_TAPE]; 

// STATE MACHINE DECLARATIONS

const u32 MAX_STATES = 16;


u32 trans[MAX_STATES][256];
u32 disallow_exit[MAX_STATES][256];

u32 states[MAX_DEPTH];
const int START_STATE = 1;
never_inline void init_state_machine() {
    trans[ 1]['{'] = 2;
    trans[ 2]['"'] = 3;
    trans[ 3]['"'] = 4;
    trans[ 4][':'] = 5;
    trans[ 5]['"'] = 6;
    trans[ 6]['"'] = 7;
    // 5->7 on all unary values ftn0123456789-
    trans[ 7][','] = 8;
    trans[ 8]['"'] = 3;

    trans[ 1]['['] = 9;
    trans[ 9]['"'] = 10;
    trans[10]['"'] = 11;
    // 9->11 on all unary values ftn0123456789-
    trans[11][','] = 12;
    trans[12]['"'] = 10;
    // 12->11 on all unary values ftn0123456789-

    const char * UNARIES = "}]ftn0123456789-";
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
    
    // note - extra-linguistic stuff in the DFA
    // when we are in 2/7 we are OK to see a } at the shallower depth
    // when we are in 9/11 we are OK to see a ] at the shallower depth
    // nothing else should be illegal through this mechanism
    for (u32 i = 0; i < MAX_STATES; i++) {
        if ((i != 2) && (i != 7))
            disallow_exit[i]['}'] = 1;
        if ((i != 9) && (i != 11))
            disallow_exit[i][']'] = 1;
    }
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
    u32 tape_locs[MAX_DEPTH];

    for (u32 i = 0; i < MAX_DEPTH; i++) {
        tape_locs[i] = i*MAX_TAPE_ENTRIES;
        states[i] = START_STATE;
    }

    u32 error_sump = 0;
    u32 old_state = 0; // experimental
    for (u32 i = NUM_RESERVED_NODES; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        u8 c = buf[idx];
#ifdef DEBUG
        cout << "i: " << i << " idx: " << idx << " c " << c << "\n";
#endif
        // TAPE MACHINE

        u32 control = char_control[c];
        s8 depth_adjust = get_depth_adjust(control);
        bool take_uptape = is_uptape(control);
        u8 write_size = get_write_size(control)/4;
        depth += depth_adjust;
#ifdef DEBUG
        cout << "TAPE MACHINE: depth change " << (s32)depth_adjust << " take_uptape: " << (u32)take_uptape 
             << " write_size " << (u32)write_size << " current_depth: " << depth << "\n";  
#endif
        u32 uptape = tape_locs[depth+1];
        tape[tape_locs[depth]] = take_uptape ? uptape : idx;
        tape_locs[depth] += write_size;

        // STATE MACHINE
#ifdef DEBUG
        cout << "STATE MACHINE: error_sump: " << error_sump << " old state " << old_state  << " disallowed_exit[old_state][c]: " << disallow_exit[old_state][c] << "\n";
        cout << "STATE MACHINE: state[depth] pre " << states[depth] << " ";
#endif
        error_sump |= disallow_exit[old_state][c];
        old_state  = states[depth] = trans[states[depth]][c];
#ifdef DEBUG
        cout << "post " << states[depth] << "\n";
#endif
    }

#ifdef DEBUG
    for (u32 i = 0; i < MAX_DEPTH; i++) {
        u32 start_loc = i*MAX_TAPE_ENTRIES;
        cout << " tape section i " << i << " from: " << start_loc 
             << " to: " << tape_locs[i] << " "
             << " size: " << (tape_locs[i]-start_loc) << "\n";
        cout << " state: " << states[i] << "\n"; 
/*
        for (u32 j = start_loc; j < tape_locs[i]; j++) {
            cout << "j: " << j << " tape[j]: " << tape[j] << "\n";
        }
*/
    }
#endif
    if (error_sump) {
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
        throw "Allocation failed";
    };

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

#ifdef __linux__
    LinuxEvents<PERF_TYPE_HARDWARE> cycles(PERF_COUNT_HW_CPU_CYCLES);
    LinuxEvents<PERF_TYPE_HARDWARE> instructions(PERF_COUNT_HW_INSTRUCTIONS);
    unsigned long cy1 = 0, cy2 = 0, cy3 = 0;
    unsigned long cl1 = 0, cl2 = 0, cl3 = 0;
#endif
    for (u32 i = 0; i < iterations; i++) {
        auto start = std::chrono::steady_clock::now();
#ifdef __linux__
        cycles.start(); instructions.start();
#endif
        find_structural_bits(p.first, p.second, pj);
#ifdef __linux__
        cy1 += cycles.end(); cl1 += instructions.end();
        cycles.start(); instructions.start();
#endif
        flatten_indexes(p.second, pj);
#ifdef __linux__
        cy2 += cycles.end(); cl2 += instructions.end();
        cycles.start(); instructions.start();
#endif
        ape_machine(p.first, p.second, pj);
#ifdef __linux__
        cy3 += cycles.end(); cl3 += instructions.end();
#endif
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> secs = end - start;
        res[i] = secs.count();
    }
#ifdef __linux__
    unsigned long total = cy1 + cy2  + cy3 ;
    printf("stage 1 instructions: %10lu cycles: %10lu (%.1f %%) ins/cycles: %.2f \n", 
         cy1, cl1, 100. *  cy1 / total, (double) cl1 / cy1);
    printf("stage 2 instructions: %10lu cycles: %10lu (%.1f %%) ins/cycles: %.2f \n", 
         cy2, cl2, 100. *  cy2 / total, (double) cl2 / cy2);
    printf("stage 3 instructions: %10lu cycles: %10lu (%.1f %%) ins/cycles: %.2f \n", 
         cy3, cl3, 100. * cy3 / total, (double) cl3 / cy3);
#endif
//    colorfuldisplay(pj, p.first);
	double min_result = *min_element(res.begin(), res.end());
	cout << "Min:  " << min_result << " bytes read: " << p.second  << " Gigabytes/second: " << (p.second) / (min_result * 1000000000.0) << "\n";
    return 0;
}
