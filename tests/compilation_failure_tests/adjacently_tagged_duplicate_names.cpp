// Verifies that simdjson::adjacently_tagged rejects, at compile time, a variant
// whose alternatives have the same name: when deserializing, the name would not
// tell which alternative to build.
//
// The CMake harness compiles this twice: with COMPILATION_TEST_USE_FAILING_CODE
// unset (the alternatives have distinct names thanks to a rename annotation,
// which must compile) and set to 1 (two types named Item in different
// namespaces, which must fail to compile with the documented message).
#include "simdjson.h"
#include <string>
#include <variant>

#if SIMDJSON_STATIC_REFLECTION

namespace first {
struct Item {
  int id = 0;
};
} // namespace first

namespace second {
#if COMPILATION_TEST_USE_FAILING_CODE
struct Item {
#else
struct [[= simdjson::rename<"second_item">]] Item {
#endif
  int id = 0;
};
} // namespace second

struct Holder {
  [[= simdjson::with<simdjson::adjacently_tagged<>>]] std::variant<first::Item, second::Item> item;
};

int main() {
  std::string json;
  return simdjson::to_json(Holder{}).get(json) == simdjson::SUCCESS ? 0 : 1;
}

#else

// The annotations require static reflection; without it there is nothing to check.
int main() { return 0; }

#endif
