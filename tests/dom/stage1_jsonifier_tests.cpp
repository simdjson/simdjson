// Tests that stage1_jsonifier (the Jsonifier stage 1) produces the same
// error code and the same structural indexes as the regular stage1, on every
// implementation the CPU supports. Implementations without the Jsonifier
// stage 1 fall back to the regular one, so they pass trivially.
//
// Inputs of at most 128 bytes take the regular small-input path in both, so
// the generated documents are longer than that: they shift escapes, quotes and
// scalars across every alignment of a 64-byte block and leave tails of every
// length, which exercises the narrow and full tail blocks of the reader.

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

namespace stage1_jsonifier_tests {
using namespace simdjson;

using parser_ptr = std::unique_ptr<internal::dom_parser_implementation>;

void snapshot(const parser_ptr &parser, std::vector<uint32_t> &indexes) {
  indexes.assign(parser->structural_indexes.get(), parser->structural_indexes.get() + parser->n_structural_indexes);
}

bool check_document(const implementation &impl, const padded_string &json, const std::string &label) {
  parser_ptr parser;
  if (impl.create_dom_parser_implementation(json.size(), DEFAULT_MAX_DEPTH, parser)) {
    std::cerr << impl.name() << ": could not allocate a parser for " << label << std::endl;
    return false;
  }
  std::vector<uint32_t> regular_indexes;
  std::vector<uint32_t> jsonifier_indexes;
  const error_code regular_error = parser->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.size(), stage1_mode::regular);
  snapshot(parser, regular_indexes);
  const error_code jsonifier_error = parser->stage1_jsonifier(reinterpret_cast<const uint8_t *>(json.data()), json.size());
  snapshot(parser, jsonifier_indexes);
  if (regular_error != jsonifier_error) {
    std::cerr << impl.name() << ": " << label << ": error " << error_message(regular_error) << " vs " << error_message(jsonifier_error) << std::endl;
    return false;
  }
  if (regular_indexes != jsonifier_indexes) {
    const auto mismatch = std::mismatch(regular_indexes.begin(), regular_indexes.end(), jsonifier_indexes.begin(), jsonifier_indexes.end());
    std::cerr << impl.name() << ": " << label << ": " << regular_indexes.size() << " vs " << jsonifier_indexes.size()
              << " structurals, first difference at index " << (mismatch.first - regular_indexes.begin()) << std::endl;
    return false;
  }
  return true;
}

bool check_all_implementations(const padded_string &json, const std::string &label) {
  bool ok = true;
  for (const implementation *impl : get_available_implementations()) {
    if (impl->supported_by_runtime_system()) { ok = check_document(*impl, json, label) && ok; }
  }
  return ok;
}

bool corpus() {
  TEST_START();
  const std::filesystem::path directory(SIMDJSON_BENCHMARK_DATA_DIR);
  size_t checked = 0;
  bool ok = true;
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".json") { continue; }
    padded_string json;
    if (padded_string::load(entry.path().string()).get(json)) { continue; }
    ok = check_all_implementations(json, entry.path().filename().string()) && ok;
    checked++;
  }
  std::cout << "checked " << checked << " corpus files" << std::endl;
  ASSERT_TRUE(checked > 0);
  ASSERT_TRUE(ok);
  TEST_SUCCEED();
}

// A record with escaped quotes, escaped backslashes, numbers and atoms.
const std::string RECORD(R"({"k\"ey":[1,-2.5e3,true,false,null,"v\\",""],"n":{"x\\\"y":"z"}})");

bool shifted_records() {
  TEST_START();
  bool ok = true;
  for (size_t prefix = 0; prefix < 64; prefix++) {
    for (size_t extra = 0; extra < 70; extra++) {
      std::string text(prefix, ' ');
      text.append("[");
      while (text.size() < 130 + extra) {
        text.append(RECORD).append(",");
      }
      text.append(RECORD).append("]");
      padded_string json(text);
      std::string label("shifted record, prefix ");
      label.append(std::to_string(prefix)).append(", length ").append(std::to_string(text.size()));
      ok = check_all_implementations(json, label) && ok;
    }
  }
  ASSERT_TRUE(ok);
  TEST_SUCCEED();
}

bool backslash_runs() {
  TEST_START();
  bool ok = true;
  for (size_t run = 0; run < 70; run++) {
    for (size_t offset = 0; offset < 64; offset++) {
      std::string text(140 + offset, ' ');
      text.append("[\"");
      text.append(run & ~size_t(1), '\\');
      text.append("\",1]");
      padded_string json(text);
      std::string label("backslash run ");
      label.append(std::to_string(run & ~size_t(1))).append(", offset ").append(std::to_string(offset));
      ok = check_all_implementations(json, label) && ok;
    }
  }
  ASSERT_TRUE(ok);
  TEST_SUCCEED();
}

bool unclosed_strings() {
  TEST_START();
  bool ok = true;
  for (size_t length = 130; length < 330; length++) {
    std::string text("[1,\"");
    text.append(length - text.size(), 'a');
    padded_string json(text);
    std::string label("unclosed string, length ");
    label.append(std::to_string(length));
    ok = check_all_implementations(json, label) && ok;
  }
  ASSERT_TRUE(ok);
  TEST_SUCCEED();
}

bool run() {
  return corpus() && shifted_records() && backslash_runs() && unclosed_strings();
}

} // namespace stage1_jsonifier_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, stage1_jsonifier_tests::run);
}
