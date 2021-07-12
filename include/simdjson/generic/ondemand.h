namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
/**
 * A fast, simple, DOM-like interface that parses JSON as you use it.
 *
 * Designed for maximum speed and a lower memory profile.
 */
namespace ondemand {

/** Represents the depth of a JSON value (number of nested arrays/objects). */
using depth_t = int32_t;

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "simdjson/generic/ondemand/json_type.h"
#include "simdjson/generic/ondemand/token_position.h"
#include "simdjson/generic/ondemand/logger.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/token_iterator.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/value_iterator.h"
#include "simdjson/generic/ondemand/array_iterator.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/document.h"
#include "simdjson/generic/ondemand/value.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/parser.h"
#include "simdjson/generic/ondemand/document_stream.h"
#include "simdjson/generic/ondemand/serialization.h"
