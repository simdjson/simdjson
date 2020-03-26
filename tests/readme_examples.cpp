#include <iostream>
#include "simdjson.h"
using namespace std;
using namespace simdjson;

void parser_parse_error_code() {
  cout << __func__ << endl;

  // Allocate a parser big enough for all files
  document::parser parser;

  // Read files with the parser, one by one
  for (padded_string json : { string("[1, 2, 3]"), string("true"), string("[ true, false ]") }) {
    cout << "Parsing " << json.data() << " ..." << endl;
    auto [doc, error] = parser.parse(json);
    if (error) { cerr << "Error: " << error << endl; exit(1); }
    cout << doc << endl;
  }
}

void parser_parse_many_error_code() {
  cout << __func__ << endl;

  // Read files with the parser
  auto json = "[1, 2, 3] true [ true, false ]"_padded;
  cout << "Parsing " << json.data() << " ..." << endl;
  document::parser parser;
  for (auto [doc, error] : parser.parse_many(json)) {
    if (error) { cerr << "Error: " << error << endl; exit(1); }
    cout << doc << endl;
  }
}

void parser_parse_max_capacity() {
  cout << __func__ << endl;

  int argc = 2;
  padded_string argv[] { string("[1,2,3]"), string("true") };
  document::parser parser(1024*1024); // Set max capacity to 1MB
  for (int i=0;i<argc;i++) {
    auto [doc, error] = parser.parse(argv[i]);
    if (error == CAPACITY) { cerr << "JSON files larger than 1MB are not supported!" << endl; exit(1); }
    if (error) { cerr << error << endl; exit(1); }
    cout << doc << endl;
  }
}

void parser_parse_fixed_capacity() {
  cout << __func__ << endl;

  int argc = 2;
  padded_string argv[] { string("[1,2,3]"), string("true") };
  document::parser parser(0); // This parser is not allowed to auto-allocate
  auto alloc_error = parser.set_capacity(1024*1024);
  if (alloc_error) { cerr << alloc_error << endl; exit(1); }; // Set initial capacity to 1MB
  for (int i=0;i<argc;i++) {
    auto [doc, error] = parser.parse(argv[i]);
    if (error == CAPACITY) { cerr << "JSON files larger than 1MB are not supported!" << endl; exit(1); }
    if (error) { cerr << error << endl; exit(1); }
    cout << doc << endl;
  }
}

#if SIMDJSON_EXCEPTIONS

void parser_parse_padded_string() {
  cout << __func__ << endl;

  auto json = "[ 1, 2, 3 ]"_padded;
  document::parser parser;
  cout << parser.parse(json) << endl;
}

void parser_parse_get_corpus() {
  cout << __func__ << endl;

  auto json = get_corpus("jsonexamples/small/demo.json");
  document::parser parser;
  cout << parser.parse(json) << endl;
}

void parser_parse_exception() {
  cout << __func__ << endl;

  // Allocate a parser big enough for all files
  document::parser parser;

  // Read files with the parser, one by one
  for (padded_string json : { string("[1, 2, 3]"), string("true"), string("[ true, false ]") }) {
    cout << "Parsing " << json.data() << " ..." << endl;
    cout << parser.parse(json) << endl;
  }
}

void parser_parse_many_exception() {
  cout << __func__ << endl;

  // Read files with the parser
  auto json = "[1, 2, 3] true [ true, false ]"_padded;
  cout << "Parsing " << json.data() << " ..." << endl;
  document::parser parser;
  for (const document::element doc : parser.parse_many(json)) {
    cout << doc << endl;
  }
}

#endif // SIMDJSON_EXCEPTIONS

int main() {
  cout << "Running examples." << endl;
  parser_parse_error_code();
  parser_parse_many_error_code();
  parser_parse_max_capacity();
  parser_parse_fixed_capacity();
#if SIMDJSON_EXCEPTIONS
  parser_parse_exception();
  parser_parse_many_exception();
  parser_parse_padded_string();
  parser_parse_get_corpus();
#endif // SIMDJSON_EXCEPTIONS
  cout << "Ran to completion!" << endl;
  return 0;
}
