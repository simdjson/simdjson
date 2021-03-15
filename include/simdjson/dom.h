#ifndef SIMDJSON_DOM_H
#define SIMDJSON_DOM_H

#include "simdjson/base.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

#include "simdjson/dom/array.h"
#include "simdjson/dom/document_stream.h"
#include "simdjson/dom/document.h"
#include "simdjson/dom/element.h"
#include "simdjson/dom/object.h"
#include "simdjson/dom/parser.h"
#include "simdjson/dom/serialization.h"

// Deprecated API
#include "simdjson/dom/jsonparser.h"
#include "simdjson/dom/parsedjson.h"
#include "simdjson/dom/parsedjson_iterator.h"

// Inline functions
#include "simdjson/dom/array-inl.h"
#include "simdjson/dom/document_stream-inl.h"
#include "simdjson/dom/document-inl.h"
#include "simdjson/dom/element-inl.h"
#include "simdjson/dom/object-inl.h"
#include "simdjson/dom/parsedjson_iterator-inl.h"
#include "simdjson/dom/parser-inl.h"
#include "simdjson/internal/tape_ref-inl.h"
#include "simdjson/dom/serialization-inl.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_DOM_H
