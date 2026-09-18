#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "cast_tester.h"
#include "simdjson.h"
#include "test_macros.h"

const char *test_files[] = {
    TWITTER_JSON, TWITTER_TIMELINE_JSON, REPEAT_JSON, CANADA_JSON,
    MESH_JSON,    APACHE_JSON,           GSOC_JSON};

/**
 * The general idea of these tests if that if you take a JSON file,
 * load it, then convert it into a string, then parse that, and
 * convert it again into a second string, then the two strings should
 * be  identifical. If not, then something was lost or added in the
 * process.
 */

bool check_prettified_indentation(const std::string &serial, int step,
                                  size_t *out_max_depth = nullptr) {
  if (serial.empty() || serial.back() != '\n') {
    return false;
  }
  const size_t eff_step = step > 0 ? static_cast<size_t>(step) : 0;
  size_t depth = 0;
  size_t max_depth = 0;
  size_t pos = 0;
  while (pos < serial.size()) {
    size_t eol = serial.find('\n', pos);
    if (eol == std::string::npos) {
      return false;
    }
    size_t lead = 0;
    while (pos + lead < eol && serial[pos + lead] == ' ') {
      lead++;
    }
    if (pos + lead == eol) {
      return false; // empty or whitespace-only line
    }
    const char first = serial[pos + lead];
    const char last = serial[eol - 1];
    if (first == ']' || first == '}') {
      if (depth == 0) {
        return false;
      }
      depth--;
    }
    if (lead != depth * eff_step) {
      std::cerr << "Expected " << (depth * eff_step) << " leading spaces, got "
                << lead << " (step=" << step << ", depth=" << depth << ")"
                << std::endl;
      return false;
    }
    if (last == '[' || last == '{') {
      depth++;
      if (depth > max_depth) {
        max_depth = depth;
      }
    }
    pos = eol + 1;
  }
  if (out_max_depth) {
    *out_max_depth = max_depth;
  }
  return depth == 0;
}

bool load_prettify(const char *filename) {
  std::cout << "Loading " << filename << std::endl;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.load(filename).get(doc);
  if (error) { std::cerr << error << std::endl; return false; }
  const std::string expected_minified = simdjson::minify(doc);

  // Test default (4-space), 2-space, and <= 0 (newlines only, no indentation)
  for (int step : {4, 2, -1}) {
    std::string serial1 = (step == 4) ? simdjson::prettify(doc)
                                      : simdjson::prettify(doc, step);
    if (!check_prettified_indentation(serial1, step)) {
      std::cerr << "Actual indentation check failed for " << filename
                << " with step=" << step << std::endl;
      return false;
    }
    simdjson::dom::element reparsed;
    error = parser.parse(serial1).get(reparsed);
    if (error) { std::cerr << error << std::endl; return false; }
    if (simdjson::minify(reparsed) != expected_minified) {
      std::cerr << "Prettified JSON content mismatch for " << filename
                << " with step=" << step << std::endl;
      return false;
    }
    std::string serial2 = (step == 4) ? simdjson::prettify(reparsed)
                                      : simdjson::prettify(reparsed, step);
    if (serial1 != serial2) {
      std::cerr << "The content differs after re-prettifying!" << std::endl;
      return false;
    }
  }
  std::cout << "Actual indentation and round-trip prettify checks passed."
            << std::endl;
  return true;
}

bool prettify_test() {
  std::cout << "Running " << __func__ << std::endl;

  for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
    bool ok = load_prettify(test_files[i]);
    if (!ok) {
      return false;
    }
  }
  return true;
}

#if SIMDJSON_EXCEPTIONS
bool prettify_result_test() {
  simdjson::dom::parser parser;
  auto result = parser.parse(std::string("[1,2]"));
  simdjson::dom::element doc;
  if (result.get(doc) != simdjson::SUCCESS) { return false; }
  auto formatted = simdjson::prettify(result);
  return formatted == simdjson::prettify(doc) && formatted != "[1,2]";
}
#endif

bool prettify_deep_indent_test() {
  std::cout << "Running " << __func__ << std::endl;
  simdjson::padded_string raw;
  if (simdjson::padded_string::load(TWITTER_JSON).get(raw)) { return false; }

  constexpr size_t wrapper_levels = 20; // > MAX_DEPTH (16)
  std::string input = std::string(wrapper_levels, '[') +
                      std::string(raw.data(), raw.size()) +
                      std::string(wrapper_levels, ']');

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  if (parser.parse(input).get(doc)) { return false; }
  const std::string expected_minified = simdjson::minify(doc);

  for (int step : {2, 3, 20, 0, -1}) {
    std::string serial = simdjson::prettify(doc, step);
    size_t max_depth = 0;
    if (!check_prettified_indentation(serial, step, &max_depth) ||
        max_depth <= wrapper_levels) {
      std::cerr << "Deep indentation check failed on twitter.json (step="
                << step << ", max_depth=" << max_depth << ")" << std::endl;
      return false;
    }
    simdjson::dom::element reparsed;
    if (parser.parse(serial).get(reparsed)) { return false; }
    if (simdjson::minify(reparsed) != expected_minified) { return false; }
  }
  return true;
}

int main() {
#if SIMDJSON_EXCEPTIONS
  if (!prettify_result_test()) { return EXIT_FAILURE; }
#endif
  if (!prettify_deep_indent_test()) { return EXIT_FAILURE; }
  return prettify_test() ? EXIT_SUCCESS : EXIT_FAILURE;
}
