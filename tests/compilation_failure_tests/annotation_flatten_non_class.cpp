// A member annotated with flatten must be a structure. The error must be
// reported at compile time, with or without exceptions.

#include "simdjson.h"

struct inner {
  int a;
};

struct outer {
#if COMPILATION_TEST_USE_FAILING_CODE
  [[= simdjson::flatten]] int in{2};
#else
  [[= simdjson::flatten]] inner in{2};
#endif
};

int main() {
  outer o{};
  std::string json;
  return simdjson::to_json(o).get(json) == simdjson::SUCCESS ? 0 : 1;
}
