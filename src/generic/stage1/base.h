#ifndef SIMDJSON_SRC_GENERIC_STAGE1_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_BASE_H
#include <generic/base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

class bit_indexer;
template<size_t STEP_SIZE>
struct buf_block_reader;
struct json_block;
class json_minifier;
class json_scanner;
struct json_string_block;
class json_string_scanner;
class json_structural_indexer;

} // namespace stage1

namespace utf8_validation {
struct utf8_checker;
} // namespace utf8_validation

using utf8_validation::utf8_checker;

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_BASE_H
