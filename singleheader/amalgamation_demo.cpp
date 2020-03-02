/* auto-generated on Mon Mar  2 14:10:52 PST 2020. Do not edit! */

#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cerr << "Please specify at least one file name. " << std::endl;
  }
  const char * filename = argv[1];
  simdjson::padded_string p = simdjson::get_corpus(filename);
  auto [doc, error] = simdjson::document::parse(p); // do the parsing
  if (error) {
    std::cout << "document::parse failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error_message(error) << std::endl;
  } else {
    std::cout << "document::parse valid" << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  //JsonStream
  const char * filename2 = argv[2];
  simdjson::padded_string p2 = simdjson::get_corpus(filename2);
  simdjson::document::parser parser;
  simdjson::JsonStream js{p2};
  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;

  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
            parse_res = js.json_parse(parser);
  }

  if( ! parser.is_valid()) {
    std::cout << "JsonStream not valid" << std::endl;
  } else {
    std::cout << "JsonStream valid" << std::endl;
  }


  return EXIT_SUCCESS;
}

