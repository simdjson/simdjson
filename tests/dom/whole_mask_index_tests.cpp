#include "simdjson.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "test_macros.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

bool whole_mask_indexes() {
  TEST_START();
  const simdjson::implementation *implementation = simdjson::get_active_implementation();
  // Counts around 32 exercise the split between widening the lower and upper halves.
  for (size_t count = 0; count <= 64; ++count) {
    for (size_t tail : {size_t(0), size_t(1), size_t(31), size_t(63), size_t(65)}) {
      std::string text(192 + tail, ' ');
      for (size_t block = 0; block < 3; ++block) {
        const size_t selected = block == 1 ? 64 - count : count;
        for (size_t i = 0; i < selected; ++i) {
          text[block * 64 + (i * 17 + count) % 64] = "[{}]"[i % 4];
        }
      }
      for (size_t i = 192; i < text.size(); i += 2) { text[i] = '['; }
      std::vector<uint32_t> expected;
      for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != ' ') { expected.push_back(uint32_t(i)); }
      }
      for (size_t alignment : {size_t(0), size_t(1), size_t(7), size_t(63)}) {
        std::vector<uint8_t> storage(text.size() + simdjson::SIMDJSON_PADDING + 63, ' ');
        uint8_t *input = storage.data() + alignment;
        std::memcpy(input, text.data(), text.size());
        std::unique_ptr<simdjson::internal::dom_parser_implementation> parser;
        ASSERT_SUCCESS(implementation->create_dom_parser_implementation(text.size(), 1024, parser));
        // Only stage 1 is under test: these masks need not form valid JSON containers.
        ASSERT_SUCCESS(parser->stage1(input, text.size(), simdjson::stage1_mode::regular));
        ASSERT_EQUAL(parser->n_structural_indexes, expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
          ASSERT_EQUAL(parser->structural_indexes[i], expected[i]);
        }
        ASSERT_EQUAL(parser->structural_indexes[expected.size()], text.size());
        ASSERT_EQUAL(parser->structural_indexes[expected.size() + 1], text.size());
        ASSERT_EQUAL(parser->structural_indexes[expected.size() + 2], uint32_t(0));
      }
    }
  }
  TEST_SUCCEED();
}

int main() {
  return whole_mask_indexes() ? EXIT_SUCCESS : EXIT_FAILURE;
}
