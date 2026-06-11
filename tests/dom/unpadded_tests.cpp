// Tests for dom::parser::parse_unpadded: parsing JSON in place from a buffer
// that has NO trailing SIMDJSON_PADDING.
//
// The key safety property is that parse_unpadded must never read past buf+len.
// To enforce that, every document here is parsed from a std::vector<char> sized
// to *exactly* len bytes: under AddressSanitizer (the ubuntu-build-address-
// sanitizer CI job) any read past the end lands in the redzone and fails the
// test. We also check that the result is identical to parsing the same bytes
// with the regular padded parser.

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "simdjson.h"
#include "test_macros.h"

namespace unpadded_tests {
using namespace simdjson;

// Serialize a DOM element to a string so two parses can be compared for equality.
simdjson_inline std::string to_text(dom::element e) {
  std::ostringstream out;
  out << e;
  return out.str();
}

// Parse `json` two ways and require success + identical output:
//  - padded:   the normal parser over a padded_string
//  - unpadded: parse_unpadded over a heap buffer of exactly json.size() bytes
// The exact-size buffer is what makes an over-read observable under ASAN.
bool check_matches(const std::string &json) {
  dom::parser padded_parser;
  dom::element padded_elt;
  auto padded_err = padded_parser.parse(simdjson::padded_string(json)).get(padded_elt);

  // Exact-size buffer: no readable byte after data()+size().
  std::vector<char> exact(json.begin(), json.end());
  dom::parser unpadded_parser;
  dom::element unpadded_elt;
  auto unpadded_err = unpadded_parser.parse_unpadded(exact.data(), exact.size()).get(unpadded_elt);

  // The two parsers must agree on whether the document is valid.
  if (padded_err != unpadded_err) {
    std::cerr << "mismatched error codes for <" << json << ">: padded="
              << padded_err << " unpadded=" << unpadded_err << std::endl;
    return false;
  }
  if (padded_err) { return true; } // both failed the same way: fine
  std::string a = to_text(padded_elt);
  std::string b = to_text(unpadded_elt);
  if (a != b) {
    std::cerr << "mismatched output: padded=<" << a << "> unpadded=<" << b << ">" << std::endl;
    return false;
  }
  return true;
}

// Sweep a string value of every length so its closing quote lands at every
// offset relative to the SIMDJSON_PADDING boundary and the scratch-copy path.
bool string_at_end_sweep() {
  TEST_START();
  for (size_t n = 0; n <= 3 * SIMDJSON_PADDING + 5; n++) {
    std::string json = "{\"k\":\"" + std::string(n, 'a') + "\"}";
    if (!check_matches(json)) { return false; }
    // Also a bare root string of length n.
    std::string root = "\"" + std::string(n, 'b') + "\"";
    if (!check_matches(root)) { return false; }
  }
  TEST_SUCCEED();
}

// Strings with escapes / unicode landing right against the end of the buffer.
bool escapes_at_end() {
  TEST_START();
  const std::vector<std::string> tails = {
    R"(\n)", R"(\t)", R"(\")", R"(\\)", R"(\/)", R"(\b)", R"(\f)", R"(\r)",
    R"(é)", R"(😀)" /* emoji surrogate pair */,
    R"(\uD800)" /* lone high surrogate (valid JSON string) */,
  };
  // Vary a leading filler so the escape sits at many offsets near the end.
  for (const auto &tail : tails) {
    for (size_t pad = 0; pad <= 2 * SIMDJSON_PADDING; pad++) {
      std::string json = "{\"k\":\"" + std::string(pad, 'x') + tail + "\"}";
      if (!check_matches(json)) { return false; }
      std::string root = "\"" + std::string(pad, 'y') + tail + "\"";
      if (!check_matches(root)) { return false; }
    }
  }
  TEST_SUCCEED();
}

// A grab-bag of small documents, including root primitives and the degenerate
// small-length cases (len < 3 exercises the BOM guard).
bool assorted_documents() {
  TEST_START();
  const std::vector<std::string> docs = {
    "{}", "[]", "true", "false", "null", "0", "-1", "123", "3.14", "1e10",
    "\"\"", "\"a\"", "\"hello world\"", "1", "[1]", "[1,2,3]",
    R"({"a":1,"b":2,"c":[true,false,null],"d":{"x":"y"}})",
    R"([{"k":"v"},{"k2":"v2"},123,4.5,"end"])",
    R"({"unicode":"éèê","emoji":"😀"})",
    R"(  {  "spaced" :  "value"  }  )",
    R"({"nested":{"deep":{"deeper":{"value":42}}}})",
    "\"\xEF\xBB\xBF\"", // a string whose content is a BOM (not a leading BOM)
  };
  for (const auto &d : docs) {
    if (!check_matches(d)) { return false; }
  }
  // Leading BOM + document.
  if (!check_matches(std::string("\xEF\xBB\xBF") + "[1,2,3]")) { return false; }
  TEST_SUCCEED();
}

// Larger, real-world files parsed unpadded from an exact-size buffer.
bool real_files() {
  TEST_START();
  for (const char *path : {TWITTER_JSON, CANADA_JSON, DEMO_JSON, REPEAT_JSON}) {
    simdjson::padded_string contents;
    ASSERT_SUCCESS( simdjson::padded_string::load(path).get(contents) );
    std::string json(contents.data(), contents.size());
    if (!check_matches(json)) {
      std::cerr << "mismatch on file " << path << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

// The std::string_view / const char* overloads of the public API.
bool api_overloads() {
  TEST_START();
  std::string json = R"({"hello":"world","n":42})";
  std::vector<char> exact(json.begin(), json.end());

  dom::parser p1;
  dom::element e1;
  ASSERT_SUCCESS( p1.parse_unpadded(std::string_view(exact.data(), exact.size())).get(e1) );
  ASSERT_EQUAL( int64_t(e1["n"]), 42 );
  ASSERT_EQUAL( std::string_view(e1["hello"]), "world" );

  dom::parser p2;
  dom::element e2;
  ASSERT_SUCCESS( p2.parse_unpadded(exact.data(), exact.size()).get(e2) ); // const char*, len
  ASSERT_EQUAL( int64_t(e2["n"]), 42 );

  // parse_into_document_unpadded with a caller-provided document.
  dom::parser p3;
  dom::document doc;
  dom::element e3;
  ASSERT_SUCCESS( p3.parse_into_document_unpadded(doc, reinterpret_cast<const uint8_t*>(exact.data()), exact.size()).get(e3) );
  ASSERT_EQUAL( std::string_view(e3["hello"]), "world" );

  // Reusing a parser for an unpadded then a padded parse must work (flag reset).
  dom::parser p4;
  dom::element tmp;
  ASSERT_SUCCESS( p4.parse_unpadded(exact.data(), exact.size()).get(tmp) );
  ASSERT_SUCCESS( p4.parse(simdjson::padded_string(json)).get(tmp) );
  ASSERT_EQUAL( int64_t(tmp["n"]), 42 );
  TEST_SUCCEED();
}

// Degenerate / invalid inputs: empty, whitespace-only, and truncated documents.
// parse_unpadded must agree with parse (same error) and, crucially, must not
// read past the (tiny) buffer -- caught here under ASAN.
bool degenerate_inputs() {
  TEST_START();
  const std::vector<std::string> docs = {
    "", " ", "   ", "\t\n", "{", "[", "\"", "\"abc", "[1,", "{\"a\":", "tru",
    "nul", "fals", "12.", "-", "[\"unterminated", std::string(1, '\0'),
  };
  for (const auto &d : docs) {
    if (!check_matches(d)) { return false; }
  }
  TEST_SUCCEED();
}

bool run() {
  return string_at_end_sweep() &&
         escapes_at_end() &&
         assorted_documents() &&
         degenerate_inputs() &&
         real_files() &&
         api_overloads();
}
} // namespace unpadded_tests

int main() {
  std::cout << "Running parse_unpadded tests." << std::endl;
  if (unpadded_tests::run()) {
    std::cout << "parse_unpadded tests are ok." << std::endl;
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
