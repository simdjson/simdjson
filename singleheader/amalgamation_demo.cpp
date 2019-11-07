/* auto-generated on Thu 07 Nov 2019 05:05:37 PM EST. Do not edit! */

#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  if(argc < 3) {
    std::cerr << "Please specify filenames " << std::endl;
  }
  const char * filename = argv[1];
  simdjson::padded_string p = simdjson::get_corpus(filename);
  simdjson::ParsedJson pj = simdjson::build_parsed_json(p); // do the parsing
  if( ! pj.is_valid() ) {
    std::cout << "build_parsed_json not valid" << std::endl;
  } else {
    std::cout << "build_parsed_json valid" << std::endl;
  }

  //JsonStream
  const char * filename2 = argv[2];
  simdjson::padded_string p2 = simdjson::get_corpus(filename2);
  simdjson::ParsedJson pj2;
  simdjson::JsonStream js{p2.data(), p2.size()};
  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;

  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
            parse_res = js.json_parse(pj2);
        }

  if( ! pj2.is_valid()) {
    std::cout << "JsonStream not valid" << std::endl;
  } else {
    std::cout << "JsonStream valid" << std::endl;
  }


  return EXIT_SUCCESS;
}

