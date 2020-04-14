#include <iostream>
#include "simdjson.h"

int main() {
  auto numberstring = "1.2"_padded;
  simdjson::dom::parser parser;
  simdjson::error_code error;
  double value;
  parser.parse(numberstring).get<double>().tie(value,error);
  if(error) {
    std::cerr << "error parsing JSON" << std::endl;
    return EXIT_FAILURE;
  }
  if(value != 1.2) {
    std::cerr << "bad value" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "I parsed " << value << " from " << numberstring.data() << std::endl;
  return EXIT_SUCCESS;
}
