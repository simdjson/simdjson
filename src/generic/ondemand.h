// When *declaring* classes, we use host compiler options so it will all link.
// When *defining* class methods, we use target compiler options.

SIMDJSON_UNTARGET_IMPLEMENTATION
#include "generic/ondemand/logger.h"
#include "generic/ondemand/raw_json_string.h"
#include "generic/ondemand/token_iterator.h"
#include "generic/ondemand/json_iterator.h"
#include "generic/ondemand/array_iterator.h"
#include "generic/ondemand/object_iterator.h"
#include "generic/ondemand/array.h"
#include "generic/ondemand/document.h"
#include "generic/ondemand/value.h"
#include "generic/ondemand/field.h"
#include "generic/ondemand/object.h"
#include "generic/ondemand/parser.h"

SIMDJSON_TARGET_IMPLEMENTATION
#include "generic/ondemand/logger-inl.h"
#include "generic/ondemand/raw_json_string-inl.h"
#include "generic/ondemand/token_iterator-inl.h"
#include "generic/ondemand/json_iterator-inl.h"
#include "generic/ondemand/array_iterator-inl.h"
#include "generic/ondemand/object_iterator-inl.h"
#include "generic/ondemand/array-inl.h"
#include "generic/ondemand/document-inl.h"
#include "generic/ondemand/value-inl.h"
#include "generic/ondemand/field-inl.h"
#include "generic/ondemand/object-inl.h"
#include "generic/ondemand/parser-inl.h"
