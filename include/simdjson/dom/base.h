#ifndef SIMDJSON_DOM_BASE_H
#define SIMDJSON_DOM_BASE_H

#include "simdjson/base.h"

namespace simdjson {

/**
 * @brief A DOM API on top of the simdjson parser.
 */
namespace dom {

/** The default batch size for parser.parse_many() and parser.load_many() */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;
/**
 * Some adversary might try to set the batch size to 0 or 1, which might cause problems.
 * We set a minimum of 32B since anything else is highly likely to be an error. In practice,
 * most users will want a much larger batch size.
 *
 * All non-negative MINIMAL_BATCH_SIZE values should be 'safe' except that, obviously, no JSON
 * document can ever span 0 or 1 byte and that very large values would create memory allocation issues.
 */
static constexpr size_t MINIMAL_BATCH_SIZE = 32;

/**
 * It is wasteful to allocate memory for tiny documents (e.g., 4 bytes).
 */
static constexpr size_t MINIMAL_DOCUMENT_CAPACITY = 32;

class array;
class document;
class document_stream;
class element;
class key_value_pair;
class object;
class parser;

#ifdef SIMDJSON_THREADS_ENABLED
struct stage1_worker;
#endif // SIMDJSON_THREADS_ENABLED

} // namespace dom

namespace internal {

template<typename T>
class string_builder;
class tape_ref;

} // namespace internal

} // namespace simdjson

#endif // SIMDJSON_DOM_BASE_H