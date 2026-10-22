// Tests that the bytes in SIMDJSON_PADDING never change the result of a parse.
//
// The padding is only guaranteed to be readable, not to hold any particular
// bytes. A caller who parses in place (parse(buf, len, false), or a
// padded_string_view over their own buffer) may leave anything there, and so
// does dom::parser when it copies a shorter document into the buffer it kept
// from an earlier one: only len bytes are copied. padded_string zeroes its
// padding, so most tests only see zeros. Stage 1 reads the last, partial block
// in place and must replace every byte past the end with whitespace; a stray
// quote, backslash, bracket or non-ASCII byte there would otherwise change the
// string state, add a structural character or fail UTF-8 validation.
//
// Every document is parsed with each padding fill below and must give the same
// error code and output as the same document over a padded_string. Lengths run
// from 1 to 300 bytes, so the last block holds every possible number of bytes
// for both 64- and 128-byte steps.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "simdjson.h"
#include "test_macros.h"

namespace padding_contents_tests {
using namespace simdjson;

const size_t MAX_LEN = 300;

const std::vector<std::string> fills = {
  "\"", "\\", "{", "]", ",", "1", "a", std::string(1, '\0'),
  "\xff", "\xc3", "\xe2\x82", "\"\\", "\"{\"",
};

// Bytes outside printable ASCII as \xNN, so a failure message stays readable.
std::string printable(const std::string &bytes) {
  static const char hex[] = "0123456789abcdef";
  std::string out;
  for (unsigned char c : bytes) {
    if (c >= 0x20 && c < 0x7f) { out += char(c); continue; }
    out += "\\x";
    out += hex[c >> 4];
    out += hex[c & 0xf];
  }
  return out;
}

std::string dom_text(dom::parser &parser, const char *buf, size_t len, error_code &error) {
  dom::element elt;
  error = parser.parse(buf, len, false).get(elt);
  if (error) { return ""; }
  std::ostringstream out;
  out << elt;
  return out.str();
}

std::string ondemand_text(ondemand::parser &parser, const char *buf, size_t len, size_t capacity, error_code &error) {
  ondemand::document doc;
  error = parser.iterate(padded_string_view(buf, len, capacity)).get(doc);
  if (error) { return ""; }
  std::string_view raw;
  error = to_json_string(doc).get(raw);
  if (error) { return ""; }
  return std::string(raw);
}

// Parse json over a padded_string, then in place with each fill repeated over
// the padding, and require the same error code and output every time.
bool check(const std::string &json) {
  padded_string clean(json);
  dom::parser dom_parser;
  ondemand::parser ondemand_parser;
  error_code dom_expected_error, ondemand_expected_error;
  const std::string dom_expected = dom_text(dom_parser, clean.data(), clean.size(), dom_expected_error);
  const std::string ondemand_expected = ondemand_text(ondemand_parser, clean.data(), clean.size(),
                                                      clean.size() + SIMDJSON_PADDING, ondemand_expected_error);
  for (const std::string &fill : fills) {
    std::vector<char> buf(json.begin(), json.end());
    buf.resize(json.size() + SIMDJSON_PADDING);
    for (size_t i = json.size(); i < buf.size(); i++) { buf[i] = fill[(i - json.size()) % fill.size()]; }

    error_code dom_error, ondemand_error;
    const std::string dom_result = dom_text(dom_parser, buf.data(), json.size(), dom_error);
    const std::string ondemand_result = ondemand_text(ondemand_parser, buf.data(), json.size(), buf.size(), ondemand_error);
    if (dom_error != dom_expected_error || dom_result != dom_expected) {
      std::cerr << "DOM: padding of '" << printable(fill) << "' changed the result for <" << printable(json) << ">: "
                << dom_error << " <" << dom_result << ">, expected "
                << dom_expected_error << " <" << dom_expected << ">" << std::endl;
      return false;
    }
    if (ondemand_error != ondemand_expected_error || ondemand_result != ondemand_expected) {
      std::cerr << "On-Demand: padding of '" << printable(fill) << "' changed the result for <" << printable(json) << ">: "
                << ondemand_error << " <" << ondemand_result << ">, expected "
                << ondemand_expected_error << " <" << ondemand_expected << ">" << std::endl;
      return false;
    }
  }
  return true;
}

// Valid documents of every length, ending in each kind of value.
bool valid_documents() {
  TEST_START();
  for (size_t len = 1; len <= MAX_LEN; len++) {
    if (!check(std::string(len, '7'))) { return false; }
    if (len >= 2 && !check("\"" + std::string(len - 2, 'a') + "\"")) { return false; }
    if (len >= 2 && !check("[" + std::string(len - 2, ' ') + "]")) { return false; }
    if (len >= 4 && !check("[\"" + std::string(len - 4, 'a') + "\"]")) { return false; }
    if (len >= 8 && !check("{\"k\":\"" + std::string(len - 8, 'a') + "\"}")) { return false; }
    if (len >= 6 && !check("[" + std::string(len - 6, ' ') + "true]")) { return false; }
    if (len >= 4 && !check("null" + std::string(len - 4, ' '))) { return false; }
  }
  TEST_SUCCEED();
}

// Invalid documents of every length: these must still be rejected, with the
// same error, whatever follows them.
bool invalid_documents() {
  TEST_START();
  for (size_t len = 1; len <= MAX_LEN; len++) {
    if (!check("\"" + std::string(len - 1, 'a'))) { return false; }               // unclosed string
    if (len >= 2 && !check("\"" + std::string(len - 2, 'a') + "\\")) { return false; }  // unfinished escape
    if (len >= 2 && !check("[" + std::string(len - 2, ' ') + ",")) { return false; }   // unclosed array
    if (len >= 3 && !check("\"" + std::string(len - 3, 'a') + "\xe2\x82")) { return false; } // truncated UTF-8
    if (len >= 2 && !check("\"" + std::string(len - 2, 'a') + "\xc3")) { return false; }
    if (!check(std::string(len, '{'))) { return false; }
  }
  TEST_SUCCEED();
}

bool run() {
  return valid_documents() &&
         invalid_documents();
}
} // namespace padding_contents_tests

int main() {
  std::cout << "Running padding contents tests against "
            << simdjson::get_active_implementation()->name() << "." << std::endl;
  if (padding_contents_tests::run()) {
    std::cout << "Padding contents tests are ok." << std::endl;
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
