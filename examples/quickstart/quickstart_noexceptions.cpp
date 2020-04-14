#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets;
  simdjson::error_code error;
  parser.load("twitter.json").tie(tweets,error);
  if(error) {
    std::cerr << "could not load document" << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::dom::element res;

  tweets["search_metadata"]["count"].tie(res,error);
  if(error) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
}

