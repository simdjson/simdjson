#include <iostream>
#include "simdjson.h"

int main() {
  simdjson::error_code error;
  double value; // variable where we store the value to be parsed
  simdjson::padded_string numberstring = "1.2"_padded; // our JSON input ("1.2")
  simdjson::dom::parser parser;
  parser.parse(numberstring).get(value,error);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << "I parsed " << value << " from " << numberstring.data() << std::endl;
}
