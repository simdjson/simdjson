// The static-reflection serializer writes integers through a 64-bit path, so
// a struct with a 128-bit integer member must be rejected at compile time
// instead of being serialized with a truncated value.

#include "simdjson.h"

struct with_wide_member {
  int a;
#if COMPILATION_TEST_USE_FAILING_CODE
  __int128_t big;
#else
  int64_t big;
#endif
};

int main() {
  with_wide_member s{1, 5};
  std::string json;
  return simdjson::to_json(s).get(json) == simdjson::SUCCESS ? 0 : 1;
}
