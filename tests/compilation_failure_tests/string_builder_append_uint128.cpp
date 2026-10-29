// string_builder::append() writes integers through a 64-bit path, so a
// 128-bit integer must be rejected at compile time instead of being truncated
// to 64 bits or converted to char.

#include "simdjson.h"

int main() {
  simdjson::builder::string_builder sb;
#if COMPILATION_TEST_USE_FAILING_CODE
  __uint128_t v = static_cast<__uint128_t>(1) << 64;
#else
  int64_t v = 5;
#endif
  sb.append(v);
  return 0;
}
