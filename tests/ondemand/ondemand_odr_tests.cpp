// Regression test: simdjson.h must be includable from several translation units
// linked together. This target is built with SIMDJSON_CONSTEXPR_STRING defined
// to nothing, which mimics a standard library without constexpr std::string
// (e.g., libstdc++ 11): free functions defined in headers then lose the implicit
// 'inline' that 'constexpr' would give them, and the link fails with "multiple
// definition" errors unless they are explicitly marked 'inline'.
#include "simdjson.h"
#include <cstdlib>
#include <iostream>
#include <string>

std::string odr_other_describe();

int main() {
#if SIMDJSON_SUPPORTS_CONCEPTS
  using fields = simdjson::ondemand::key_selector<"name", "city">;
  std::string here = fields::describe();
  if (here.empty() || here != odr_other_describe()) {
    std::cerr << "key_selector::describe() mismatch across translation units" << std::endl;
    return EXIT_FAILURE;
  }
#endif
  std::cout << "ODR test passed" << std::endl;
  return EXIT_SUCCESS;
}
