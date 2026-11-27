// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_STAGE1_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_STAGE1_H
#include <generic/jsonifier/simd_string_reader.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

// Lone atoms and padded inputs of at most 128 bytes take the same paths as the
// regular stage 1. Longer inputs go through the Jsonifier reader, which indexes
// structurals but does not validate UTF-8 or reject unescaped control
// characters inside strings.
simdjson_inline error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  if (len == 0) { return EMPTY; }
  if (len <= 64 && buf[0] != '{' && buf[0] != '[' && buf[0] != '"') {
    error_code atom = simdjson::internal::try_stage1_lone_atom(parser, buf, len);
    if (atom != UNINITIALIZED) { return atom; }
  }
  if (!parser._unpadded && len <= 128) {
    return len <= 64 ? stage1::json_structural_indexer::index_small<1>(buf, len, parser)
                     : stage1::json_structural_indexer::index_small<2>(buf, len, parser);
  }
  simd_string_reader reader;
  reader.template reset<false>(buf, len, parser.structural_indexes.get());
  parser.n_structural_indexes = static_cast<uint32_t>(reader.tape_count);
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 1] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 2] = 0;
  parser.next_structural_index = 0;
  if (simdjson_unlikely(reader.ends_in_string)) { return UNCLOSED_STRING; }
  if (simdjson_unlikely(parser.n_structural_indexes == 0u)) { return EMPTY; }
  return SUCCESS;
}

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_STAGE1_H
