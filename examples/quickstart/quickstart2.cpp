#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load("twitter.json");
  std::cout << "ID: " << tweets["statuses"].at(0)["id"] << std::endl;
}
