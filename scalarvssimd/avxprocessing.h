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
    u64 prev_iter_pseudo_structural_carry = 0ULL;

    for (size_t idx = 0; idx < len; idx+=64) {
        m256 input_lo = _mm256_load_si256((const m256 *)(buf + idx + 0));
        m256 input_hi = _mm256_load_si256((const m256 *)(buf + idx + 32));

        ////////////////////////////////////////////////////////////////////////////////////////////
        //     Step 1: detect odd sequences of backslashes
        ////////////////////////////////////////////////////////////////////////////////////////////

        u64 bs_bits = cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
        u64 start_edges = bs_bits & ~(bs_bits << 1);

        // flip lowest if we have an odd-length run at the end of the prior iteration
        u64 even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
        u64 even_starts = start_edges & even_start_mask;
        u64 odd_starts = start_edges & ~even_start_mask;

        u64 even_carries = bs_bits + even_starts;

        u64 odd_carries;
        // must record the carry-out of our odd-carries out of bit 63; this indicates whether the
        // sense of any edge going to the next iteration should be flipped
        bool iter_ends_odd_backslash = __builtin_uaddll_overflow(bs_bits, odd_starts, &odd_carries);

        odd_carries |= prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                                     // if we had an odd-numbered run at the end of
                                                     // the previous iteration
        prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;

        u64 even_carry_ends = even_carries & ~bs_bits;
        u64 odd_carry_ends = odd_carries & ~bs_bits;

        u64 even_start_odd_end = even_carry_ends & odd_bits;
        u64 odd_start_even_end = odd_carry_ends & even_bits;

        u64 odd_ends = even_start_odd_end | odd_start_even_end;

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


        // mask off anything inside quotes
        structurals &= ~quote_mask;

        // whitespace inside our quotes also doesn't count; otherwise "    foo" would generate a spurious
        // pseudo-structural-character at 'foo'
        whitespace &= ~quote_mask;

        // add the real quote bits back into our bitmask as well, so we can
        // quickly traverse the strings we've spent all this trouble gathering
        structurals |= quote_bits;

        // Now, establish "pseudo-structural characters". These are characters that follow a structural
        // character followed by zero or more  whitespace
        // this allows us to discover true/false/null and numbers in any location where they might legally
        // occur; it will also create another 'checkpoint' where if a non-quoted region of our input
        // has whitespace after a structural character fullowed by a syntax error, we can detect this
        // and get an error in a later stage (i.e. the state machine)

        // Slightly more painful than it would seem. It's possible that either structurals or whitespace are
        // all 1s (e.g. {{{{{{{....{{{{x64, or a really long whitespace). As such there is no safe place
        // to add a '1' from the previous iteration without *that* triggering the carry we are looking
        // out for, so we must check both carries for overflow

        u64 tmp = structurals | whitespace;
        u64 tmp2;
        bool ps_carry = __builtin_uaddll_overflow(tmp, structurals, &tmp2);
        u64 tmp3;
        ps_carry = ps_carry | __builtin_uaddll_overflow(tmp2, prev_iter_pseudo_structural_carry, &tmp3);
        prev_iter_pseudo_structural_carry = ps_carry ? 0x1ULL : 0x0ULL;
        tmp3 &= ~quote_mask;
        tmp3 &= ~whitespace;
        structurals |= tmp3;

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
        while (s) {
            u32 si = (u32)idx + __builtin_ctzll(s);
            base_ptr[base++] = si;
            s &= s - 1ULL;
        }
    }
    pj.n_structural_indexes = base;
    return true;
}

// Parse our json given a big array of 32-bit integers telling us where
// the interesting stuff is
bool avx_json_parse(const u8 * buf, UNUSED size_t len, ParsedJson & pj) {
    u32 last; // index of previous structure at this level or 0 if none
    u32 up; // index of structure that contains this one

    JsonNode * nodes = pj.nodes;

    JsonNode & dummy = nodes[DUMMY_NODE];
    JsonNode & root = nodes[ROOT_NODE];
    dummy.prev = dummy.up = DUMMY_NODE;
    root.prev = DUMMY_NODE;
    root.up = ROOT_NODE;
    last = up = ROOT_NODE;

    for (u32 i = NUM_RESERVED_NODES; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        JsonNode & n = nodes[i];
        u8 c = buf[idx];
        if (unlikely((c & 0xdf) == 0x5b)) { // meaning 7b or 5b, { or [
            // open a scope
            n.prev = last;
            n.up = up;
            up = i;
            last = 0;
        } else if (unlikely((c & 0xdf) == 0x5d)) { // meaning 7d or 5d, } or ]
            // close a scope
            n.prev = up;
            n.up = pj.nodes[up].up;
            up = pj.nodes[up].up;
            last = i;
        } else {
            n.prev = last;
            n.up = up;
            last = i;
        }
        n.next = 0;
        nodes[n.prev].next = i;
    }
    dummy.next = DUMMY_NODE; // dummy.next is a sump for meaningless 'nexts', clear it
#ifdef DEBUG
    for (u32 i = 0; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        JsonNode & n = nodes[i];
        cout << "i: " << i;
        cout << " n.up: " << n.up;
        cout << " n.next: " << n.next;
        cout << " n.prev: " << n.prev;
        cout << " idx: " << idx << " buf[idx] " << buf[idx] << "\n";
    }
#endif
    return true;
}
