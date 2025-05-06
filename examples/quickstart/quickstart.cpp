#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load("twitter.json");
  std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;
}
