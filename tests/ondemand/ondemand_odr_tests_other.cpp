// Second translation unit for ondemand_odr_tests.cpp.
#include "simdjson.h"
#include <string>

std::string odr_other_describe() {
#if SIMDJSON_SUPPORTS_CONCEPTS
  using fields = simdjson::ondemand::key_selector<"name", "city">;
  return fields::describe();
#else
  return std::string();
#endif
}
