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

// TODO: move this to be more like a real class
struct ParsedJson {
public:
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
    
    // TODO: will need a way of saving strings that's a bit more encapsulated

    void write_tape(u32 depth, u64 val, u8 c) {
        tape[tape_locs[depth]] = val | (((u64)c) << 56);
        tape_locs[depth]++;
    }

    void write_tape_s64(u32 depth, s64 i) {
        *((s64 *)current_number_buf_loc) = i;
        current_number_buf_loc += 8;
        write_tape(depth, current_number_buf_loc - number_buf, 'l');
    }

    void write_tape_double(u32 depth, double d) {
        *((double *)current_number_buf_loc) = d;
        current_number_buf_loc += 8;
        write_tape(depth, current_number_buf_loc - number_buf, 'd');
    }

    u32 save_loc(u32 depth) {
        return tape_locs[depth];
    }

    void write_saved_loc(u32 saved_loc, u64 val, u8 c) {
        tape[saved_loc] = val | (((u64)c) << 56);
    }


};


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

// dump bits low to high
inline void dumpbits_always(u64 v, std::string msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

inline void dumpbits32_always(u32 v, std::string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
