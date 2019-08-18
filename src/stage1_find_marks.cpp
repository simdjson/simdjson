#include "simdjson/portability.h"

namespace {
// for when clmul is unavailable
[[maybe_unused]] uint64_t portable_compute_quote_mask(uint64_t quote_bits) {
  uint64_t quote_mask = quote_bits ^ (quote_bits << 1);
  quote_mask = quote_mask ^ (quote_mask << 2);
  quote_mask = quote_mask ^ (quote_mask << 4);
  quote_mask = quote_mask ^ (quote_mask << 8);
  quote_mask = quote_mask ^ (quote_mask << 16);
  quote_mask = quote_mask ^ (quote_mask << 32);
  return quote_mask;
}
} // namespace

#include "arm64/stage1_find_marks.h"
#include "haswell/stage1_find_marks.h"
#include "westmere/stage1_find_marks.h"
