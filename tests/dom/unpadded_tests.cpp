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

// Root and non-root numbers in every shape. Root numbers exercise the
// space-padded-copy path (visit_root_number); non-root numbers are followed by
// an in-buffer structural character.
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
    std::string num = "0.";
    num.append(frac, '9');
    if (!check_matches(num)) { return false; }                       // root float
    { std::string s = "["; s += num; s += "]"; if (!check_matches(s)) { return false; } }           // float ends the array
    { std::string s = "{\"k\":"; s += num; s += "}"; if (!check_matches(s)) { return false; } }     // float ends the object
    // Long float preceded by filler so it starts far from the end but its digits
    // still reach the final bytes (exercises the next-structural-based trigger).
    std::string longfloat = "[123456789,"; longfloat += num; longfloat += "]";
    if (!check_matches(longfloat)) { return false; }
    // Long integer at the end too.
    std::string bignum = std::string(frac, '7');
    std::string longint = "[1,"; longint += bignum; longint += "]";
    if (!check_matches(longint)) { return false; }
  }
  TEST_SUCCEED();
}

// Malformed / truncated atom-like tokens at the buffer end. The 'n'/'t'/'f'
// dispatch validates a value against the full null/true/false atom, reading a
// fixed number of bytes; a shorter or wrong token at the end of an unpadded
// buffer must not read past it (caught here under ASAN). All of these are
// invalid JSON, so padded and unpadded must agree on the error.
bool malformed_atoms_at_end() {
  TEST_START();
  const std::vector<std::string> toks = {
    "n", "nu", "nul", "nulx", "nall", "t", "tr", "tru", "trux", "ture",
    "f", "fa", "fal", "fals", "falx", "fale", "true", "false", "null",
  };
  for (const auto &t : toks) {
    if (!check_matches(t)) { return false; }                        // root
    if (!check_matches("[" + t + "]")) { return false; }            // ends the array
    if (!check_matches("{\"k\":" + t + "}")) { return false; }      // ends the object
    if (!check_matches("[" + t + ",1]")) { return false; }
    { std::string s = "["; s.append(SIMDJSON_PADDING, ' '); s += t; s += "]"; if (!check_matches(s)) { return false; } }
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
    case 1: out += "-"; out += std::to_string(r.below(1000000)); break;
    case 2: out += std::to_string(r.below(1000)); out += '.'; out += std::to_string(r.below(1000)); break;
    case 3: out += std::to_string(r.below(100)); out += 'e'; out += std::to_string(int(r.below(20)) - 10); break;
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

// Generate thousands of diverse valid documents and require parse_unpadded to
// agree with the padded parser on every one. Under ASAN this is the broadest
// over-read check: many documents end in strings/numbers/escapes at every
// possible offset relative to the buffer end.
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
    std::string json = "{\"k\":\"";
    json.append(n, 'a');
    json += "\"}";
    if (!check_matches(json)) { return false; }
    // Also a bare root string of length n.
    std::string root = "\"";
    root.append(n, 'b');
    root += "\"";
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
    R"(\uD800)" /* lone high surrogate (valid JSON string) */,
  };
  // Vary a leading filler so the escape sits at many offsets near the end.
  for (const auto &tail : tails) {
    for (size_t pad = 0; pad <= 2 * SIMDJSON_PADDING; pad++) {
      std::string json = "{\"k\":\"";
      json.append(pad, 'x');
      json += tail;
      json += "\"}";
      if (!check_matches(json)) { return false; }
      std::string root = "\"";
      root.append(pad, 'y');
      root += tail;
      root += "\"";
      if (!check_matches(root)) { return false; }
    }
  }
  TEST_SUCCEED();
}

// Targeted regression test for a surrogate-pair lookahead read that reaches
// +11 bytes past a backslash in the fast (BYTES_PROCESSED-sized) loop of
// parse_string_safe. On kernels where BYTES_PROCESSED == SIMDJSON_PADDING
// (e.g. icelake) this requires a guard margin of at least +12. Stage 1 accepts
// the document (the final " closes the string) but the escape is a high
// surrogate followed by a truncated \u; the second hex_to_u32_nocheck reads
// 4 bytes at the critical offset. We place the backslash at content offset 63
// so it is bs_dist=63 of the first chunk on 64-byte kernels.
bool surrogate_pair_deep_lookahead_at_chunk_boundary() {
  TEST_START();
  // Root string case.
  std::string json = "\"";
  json.append(63, 'a');
  json += "\\uD800\\u\"";   // high surrogate + \u (truncated) + string closer
  if (!check_matches(json)) { return false; }

  // Non-root (object value) case.
  std::string obj = "{\"k\":\"";
  obj.append(63, 'b');
  obj += "\\uD800\\u\"}";  // final " closes the string value
  if (!check_matches(obj)) { return false; }

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
    // {"unicode":"<U+00E9 U+00E8 U+00EA>","emoji":"<U+1F600>"} as raw UTF-8
    "{\"unicode\":\"\xC3\xA9\xC3\xA8\xC3\xAA\",\"emoji\":\"\xF0\x9F\x98\x80\"}",
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

// The std::string_view / const char* overloads of the public API.
bool api_overloads() {
  TEST_START();
  std::string json = R"({"hello":"world","n":42})";
  std::vector<char> exact(json.begin(), json.end());

  int64_t n = 0;
  std::string_view sv;

  dom::parser p1;
  dom::element e1;
  ASSERT_SUCCESS( p1.parse_unpadded(std::string_view(exact.data(), exact.size())).get(e1) );
  ASSERT_SUCCESS( e1["n"].get(n) );
  ASSERT_EQUAL( n, 42 );
  ASSERT_SUCCESS( e1["hello"].get(sv) );
  ASSERT_EQUAL( sv, "world" );

  dom::parser p2;
  dom::element e2;
  ASSERT_SUCCESS( p2.parse_unpadded(exact.data(), exact.size()).get(e2) ); // const char*, len
  ASSERT_SUCCESS( e2["n"].get(n) );
  ASSERT_EQUAL( n, 42 );

  // parse_into_document_unpadded with a caller-provided document.
  dom::parser p3;
  dom::document doc;
  dom::element e3;
  ASSERT_SUCCESS( p3.parse_into_document_unpadded(doc, reinterpret_cast<const uint8_t*>(exact.data()), exact.size()).get(e3) );
  ASSERT_SUCCESS( e3["hello"].get(sv) );
  ASSERT_EQUAL( sv, "world" );

  // Reusing a parser for an unpadded then a padded parse must work (flag reset).
  dom::parser p4;
  dom::element tmp;
  ASSERT_SUCCESS( p4.parse_unpadded(exact.data(), exact.size()).get(tmp) );
  ASSERT_SUCCESS( p4.parse(simdjson::padded_string(json)).get(tmp) );
  ASSERT_SUCCESS( tmp["n"].get(n) );
  ASSERT_EQUAL( n, 42 );
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

#if SIMDJSON_ENABLE_NAN_INF
// Only built with -DSIMDJSON_ENABLE_NAN_INF=ON. The non-root inf validator does
// an 8-byte 'infinity' compare; a malformed inf-spelling at the very end of an
// unpadded buffer would over-read it without the length-aware guard. Includes
// valid and malformed tokens, at the document end and behind padding-sized filler.
bool nan_inf_at_end() {
  TEST_START();
  const std::vector<std::string> toks = {
    "inf", "Inf", "INF", "infinity", "Infinity", "nan", "NaN", "NAN",
    "in", "inf2", "infi", "infin", "infinit", "na", "nat", "nana",
  };
  for (const auto &t : toks) {
    if (!check_matches(t)) { return false; }                         // root
    if (!check_matches("[" + t + "]")) { return false; }             // ends the array
    if (!check_matches("{\"k\":" + t + "}")) { return false; }       // ends the object
    { std::string s = "["; s.append(SIMDJSON_PADDING, ' '); s += t; s += "]"; if (!check_matches(s)) { return false; } }
  }
  TEST_SUCCEED();
}
#endif

bool run() {
  return
#if SIMDJSON_ENABLE_NAN_INF
         nan_inf_at_end() &&
#endif
         numbers() &&
         number_at_end_sweep() &&
         malformed_atoms_at_end() &&
         string_at_end_sweep() &&
         escapes_at_end() &&
         surrogate_pair_deep_lookahead_at_chunk_boundary() &&
         assorted_documents() &&
         degenerate_inputs() &&
         random_documents() &&
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
