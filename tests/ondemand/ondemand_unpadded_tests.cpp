// Tests for ondemand::parser::iterate_unpadded: parsing JSON in place from a
// buffer that has NO trailing SIMDJSON_PADDING (the On Demand equivalent of the
// DOM dom::parser::parse_unpadded, see tests/dom/unpadded_tests.cpp).
//
// The key safety property is that iterate_unpadded must never read past buf+len.
// To enforce that, every document here is parsed from a std::vector<char> sized
// to *exactly* len bytes: under AddressSanitizer (the ubuntu-build-address-
// sanitizer CI job) any read past the end lands in the redzone and fails the
// test. We also check that the lazily-parsed result is identical to parsing the
// same bytes with the regular padded iterate().

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "simdjson.h"
#include "test_ondemand.h"

namespace ondemand_unpadded_tests {
using namespace simdjson;

// Recursively walk an On Demand node (document or value) and serialize it to a
// canonical text form, so two parses can be compared for equality. Returns the
// first error encountered (so a malformed document surfaces the same error on
// both the padded and the unpadded side). Numbers are emitted via get_double so
// the 8-byte float fast-path is exercised at every offset; the integer paths are
// covered by the explicit near-end cases below.
// node may be an ondemand::document (top level) or a simdjson_result<value>
// (recursion): both forward type()/get_array()/get_object()/get_double()/etc.
template <typename node_type>
error_code serialize(node_type &&node, std::ostream &out) {
  ondemand::json_type t;
  error_code e = node.type().get(t);
  if (e) { return e; }
  switch (t) {
    case ondemand::json_type::array: {
      out << '[';
      bool first = true;
      ondemand::array arr;
      if ((e = node.get_array().get(arr))) { return e; }
      for (auto child : arr) {
        if (!first) { out << ','; }
        first = false;
        if ((e = serialize(child, out))) { return e; }
      }
      out << ']';
      return SUCCESS;
    }
    case ondemand::json_type::object: {
      out << '{';
      bool first = true;
      ondemand::object obj;
      if ((e = node.get_object().get(obj))) { return e; }
      for (auto field : obj) {
        std::string_view key;
        if ((e = field.unescaped_key().get(key))) { return e; }
        if (!first) { out << ','; }
        first = false;
        out << '"' << key << "\":";
        if ((e = serialize(field.value(), out))) { return e; }
      }
      out << '}';
      return SUCCESS;
    }
    case ondemand::json_type::number: {
      double d;
      if ((e = node.get_double().get(d))) { return e; }
      out << std::setprecision(17) << d;
      return SUCCESS;
    }
    case ondemand::json_type::string: {
      std::string_view s;
      if ((e = node.get_string().get(s))) { return e; }
      out << '"' << s << '"';
      return SUCCESS;
    }
    case ondemand::json_type::boolean: {
      bool b;
      if ((e = node.get_bool().get(b))) { return e; }
      out << (b ? 'T' : 'F');
      return SUCCESS;
    }
    case ondemand::json_type::null: {
      bool is_null_value;
      if ((e = node.is_null().get(is_null_value))) { return e; }
      out << (is_null_value ? "N" : "?");
      return SUCCESS;
    }
    default:
      return SUCCESS;
  }
}

// Iterate `json` and return (error, serialized-output). The document, its source
// buffer and the parser are all kept alive for the duration of the walk.
template <typename iterate_fn>
std::pair<error_code, std::string> walk(iterate_fn iterate) {
  ondemand::parser parser;
  ondemand::document doc;
  error_code err = iterate(parser).get(doc);
  if (err) { return {err, std::string()}; }
  std::ostringstream out;
  err = serialize(doc, out);
  return {err, out.str()};
}

// Parse `json` two ways and require they agree:
//  - padded:   the normal iterate() over a padded_string
//  - unpadded: iterate_unpadded over a heap buffer of exactly json.size() bytes
// The exact-size buffer is what makes an over-read observable under ASAN.
bool check_matches(const std::string &json) {
  simdjson::padded_string padded(json);
  auto a = walk([&](ondemand::parser &p) { return p.iterate(padded); });

  std::vector<char> exact(json.begin(), json.end()); // no readable byte after end
  auto b = walk([&](ondemand::parser &p) { return p.iterate_unpadded(exact.data(), exact.size()); });

  if (bool(a.first) != bool(b.first)) {
    std::cerr << "mismatched error for <" << json << ">: padded=" << a.first
              << " unpadded=" << b.first << std::endl;
    return false;
  }
  if (a.first) { return true; } // both failed (possibly at different points): fine
  if (a.second != b.second) {
    std::cerr << "mismatched output for <" << json << ">: padded=<" << a.second
              << "> unpadded=<" << b.second << ">" << std::endl;
    return false;
  }
  return true;
}

// Root and non-root numbers in every shape.
bool numbers() {
  TEST_START();
  const std::vector<std::string> nums = {
    "0", "-0", "1", "-1", "123", "-123", "3.14", "-3.14", "1e10", "1E10",
    "1e-10", "1.5e+3", "0.0", "0.1", "3.141592653589793",
    "123456789012345678", "9223372036854775807", "-9223372036854775808",
    "18446744073709551615", "1.7976931348623157e308", "100000000000000000000",
  };
  for (const auto &n : nums) {
    if (!check_matches(n)) { return false; }                    // root (copy path)
    if (!check_matches("[" + n + "]")) { return false; }        // last array element
    if (!check_matches("{\"k\":" + n + "}")) { return false; }  // last object value
    if (!check_matches("[" + n + ",1]")) { return false; }      // followed by a comma
    if (!check_matches("[1," + n + "]")) { return false; }
  }
  TEST_SUCCEED();
}

// Sweep a float so its (8-byte-read) fractional digits land at every offset
// relative to the end of the buffer, including long fractional parts preceded by
// filler (which a start-position threshold would miss).
bool number_at_end_sweep() {
  TEST_START();
  for (size_t frac = 1; frac <= 3 * SIMDJSON_PADDING + 5; frac++) {
    std::string num = "0." + std::string(frac, '9');
    if (!check_matches("[" + num + "]")) { return false; }           // float ends the array
    if (!check_matches("{\"k\":" + num + "}")) { return false; }     // float ends the object
    // Long float preceded by filler so it starts far from the end but its digits
    // still reach the final bytes (exercises the next-structural-based trigger).
    if (!check_matches("[123456789," + num + "]")) { return false; }
    // Long integer at the end too.
    std::string bignum = std::string(frac, '7');
    if (!check_matches("[1," + bignum + "]")) { return false; }
  }
  TEST_SUCCEED();
}

// Malformed / truncated atom-like tokens at the buffer end. The 'n'/'t'/'f'
// dispatch validates a value against the full null/true/false atom, reading a
// fixed number of bytes; a shorter or wrong token at the end of an unpadded
// buffer must not read past it (caught here under ASAN).
bool malformed_atoms_at_end() {
  TEST_START();
  const std::vector<std::string> toks = {
    "n", "nu", "nul", "nulx", "nall", "t", "tr", "tru", "trux", "ture",
    "f", "fa", "fal", "fals", "falx", "fale", "true", "false", "null",
  };
  for (const auto &t : toks) {
    if (!check_matches("[" + t + "]")) { return false; }            // ends the array
    if (!check_matches("{\"k\":" + t + "}")) { return false; }      // ends the object
    if (!check_matches("[" + t + ",1]")) { return false; }
    if (!check_matches("[" + std::string(SIMDJSON_PADDING, ' ') + t + "]")) { return false; }
  }
  TEST_SUCCEED();
}

// --- Deterministic random JSON generator (for fuzz-style comparison) ----------
// xorshift64 so any failure is reproducible from its seed.
struct rng {
  uint64_t s;
  explicit rng(uint64_t seed) : s(seed ? seed : 0x9e3779b97f4a7c15ull) {}
  uint64_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
  uint32_t below(uint32_t n) { return static_cast<uint32_t>(next() % n); }
};

void gen_string(rng &r, std::string &out) {
  out += '"';
  uint32_t n = r.below(24);
  for (uint32_t i = 0; i < n; i++) {
    switch (r.below(9)) {
      case 0: out += "\\n"; break;
      case 1: out += "\\t"; break;
      case 2: out += "\\\""; break;
      case 3: out += "\\\\"; break;
      case 4: out += "\\/"; break;
      case 5: out += "\\u00e9"; break;          // U+00E9 via JSON \u escape
      case 6: out += "\xC3\xA9"; break;         // U+00E9 as raw UTF-8
      case 7: out += "\xF0\x9F\x98\x80"; break; // U+1F600 emoji (4-byte UTF-8)
      default: out += static_cast<char>('a' + r.below(26)); break;
    }
  }
  out += '"';
}

void gen_number(rng &r, std::string &out) {
  switch (r.below(6)) {
    case 0: out += std::to_string(r.below(1000000)); break;
    case 1: out += "-" + std::to_string(r.below(1000000)); break;
    case 2: out += std::to_string(r.below(1000)) + "." + std::to_string(r.below(1000)); break;
    case 3: out += std::to_string(r.below(100)) + "e" + std::to_string(int(r.below(20)) - 10); break;
    case 4: out += "0"; break;
    default: out += std::to_string(r.next()); break; // big uint64
  }
}

void gen_value(rng &r, std::string &out, int depth) {
  // At depth 0 only emit scalars so generation terminates (and produces many
  // short documents near the padding boundary).
  uint32_t choice = (depth <= 0) ? (3 + r.below(4)) : r.below(7);
  switch (choice) {
    case 0: { // object
      out += '{';
      uint32_t n = r.below(5);
      for (uint32_t i = 0; i < n; i++) { if (i) { out += ','; } gen_string(r, out); out += ':'; gen_value(r, out, depth - 1); }
      out += '}';
      break;
    }
    case 1: { // array
      out += '[';
      uint32_t n = r.below(6);
      for (uint32_t i = 0; i < n; i++) { if (i) { out += ','; } gen_value(r, out, depth - 1); }
      out += ']';
      break;
    }
    case 2: gen_string(r, out); break;
    case 3: gen_number(r, out); break;
    case 4: out += "true"; break;
    case 5: out += "false"; break;
    default: out += "null"; break;
  }
}

// Generate thousands of diverse valid documents and require iterate_unpadded to
// agree with the padded iterate() on every one. Under ASAN this is the broadest
// over-read check: many documents end in strings/numbers/escapes at every
// possible offset relative to the buffer end. Top-level scalars are wrapped in an
// array so the document is always a container we can walk.
bool random_documents() {
  TEST_START();
  for (uint64_t seed = 1; seed <= 5000; seed++) {
    rng r(seed * 0x100000001b3ull);
    std::string json;
    gen_value(r, json, 4);
    if (!check_matches(json)) {
      std::cerr << "random_documents failed at seed " << seed << " json=<" << json << ">" << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

// Sweep a string value of every length so its closing quote lands at every
// offset relative to the SIMDJSON_PADDING boundary and the scratch-copy path.
bool string_at_end_sweep() {
  TEST_START();
  for (size_t n = 0; n <= 3 * SIMDJSON_PADDING + 5; n++) {
    std::string json = "{\"k\":\"" + std::string(n, 'a') + "\"}";
    if (!check_matches(json)) { return false; }
    std::string root = "[\"" + std::string(n, 'b') + "\"]";
    if (!check_matches(root)) { return false; }
  }
  TEST_SUCCEED();
}

// Strings with escapes / unicode landing right against the end of the buffer.
bool escapes_at_end() {
  TEST_START();
  const std::vector<std::string> tails = {
    R"(\n)", R"(\t)", R"(\")", R"(\\)", R"(\/)", R"(\b)", R"(\f)", R"(\r)",
    "\xC3\xA9" /* U+00E9 'e-acute', raw UTF-8 */, "\xF0\x9F\x98\x80" /* U+1F600 emoji */,
  };
  // Vary a leading filler so the escape sits at many offsets near the end.
  for (const auto &tail : tails) {
    for (size_t pad = 0; pad <= 2 * SIMDJSON_PADDING; pad++) {
      std::string json = "{\"k\":\"" + std::string(pad, 'x') + tail + "\"}";
      if (!check_matches(json)) { return false; }
      std::string root = "[\"" + std::string(pad, 'y') + tail + "\"]";
      if (!check_matches(root)) { return false; }
    }
  }
  TEST_SUCCEED();
}

// Object key matching near the end of the buffer, including adversarial lookups
// whose key is LONGER than any present key (which the unbounded compare would
// over-read on unpadded input).
bool key_lookup_at_end() {
  TEST_START();
  // The matched key is the last token before the closing brace.
  for (size_t n = 1; n <= 2 * SIMDJSON_PADDING; n++) {
    std::string key(n, 'k');
    std::string json = "{\"a\":1,\"" + key + "\":2}";
    std::vector<char> exact(json.begin(), json.end());
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate_unpadded(exact.data(), exact.size()).get(doc));
    uint64_t v;
    ASSERT_SUCCESS(doc.find_field_unordered(key).get_uint64().get(v));
    ASSERT_EQUAL(v, 2);

    // A lookup key longer than every present key must simply not match (no over-read).
    std::vector<char> exact2(json.begin(), json.end());
    ondemand::parser parser2;
    ondemand::document doc2;
    ASSERT_SUCCESS(parser2.iterate_unpadded(exact2.data(), exact2.size()).get(doc2));
    std::string toolong = key + "_and_then_some_more_bytes";
    auto missing = doc2.find_field_unordered(toolong);
    if (missing.error() == SUCCESS) { std::cerr << "unexpected match for over-long key\n"; return false; }
  }
  TEST_SUCCEED();
}

// A grab-bag of small documents.
bool assorted_documents() {
  TEST_START();
  const std::vector<std::string> docs = {
    "{}", "[]", "[true]", "[false]", "[null]", "[0]", "[-1]", "[123]", "[3.14]", "[1e10]",
    "[\"\"]", "[\"a\"]", "[\"hello world\"]", "[1]", "[1,2,3]",
    R"({"a":1,"b":2,"c":[true,false,null],"d":{"x":"y"}})",
    R"([{"k":"v"},{"k2":"v2"},123,4.5,"end"])",
    // {"unicode":"<U+00E9 U+00E8 U+00EA>","emoji":"<U+1F600>"} as raw UTF-8
    "{\"unicode\":\"\xC3\xA9\xC3\xA8\xC3\xAA\",\"emoji\":\"\xF0\x9F\x98\x80\"}",
    R"(  {  "spaced" :  "value"  }  )",
    R"({"nested":{"deep":{"deeper":{"value":42}}}})",
    "[\"\xEF\xBB\xBF\"]", // a string whose content is a BOM (not a leading BOM)
  };
  for (const auto &d : docs) {
    if (!check_matches(d)) { return false; }
  }
  // Leading BOM + document.
  if (!check_matches(std::string("\xEF\xBB\xBF") + "[1,2,3]")) { return false; }
  TEST_SUCCEED();
}

// Larger, real-world files parsed unpadded from an exact-size buffer. Files that
// are not present in this build are skipped (some are fetched at build time).
bool real_files() {
  TEST_START();
  const char *paths[] = {
    TWITTER_JSON, TWITTER_TIMELINE_JSON, CANADA_JSON, MESH_JSON, APACHE_JSON,
    GSOC_JSON, REPEAT_JSON, DEMO_JSON, SMALLDEMO_JSON, ADVERSARIAL_JSON,
    FLATADVERSARIAL_JSON, TRUENULL_JSON,
  };
  size_t tested = 0;
  for (const char *path : paths) {
    simdjson::padded_string contents;
    if (simdjson::padded_string::load(path).get(contents)) { continue; } // skip missing
    std::string json(contents.data(), contents.size());
    if (!check_matches(json)) {
      std::cerr << "mismatch on file " << path << std::endl;
      return false;
    }
    tested++;
  }
  std::cout << "  (compared " << tested << " real files)" << std::endl;
  TEST_SUCCEED();
}

// The std::string_view / const char* / const uint8_t* overloads, and parser reuse.
bool api_overloads() {
  TEST_START();
  std::string json = R"({"hello":"world","n":42})";

  {
    std::vector<char> exact(json.begin(), json.end());
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate_unpadded(std::string_view(exact.data(), exact.size())).get(doc));
    int64_t n; ASSERT_SUCCESS(doc["n"].get_int64().get(n)); ASSERT_EQUAL(n, 42);
  }
  {
    std::vector<char> exact(json.begin(), json.end());
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate_unpadded(exact.data(), exact.size()).get(doc)); // const char*, len
    std::string_view h; ASSERT_SUCCESS(doc["hello"].get_string().get(h)); ASSERT_EQUAL(h, "world");
  }
  {
    std::vector<char> exact(json.begin(), json.end());
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate_unpadded(reinterpret_cast<const uint8_t*>(exact.data()), exact.size()).get(doc));
    int64_t n; ASSERT_SUCCESS(doc["n"].get_int64().get(n)); ASSERT_EQUAL(n, 42);
  }
  // Reusing a parser for an unpadded then a padded parse must work (flag reset).
  {
    std::vector<char> exact(json.begin(), json.end());
    ondemand::parser parser;
    ondemand::document d1;
    ASSERT_SUCCESS(parser.iterate_unpadded(exact.data(), exact.size()).get(d1));
    int64_t n1; ASSERT_SUCCESS(d1["n"].get_int64().get(n1)); ASSERT_EQUAL(n1, 42);

    simdjson::padded_string padded(json);
    ondemand::document d2;
    ASSERT_SUCCESS(parser.iterate(padded).get(d2));
    int64_t n2; ASSERT_SUCCESS(d2["n"].get_int64().get(n2)); ASSERT_EQUAL(n2, 42);
  }
  TEST_SUCCEED();
}

bool run() {
  return
         numbers() &&
         number_at_end_sweep() &&
         malformed_atoms_at_end() &&
         string_at_end_sweep() &&
         escapes_at_end() &&
         key_lookup_at_end() &&
         assorted_documents() &&
         random_documents() &&
         real_files() &&
         api_overloads() &&
         true;
}

} // namespace ondemand_unpadded_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, ondemand_unpadded_tests::run);
}
