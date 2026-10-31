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

// "prefix" + std::string(...) inserts the literal into a temporary and trips
// GCC bug 105329 (-Wrestrict) at -O3 on the Ubuntu 22.04 GCC 12 job.
std::string make_json(const char *prefix, size_t count, char fill, const char *suffix = "") {
  std::string json(prefix);
  json.append(count, fill);
  json += suffix;
  return json;
}

// Valid documents of every length, ending in each kind of value.
bool valid_documents() {
  TEST_START();
  for (size_t len = 1; len <= MAX_LEN; len++) {
    if (!check(std::string(len, '7'))) { return false; }
    if (len >= 2 && !check(make_json("\"", len - 2, 'a', "\""))) { return false; }
    if (len >= 2 && !check(make_json("[", len - 2, ' ', "]"))) { return false; }
    if (len >= 4 && !check(make_json("[\"", len - 4, 'a', "\"]"))) { return false; }
    if (len >= 8 && !check(make_json("{\"k\":\"", len - 8, 'a', "\"}"))) { return false; }
    if (len >= 6 && !check(make_json("[", len - 6, ' ', "true]"))) { return false; }
    if (len >= 4 && !check(make_json("null", len - 4, ' '))) { return false; }
  }
  TEST_SUCCEED();
}

// Invalid documents of every length: these must still be rejected, with the
// same error, whatever follows them.
bool invalid_documents() {
  TEST_START();
  for (size_t len = 1; len <= MAX_LEN; len++) {
    if (!check(make_json("\"", len - 1, 'a'))) { return false; }                    // unclosed string
    if (len >= 2 && !check(make_json("\"", len - 2, 'a', "\\"))) { return false; }  // unfinished escape
    if (len >= 2 && !check(make_json("[", len - 2, ' ', ","))) { return false; }    // unclosed array
    if (len >= 3 && !check(make_json("\"", len - 3, 'a', "\xe2\x82"))) { return false; } // truncated UTF-8
    if (len >= 2 && !check(make_json("\"", len - 2, 'a', "\xc3"))) { return false; }
    if (!check(std::string(len, '{'))) { return false; }
  }
  TEST_SUCCEED();
}

// Parse json followed by SIMDJSON_PADDING bytes of white space in an exactly
// sized buffer, with no capacity past the end: has_sufficient_padding() accepts
// such a view, and pad() builds one when the string already ends with the white
// space, so the parser must not read past it. Requires the same error code and
// output as over a padded_string from every entry point that takes a
// padded_string_view.
bool check_trailing_whitespace(const std::string &json, char space) {
  padded_string clean(json);
  dom::parser dom_parser;
  ondemand::parser ondemand_parser;
  error_code dom_expected_error, ondemand_expected_error;
  const std::string dom_expected = dom_text(dom_parser, clean.data(), clean.size(), dom_expected_error);
  const std::string ondemand_expected = ondemand_text(ondemand_parser, clean.data(), clean.size(),
                                                      clean.size() + SIMDJSON_PADDING, ondemand_expected_error);

  std::vector<char> buf(json.begin(), json.end());
  buf.resize(json.size() + SIMDJSON_PADDING, space);
  const padded_string_view view(buf.data(), buf.size(), buf.size());
  padded_string_view trimmed(view);
  if (!view.has_sufficient_padding() || !trimmed.use_trailing_whitespace_as_padding() ||
      trimmed.length() != json.size() || trimmed.padding() != SIMDJSON_PADDING) {
    std::cerr << "trailing white space not taken as padding for <" << printable(json) << ">" << std::endl;
    return false;
  }

  error_code error;
  std::string result = ondemand_text(ondemand_parser, buf.data(), buf.size(), buf.size(), error);
  if (error != ondemand_expected_error || result != ondemand_expected) {
    std::cerr << "On-Demand iterate over trailing white space changed the result for <" << printable(json) << ">: "
              << error << " <" << result << ">" << std::endl;
    return false;
  }
  // pad() appends nothing when the white space is already there.
  std::string str(buf.begin(), buf.end());
  str.shrink_to_fit();
  const padded_string_view padded = pad(str);
  result = ondemand_text(ondemand_parser, padded.data(), padded.length(), padded.capacity(), error);
  if (error != ondemand_expected_error || result != ondemand_expected) {
    std::cerr << "On-Demand iterate over pad() changed the result for <" << printable(json) << ">: "
              << error << " <" << result << ">" << std::endl;
    return false;
  }
  dom::element elt;
  std::ostringstream dom_result;
  error = dom_parser.parse(view).get(elt);
  if (!error) { dom_result << elt; }
  if (error != dom_expected_error || dom_result.str() != dom_expected) {
    std::cerr << "DOM parse over trailing white space changed the result for <" << printable(json) << ">: "
              << error << " <" << dom_result.str() << ">" << std::endl;
    return false;
  }

  // The streams hold a single document, which must give the same result.
  size_t count = 0;
  ondemand::document_stream ondemand_stream;
  error = ondemand_parser.iterate_many(view).get(ondemand_stream);
  if (!error) {
    for (auto doc : ondemand_stream) {
      std::string_view raw;
      error = to_json_string(doc).get(raw);
      result = error ? "" : std::string(raw);
      count++;
    }
  }
  if (count != 1 || error != ondemand_expected_error || result != ondemand_expected) {
    std::cerr << "On-Demand iterate_many over trailing white space changed the result for <" << printable(json) << ">: "
              << error << " <" << result << ">, " << count << " documents" << std::endl;
    return false;
  }
  count = 0;
  dom::document_stream dom_stream;
  error = dom_parser.parse_many(view).get(dom_stream);
  if (!error) {
    for (auto doc : dom_stream) {
      dom_result.str("");
      error = doc.get(elt);
      if (!error) { dom_result << elt; }
      count++;
    }
  }
  if (count != 1 || error != dom_expected_error || dom_result.str() != dom_expected) {
    std::cerr << "DOM parse_many over trailing white space changed the result for <" << printable(json) << ">: "
              << error << " <" << dom_result.str() << ">, " << count << " documents" << std::endl;
    return false;
  }
  return true;
}

// Valid documents of every length followed by white space standing in for the
// padding. Under a sanitizer, a read past the white space is caught.
bool trailing_whitespace_as_padding() {
  TEST_START();
  for (char space : {' ', '\n'}) {
    for (size_t len = 1; len <= MAX_LEN; len++) {
      if (!check_trailing_whitespace(std::string(len, '7'), space)) { return false; }
      if (len >= 2 && !check_trailing_whitespace(make_json("\"", len - 2, 'a', "\""), space)) { return false; }
      if (len >= 2 && !check_trailing_whitespace(make_json("[", len - 2, ' ', "]"), space)) { return false; }
      if (len >= 8 && !check_trailing_whitespace(make_json("{\"k\":\"", len - 8, 'a', "\"}"), space)) { return false; }
      if (len >= 4 && !check_trailing_whitespace(make_json("null", len - 4, ' '), space)) { return false; }
    }
  }
  TEST_SUCCEED();
}

bool run() {
  return valid_documents() &&
         invalid_documents() &&
         trailing_whitespace_as_padding();
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
