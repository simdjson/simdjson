#pragma once

#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <iostream>
#define MAX_JSON_BYTES 0xFFFFFF

const u32 MAX_DEPTH = 256;
const u32 DEPTH_SAFETY_MARGIN = 32; // should be power-of-2 as we check this
                                    // with a modulo in our hot stage 3 loop
const u32 START_DEPTH = DEPTH_SAFETY_MARGIN;
const u32 REDLINE_DEPTH = MAX_DEPTH - DEPTH_SAFETY_MARGIN;
const size_t MAX_TAPE_ENTRIES = 127 * 1024;
const size_t MAX_TAPE = MAX_DEPTH * MAX_TAPE_ENTRIES;

struct ParsedJson {
  size_t bytecapacity; // indicates how many bits are meant to be supported by
                       // structurals
  u8 *structurals;
  u32 n_structural_indexes;
  u32 *structural_indexes;

  // grossly overprovisioned
  u64 tape[MAX_TAPE];
  u32 tape_locs[MAX_DEPTH];
  u8 string_buf[MAX_JSON_BYTES];
  u8 *current_string_buf_loc;
  u8 number_buf[MAX_JSON_BYTES * 4]; // holds either doubles or longs, really
  u8 *current_number_buf_loc;
};

// all of this stuff needs to get moved somewhere reasonable
// like our ParsedJson structure
/*
extern u64 tape[MAX_TAPE];
extern u32 tape_locs[MAX_DEPTH];
extern u8 string_buf[512*1024];
extern u8 * current_string_buf_loc;
extern u8 number_buf[512*1024]; // holds either doubles or longs, really
extern u8 * current_number_buf_loc;
*/

#ifdef DEBUG
inline void dump256(m256 d, string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << setw(3) << (int)*(((u8 *)(&d)) + i);
    if (!((i + 1) % 8))
      std::cout << "|";
    else if (!((i + 1) % 4))
      std::cout << ":";
    else
      std::cout << " ";
  }
  std::cout << " " << msg << "\n";
}

// dump bits low to high
void dumpbits(u64 v, string msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

void dumpbits32(u32 v, string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
#else
#define dump256(a, b) ;
#define dumpbits(a, b) ;
#define dumpbits32(a, b) ;
#endif
