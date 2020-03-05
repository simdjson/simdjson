#include <iostream>
#include "simdjson.h"
using namespace std;
using namespace simdjson;

void document_parse_error_code() {
  cout << __func__ << endl;

  string json("[ 1, 2, 3 ]");
  auto [doc, error] = document::parse(json);
  if (error) { cerr << "Error: " << error << endl; exit(1); }
  if (!doc.print_json(cout)) { exit(1); }
  cout << endl;
}

void document_parse_exception() {
  cout << __func__ << endl;

  string json("[ 1, 2, 3 ]");
  document doc = document::parse(json);
  if (!doc.print_json(cout)) { exit(1); }
  cout << endl;
}

void document_parse_padded_string() {
  cout << __func__ << endl;

  padded_string json(string("[ 1, 2, 3 ]"));
  document doc = document::parse(json);
  if (!doc.print_json(cout)) { exit(1); }
  cout << endl;
}

void document_parse_get_corpus() {
  cout << __func__ << endl;

  padded_string json(get_corpus("jsonexamples/small/demo.json"));
  document doc = document::parse(json);
  if (!doc.print_json(cout)) { exit(1); }
  cout << endl;
}

void parser_parse() {
  cout << __func__ << endl;

  // Allocate a parser big enough for all files
  document::parser parser;
  if (!parser.allocate_capacity(1024*1024)) { exit(1); }

  // Read files with the parser, one by one
  for (padded_string json : { string("[1, 2, 3]"), string("true"), string("[ true, false ]") }) {
    cout << "Parsing " << json.data() << " ..." << endl;
    auto [doc, error] = parser.parse(json);
    if (error) { cerr << "Error: " << error << endl; exit(1); }
    if (!doc.print_json(cout)) { exit(1); }
    cout << endl;
  }
}

void parser_parse_many_error_code() {
  cout << __func__ << endl;

  // Read files with the parser
  padded_string json = string("[1, 2, 3] true [ true, false ]");
  cout << "Parsing " << json.data() << " ..." << endl;
  document::parser parser;
  for (auto [doc, error] : parser.parse_many(json)) {
    if (error) { cerr << "Error: " << error << endl; exit(1); }
    if (!doc.print_json(cout)) { exit(1); }
    cout << endl;
  }
}

void parser_parse_many_exception() {
  cout << __func__ << endl;

  // Read files with the parser
  padded_string json = string("[1, 2, 3] true [ true, false ]");
  cout << "Parsing " << json.data() << " ..." << endl;
  document::parser parser;
  for (const document &doc : parser.parse_many(json)) {
    if (!doc.print_json(cout)) { exit(1); }
    cout << endl;
  }
}

int main() {
  cout << "Running examples." << endl;
  document_parse_error_code();
  document_parse_exception();
  document_parse_padded_string();
  document_parse_get_corpus();
  parser_parse();
  parser_parse_many_error_code();
  parser_parse_many_exception();
  cout << "Ran to completion!" << endl;
  return 0;
}
