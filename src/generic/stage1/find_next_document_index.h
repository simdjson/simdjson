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

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H