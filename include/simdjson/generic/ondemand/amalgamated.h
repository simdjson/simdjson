#if defined(SIMDJSON_CONDITIONAL_INCLUDE) && !defined(SIMDJSON_GENERIC_ONDEMAND_DEPENDENCIES_H)
#error simdjson/generic/ondemand/dependencies.h must be included before simdjson/generic/ondemand/amalgamated.h!
#endif

// Stuff other things depend on
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/deserialize.h"
#include "simdjson/generic/ondemand/value_iterator.h"
#include "simdjson/generic/ondemand/value.h"
#include "simdjson/generic/ondemand/logger.h"
#include "simdjson/generic/ondemand/token_iterator.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/json_type.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/parser.h"

// All other declarations
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/array_iterator.h"
#include "simdjson/generic/ondemand/document.h"
#include "simdjson/generic/ondemand/document_stream.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/serialization.h"

// Deserialization for standard types
#include "simdjson/generic/ondemand/std_deserialize.h"

// Inline definitions
#include "simdjson/generic/ondemand/array-inl.h"
#include "simdjson/generic/ondemand/array_iterator-inl.h"
#include "simdjson/generic/ondemand/value-inl.h"
#include "simdjson/generic/ondemand/document-inl.h"
#include "simdjson/generic/ondemand/document_stream-inl.h"
#include "simdjson/generic/ondemand/field-inl.h"
#include "simdjson/generic/ondemand/json_iterator-inl.h"
#include "simdjson/generic/ondemand/json_type-inl.h"
#include "simdjson/generic/ondemand/logger-inl.h"
#include "simdjson/generic/ondemand/object-inl.h"
#include "simdjson/generic/ondemand/object_iterator-inl.h"
#include "simdjson/generic/ondemand/parser-inl.h"
#include "simdjson/generic/ondemand/raw_json_string-inl.h"
#include "simdjson/generic/ondemand/serialization-inl.h"
#include "simdjson/generic/ondemand/token_iterator-inl.h"
#include "simdjson/generic/ondemand/value_iterator-inl.h"

// JSON builder, ideally they should not be part of the ondemand directory
// but it is convenient for now to have them here.
#include "simdjson/generic/ondemand/json_string_builder.h"
#include "simdjson/generic/ondemand/json_string_builder-inl.h"
#include "simdjson/generic/ondemand/json_builder.h"

