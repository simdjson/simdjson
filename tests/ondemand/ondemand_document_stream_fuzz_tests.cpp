#include <string>
#include <vector>

#include "simdjson.h"
#include "test_ondemand.h"
#include "document_stream_fuzz_test_common.h"

namespace document_stream_fuzz_tests {

std::string strip_stream_artifacts(simdjson::stream_format format, std::string_view text) {
  size_t start = 0;
  size_t end = text.size();
  while (start < end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\n' || text[start] == '\r' || text[start] == '\x1e')) {
    start++;
  }
  while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' || text[end - 1] == '\r' || text[end - 1] == '\x1e')) {
    end--;
  }
  std::string cleaned(text.substr(start, end - start));
  if (format == simdjson::stream_format::comma_delimited || format == simdjson::stream_format::comma_delimited_array) {
    while (!cleaned.empty() && cleaned.back() == ',') {
      cleaned.pop_back();
      while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t' || cleaned.back() == '\n' || cleaned.back() == '\r')) {
        cleaned.pop_back();
      }
    }
  }
  return cleaned;
}

std::string canonicalize_document(simdjson::stream_format format, std::string_view text) {
  std::string cleaned = strip_stream_artifacts(format, text);
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  if (parser.parse(cleaned).get(doc)) {
    return std::string("PARSE_ERROR:") + cleaned;
  }
  return simdjson::minify(doc);
}

const std::vector<std::string> &expected_documents() {
  static const std::vector<std::string> docs = document_stream_fuzz::make_documents();
  return docs;
}

const std::vector<document_stream_fuzz::stream_case> &stream_cases() {
  static const std::vector<document_stream_fuzz::stream_case> cases =
      document_stream_fuzz::make_stream_cases(expected_documents());
  return cases;
}

bool verify_case(const document_stream_fuzz::stream_case &test_case) {
  TEST_START();
  const auto &expected = test_case.expected_documents;
  ASSERT_TRUE(test_case.input.size() > document_stream_fuzz::batch_size * 4);

  simdjson::padded_string input(test_case.input);
  simdjson::ondemand::parser parser;

  for (int pass = 0; pass < 2; pass++) {
    simdjson::ondemand::document_stream stream;
    ASSERT_SUCCESS(parser.iterate_many(input, document_stream_fuzz::batch_size, test_case.format).get(stream));

    size_t index = 0;
    for (auto it = stream.begin(); it != stream.end(); ++it) {
      auto doc_result = *it;
      ASSERT_SUCCESS(doc_result.error());
      ASSERT_TRUE(index < expected.size());

      simdjson::ondemand::document_reference doc;
      ASSERT_SUCCESS(doc_result.get(doc));

      std::string_view actual;
      ASSERT_SUCCESS(simdjson::to_json_string(doc).get(actual));
      ASSERT_EQUAL(
          canonicalize_document(test_case.format, actual),
          canonicalize_document(test_case.format, expected[index])
      );
      index++;
    }
    ASSERT_EQUAL(index, expected.size());
  }

  TEST_SUCCEED();
}

bool run() {
  for (const auto &test_case : stream_cases()) {
    std::cout << "Running fuzz corpus against stream format: " << test_case.name << std::endl;
    if (!verify_case(test_case)) {
      return false;
    }
  }
  return true;
}

} // namespace document_stream_fuzz_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, document_stream_fuzz_tests::run);
}