// string_builder::append() writes integers through a 64-bit path, so an array
// of 128-bit integers must be rejected at compile time instead of being
// truncated.

#include "simdjson.h"
#include <vector>

int main() {
  simdjson::builder::string_builder sb;
#if COMPILATION_TEST_USE_FAILING_CODE
  std::vector<__int128_t> v{-(static_cast<__int128_t>(1) << 64) - 5};
#else
  std::vector<int64_t> v{5};
#endif
  sb.append(v);
  return 0;
}
