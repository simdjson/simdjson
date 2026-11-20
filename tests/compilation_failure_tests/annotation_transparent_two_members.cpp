// A structure annotated with transparent must have exactly one data member
// that is not skipped. The error must be reported at compile time, with or
// without exceptions.

#include "simdjson.h"

struct [[= simdjson::transparent]] user_id {
  int64_t value;
#if COMPILATION_TEST_USE_FAILING_CODE
  int64_t other;
#else
  [[= simdjson::skip]] int64_t other;
#endif
};

int main() {
  user_id id{1, 2};
  std::string json;
  return simdjson::to_json(id).get(json) == simdjson::SUCCESS ? 0 : 1;
}
