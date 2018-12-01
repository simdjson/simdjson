#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cassert>

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"

#ifndef NO_PDEP_PLEASE
#define NO_PDEP_PLEASE // though this is not always a win, it seems to 
// be more often a win than not. And it will be faster on AMD.
#endif

#ifndef NO_PDEP_WIDTH
#define NO_PDEP_WIDTH 8
#endif

#define SET_BIT(i)                                                             \
  base_ptr[base + i] = (u32)idx + __tzcnt_u64(s);                          \
  s = s & (s - 1);

#define SET_BIT1 SET_BIT(0)
#define SET_BIT2 SET_BIT1 SET_BIT(1)
#define SET_BIT3 SET_BIT2 SET_BIT(2)
#define SET_BIT4 SET_BIT3 SET_BIT(3)
#define SET_BIT5 SET_BIT4 SET_BIT(4)
#define SET_BIT6 SET_BIT5 SET_BIT(5)
#define SET_BIT7 SET_BIT6 SET_BIT(6)
#define SET_BIT8 SET_BIT7 SET_BIT(7)
#define SET_BIT9 SET_BIT8 SET_BIT(8)
#define SET_BIT10 SET_BIT9 SET_BIT(9)
#define SET_BIT11 SET_BIT10 SET_BIT(10)
#define SET_BIT12 SET_BIT11 SET_BIT(11)
#define SET_BIT13 SET_BIT12 SET_BIT(12)
#define SET_BIT14 SET_BIT13 SET_BIT(13)
#define SET_BIT15 SET_BIT14 SET_BIT(14)
#define SET_BIT16 SET_BIT15 SET_BIT(15)

#define CALL(macro, ...) macro(__VA_ARGS__)

#define SET_BITLOOPN(n) SET_BIT##n

// just transform the bitmask to a big list of 32-bit integers for now
// that's all; the type of character the offset points to will
// tell us exactly what we need to know. Naive but straightforward
// implementation
WARN_UNUSED
bool flatten_indexes(size_t len, ParsedJson &pj) {
  u32 *base_ptr = pj.structural_indexes;
  u32 base = 0;
#ifdef BUILDHISTOGRAM
  uint32_t counters[66];
  uint32_t total = 0;
  for (int k = 0; k < 66; k++)
    counters[k] = 0;
  for (size_t idx = 0; idx < len; idx += 64) {
    u64 s = *(u64 *)(pj.structurals + idx / 8);
    u32 cnt = __builtin_popcountll(s);
    total++;
    counters[cnt]++;
  }
  printf("\n histogram:\n");
  for (int k = 0; k < 66; k++) {
    if (counters[k] > 0)
      printf("%10d %10.u %10.3f \n", k, counters[k], counters[k] * 1.0 / total);
  }
  printf("\n\n");
#endif
  for (size_t idx = 0; idx < len; idx += 64) {
    u64 s = *(u64 *)(pj.structurals + idx / 8);
#ifdef SUPPRESS_CHEESY_FLATTEN
    while (s) {
      base_ptr[base++] = (u32)idx + __builtin_ctzll(s);
      s &= s - 1ULL;
    }
#elif defined(NO_PDEP_PLEASE)
    u32 cnt = __builtin_popcountll(s);
    u32 next_base = base + cnt;
    while (s) {
      CALL(SET_BITLOOPN, NO_PDEP_WIDTH)
      /*for(size_t i = 0; i < NO_PDEP_WIDTH; i++) {
        base_ptr[base+i] = (u32)idx + __builtin_ctzll(s);
        s = s & (s - 1);
      }*/
      base += NO_PDEP_WIDTH;
    }
    base = next_base;
#else
    u32 cnt = __builtin_popcountll(s);
    u32 next_base = base + cnt;
    while (s) {
      // spoil the suspense by reducing dependency chains; actually a win even
      // with cost of pdep
      u64 s3 = _pdep_u64(~0x7ULL, s);  // s3 will have bottom 3 1-bits unset
      u64 s5 = _pdep_u64(~0x1fULL, s); // s5 will have bottom 5 1-bits unset

      base_ptr[base + 0] = (u32)idx + __builtin_ctzll(s);
      u64 s1 = s & (s - 1ULL);
      base_ptr[base + 1] = (u32)idx + __builtin_ctzll(s1);
      u64 s2 = s1 & (s1 - 1ULL);
      base_ptr[base + 2] =
          (u32)idx + __builtin_ctzll(s2); // u64 s3 = s2 & (s2 - 1ULL);
      base_ptr[base + 3] = (u32)idx + __builtin_ctzll(s3);
      u64 s4 = s3 & (s3 - 1ULL);

      base_ptr[base + 4] =
          (u32)idx + __builtin_ctzll(s4); // u64 s5 = s4 & (s4 - 1ULL);
      base_ptr[base + 5] = (u32)idx + __builtin_ctzll(s5);
      u64 s6 = s5 & (s5 - 1ULL);
      s = s6;
      base += 6;
    }
    base = next_base;
#endif
  }
  pj.n_structural_indexes = base;
  if(len != base_ptr[pj.n_structural_indexes-1]) {
    printf("last structural should be pointing at the end of the string\n");
    return false;
  }
  base_ptr[pj.n_structural_indexes] = 0; // make it safe to dereference one beyond this array
  return true;
}
