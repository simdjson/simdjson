// This file is not part of our main, regular tests.
#include "../singleheader/simdjson.h"
#include <iostream>
#include <type_traits>

int main() {
  if (!std::is_trivially_copyable<simdjson::dom::element>::value) {
    std::cerr << "simdjson::dom::element must be trivially copyable"
              << std::endl;
    return EXIT_FAILURE;
  }
  if (!std::is_trivially_copyable<simdjson::dom::array>::value) {
    std::cerr << "simdjson::dom::array must be trivially copyable" << std::endl;
    return EXIT_FAILURE;
  }
  if (!std::is_trivially_copyable<simdjson::dom::object>::value) {
    std::cerr << "simdjson::dom::object must be trivially copyable"
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
