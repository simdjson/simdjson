#ifndef SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H
#include <generic/stage1/base.h>
#include <simdjson/generic/dom_parser_implementation.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ':' ','
  * and when the second element is NOT one of these characters: '}' ']' ':' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
simdjson_inline uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // Variant: do not count separately, just figure out depth
  if(parser.n_structural_indexes == 0) { return 0; }
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  // If we made it to the end, we want to finish counting to see if we have a full document.
  switch (parser.buf[parser.structural_indexes[0]]) {
    case '}':
      obj_cnt--;
      break;
    case ']':
      arr_cnt--;
      break;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
  }
  if (!arr_cnt && !obj_cnt) {
    // We have a complete document.
    return parser.n_structural_indexes;
  }
  return 0;
}

/**
 * Sentinel value returned to indicate a document started but didn't fit
 * (CAPACITY error), as opposed to 0 which means no document content found
 * (EMPTY).
 */
constexpr uint32_t DOCUMENT_TOO_LARGE = UINT32_MAX;

/**
 * For RFC 7464 JSON text sequences, filter RS from structural indexes and
 * find batch boundaries.
 *
 * In JSON sequence mode, RS (0x1E) marks the start of each JSON text.
 * RS bytes appear in structural_indexes as they are classified as scalars.
 * This function:
 * 1. Scans structural_indexes to find and count RS positions
 * 2. Filters RS out of structural_indexes in-place
 * 3. Determines batch boundaries based on RS positions
 *
 * @param parser The parser with structural_indexes and buf.
 * @param len The length of the current batch buffer.
 * @param is_final True if this is the final batch (no more data coming).
 * @param next_batch_start Output: offset where the next batch should start.
 * @return The number of structural indexes to keep (after RS filtering),
 *         0 if no document content found (EMPTY),
 *         or DOCUMENT_TOO_LARGE if a document started but didn't fit (CAPACITY).
 */
simdjson_inline uint32_t find_next_document_index_json_sequence(
    dom_parser_implementation &parser,
    size_t len,
    bool is_final,
    uint32_t &next_batch_start) {
  // Default: next batch starts at end of buffer
  next_batch_start = uint32_t(len);

  if (parser.n_structural_indexes == 0) { return 0; }

  // Phase 1: Scan structural_indexes to find RS positions and handle them.
  // RS marks the start of a JSON text. For objects/arrays, the '{' or '[' after RS
  // is already in structural_indexes (it's an operator). For scalars like numbers,
  // the digit following RS is NOT in structural_indexes because the scanner sees
  // RS as a scalar, making the digit a scalar continuation, not a start.
  // We must: (1) remove RS from structural_indexes, and (2) for scalars, add the
  // actual value start position.
  uint32_t write_idx = 0;
  uint32_t last_rs_pos = 0;
  uint32_t rs_count = 0;

  for (uint32_t read_idx = 0; read_idx < parser.n_structural_indexes; read_idx++) {
    const uint32_t pos = parser.structural_indexes[read_idx];
    if (parser.buf[pos] == 0x1E) {
      // This is an RS character - find the actual JSON value start
      last_rs_pos = pos;
      rs_count++;
      // Skip RS and any whitespace to find the actual value start
      uint32_t value_start = pos + 1;
      while (value_start < len) {
        const uint8_t c = parser.buf[value_start];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { value_start++; }
        else { break; }
      }
      // Check if the value start is an operator (already in structural_indexes),
      // another RS (empty record), or a scalar (needs to be added).
      if (value_start < len) {
        const uint8_t c = parser.buf[value_start];
        const bool is_operator = (c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',');
        const bool is_rs = (c == 0x1E);
        if (!is_operator && !is_rs) {
          // Scalar value - add its position since scanner missed it
          parser.structural_indexes[write_idx++] = value_start;
        }
        // For operators, they will appear later in structural_indexes
        // For RS, it will be processed in a subsequent iteration
      }
    } else {
      // Not RS, copy to output
      parser.structural_indexes[write_idx++] = pos;
    }
  }

  // Update structural index count
  parser.n_structural_indexes = write_idx;

  if (parser.n_structural_indexes == 0) { return 0; }
  if (rs_count == 0) {
    // No RS found; for final batch, try generic boundary detection
    return is_final ? find_next_document_index(parser) : 0;
  }

  // Phase 2: Determine batch boundaries based on RS positions

  if (is_final) {
    // Final batch: all documents are complete (last one ends at EOF).
    // In json_sequence mode, RS markers define document boundaries, so all
    // remaining structurals form complete documents. Return them all directly.
    // (Calling find_next_document_index() would fail for scalar documents.)
    return parser.n_structural_indexes;
  }

  // Partial batch: need to find complete documents only.
  // A document starting at an RS is complete if there is another RS after it.
  next_batch_start = last_rs_pos;

  if (rs_count < 2) {
    // Only one RS, so we have at most one document that may be incomplete.
    // We cannot confirm it is complete without another RS.
    // Return DOCUMENT_TOO_LARGE if content was found (write_idx > 0), 0 if only separators.
    return (parser.n_structural_indexes > 0) ? DOCUMENT_TOO_LARGE : 0;
  }

  // We have at least 2 RS markers. The last complete document ends before last_rs_pos.

  // Find the structural index cutoff: keep only structurals < last_rs_pos.
  // Since we already filtered RS, all remaining structurals are valid.
  // We iterate backward to find the last structural before last_rs_pos.
  uint32_t keep_count = 0;
  for (uint32_t i = parser.n_structural_indexes; i > 0; i--) {
    if (parser.structural_indexes[i - 1] < last_rs_pos) {
      keep_count = i;
      break;
    }
  }

  // No structurals before the last RS - no complete documents
  if (keep_count == 0) { return 0; }

  // All documents before the last RS are complete by definition (the next RS
  // confirms their end). No need to call find_next_document_index() which
  // would fail for scalar documents like `1` or `"hello"`.
  return keep_count;
}

/**
 * Filter comma-delimited documents by removing root-level commas from
 * structural indexes.
 *
 * For comma-delimited format like `{...},{...},{...}`, we need to remove
 * the commas that separate documents (depth 0) while preserving commas
 * inside arrays and objects (depth > 0).
 *
 * After filtering, the structural indexes look like whitespace-delimited
 * documents, so find_next_document_index() works unchanged.
 *
 * @param parser The parser with structural_indexes and buf.
 * @param len The length of the current batch buffer.
 * @param is_final True if this is the final batch (no more data coming).
 * @param next_batch_start Output: offset where the next batch should start.
 * @return The number of structural indexes to keep,
 *         0 if no document content found (EMPTY),
 *         or DOCUMENT_TOO_LARGE if a document started but didn't fit (CAPACITY).
 */
simdjson_inline uint32_t filter_comma_delimited(
    dom_parser_implementation &parser,
    size_t len,
    bool is_final,
    uint32_t &next_batch_start) {
  // Default: next batch starts at end of buffer
  next_batch_start = uint32_t(len);

  if (parser.n_structural_indexes == 0) { return 0; }

  // Track depth to identify root-level commas (depth 0)
  int depth = 0;
  uint32_t write_idx = 0;
  uint32_t last_root_comma_pos = 0;
  uint32_t root_comma_count = 0;

  for (uint32_t i = 0; i < parser.n_structural_indexes; i++) {
    uint32_t idx = parser.structural_indexes[i];
    uint8_t c = parser.buf[idx];

    switch (c) {
      case '{': case '[':
        depth++;
        parser.structural_indexes[write_idx++] = idx;
        break;
      case '}': case ']':
        depth--;
        parser.structural_indexes[write_idx++] = idx;
        break;
      case ',':
        if (depth == 0) {
          // Root-level comma = document boundary, skip it
          last_root_comma_pos = idx;
          root_comma_count++;
          continue;
        }
        parser.structural_indexes[write_idx++] = idx;
        break;
      default:
        // Colons, scalars, etc.
        parser.structural_indexes[write_idx++] = idx;
        break;
    }
  }

  // Update structural index count
  parser.n_structural_indexes = write_idx;

  if (parser.n_structural_indexes == 0) { return 0; }

  if (is_final) {
    // Final batch: use standard boundary detection on filtered indexes
    return find_next_document_index(parser);
  }

  // Partial batch: need to find complete documents only.
  // A document ending with a root comma is complete.
  if (root_comma_count == 0) {
    // No root commas found; we cannot confirm any document is complete.
    // The whole batch might be one incomplete document.
    // Return DOCUMENT_TOO_LARGE if content was found (write_idx > 0), 0 if only commas.
    return (parser.n_structural_indexes > 0) ? DOCUMENT_TOO_LARGE : 0;
  }

  // We have at least one root comma. Documents before the last comma are complete.
  next_batch_start = last_root_comma_pos + 1;

  // Find the structural index cutoff: keep only structurals < last_root_comma_pos
  uint32_t keep_count = 0;
  for (uint32_t i = parser.n_structural_indexes; i > 0; i--) {
    if (parser.structural_indexes[i - 1] < last_root_comma_pos) {
      keep_count = i;
      break;
    }
  }

  if (keep_count == 0) { return 0; }

  // Use standard boundary detection on the complete portion
  parser.n_structural_indexes = keep_count;
  return find_next_document_index(parser);
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H
