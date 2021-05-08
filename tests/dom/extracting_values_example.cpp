#include <iostream>
#include "simdjson.h"

int main() {
  simdjson::error_code error;
  simdjson::padded_string numberstring = "1.2"_padded; // our JSON input ("1.2")
  simdjson::dom::parser parser;
  double value; // variable where we store the value to be parsed
  error = parser.parse(numberstring).get(value);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << "I parsed " << value << " from " << numberstring.data() << std::endl;
}
