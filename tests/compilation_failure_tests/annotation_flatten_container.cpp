// When deserializing, a member annotated with flatten must be a structure read
// member by member, not a container. The error must be reported at compile
// time, with or without exceptions.

#include "simdjson.h"

#include <vector>

struct inner {
  int a;
};

struct outer {
#if COMPILATION_TEST_USE_FAILING_CODE
  [[= simdjson::flatten]] std::vector<int> in{};
#else
  [[= simdjson::flatten]] inner in{};
#endif
};

int main() {
  simdjson::padded_string json(std::string("{\"a\":1}"));
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  if (parser.iterate(json).get(doc)) { return 1; }
  outer o{};
  return doc.get(o) == simdjson::SUCCESS ? 0 : 1;
}
