#if defined(SIMDJSON_AMALGAMATED) && !defined(SIMDJSON_GENERIC_SINGLESTAGE_DEPENDENCIES_H)
#error simdjson/generic/singlestage/dependencies.h must be included before simdjson/generic/singlestage/amalgamated.h!
#endif

// Stuff other things depend on
#include "simdjson/generic/singlestage/base.h"
#include "simdjson/generic/singlestage/value_iterator.h"
#include "simdjson/generic/singlestage/value.h"
#include "simdjson/generic/singlestage/logger.h"
#include "simdjson/generic/singlestage/token_iterator.h"
#include "simdjson/generic/singlestage/json_iterator.h"
#include "simdjson/generic/singlestage/json_type.h"
#include "simdjson/generic/singlestage/raw_json_string.h"
#include "simdjson/generic/singlestage/parser.h"

// All other declarations
#include "simdjson/generic/singlestage/array.h"
#include "simdjson/generic/singlestage/array_iterator.h"
#include "simdjson/generic/singlestage/document.h"
#include "simdjson/generic/singlestage/document_stream.h"
#include "simdjson/generic/singlestage/field.h"
#include "simdjson/generic/singlestage/object.h"
#include "simdjson/generic/singlestage/object_iterator.h"
#include "simdjson/generic/singlestage/serialization.h"

// Inline definitions
#include "simdjson/generic/singlestage/array-inl.h"
#include "simdjson/generic/singlestage/array_iterator-inl.h"
#include "simdjson/generic/singlestage/document-inl.h"
#include "simdjson/generic/singlestage/document_stream-inl.h"
#include "simdjson/generic/singlestage/field-inl.h"
#include "simdjson/generic/singlestage/json_iterator-inl.h"
#include "simdjson/generic/singlestage/json_type-inl.h"
#include "simdjson/generic/singlestage/logger-inl.h"
#include "simdjson/generic/singlestage/object-inl.h"
#include "simdjson/generic/singlestage/object_iterator-inl.h"
#include "simdjson/generic/singlestage/parser-inl.h"
#include "simdjson/generic/singlestage/raw_json_string-inl.h"
#include "simdjson/generic/singlestage/serialization-inl.h"
#include "simdjson/generic/singlestage/token_iterator-inl.h"
#include "simdjson/generic/singlestage/value-inl.h"
#include "simdjson/generic/singlestage/value_iterator-inl.h"
