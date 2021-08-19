#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets;
  auto error = parser.load("twitter.json").get(tweets);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  simdjson::dom::element res;

  if ((error = tweets["search_metadata"]["count"].get(res))) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
}

