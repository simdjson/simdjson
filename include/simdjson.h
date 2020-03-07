#ifndef SIMDJSON_H
#define SIMDJSON_H

#include "simdjson/compiler_check.h"

// Public API
#include "simdjson/simdjson_version.h"
#include "simdjson/error.h"
#include "simdjson/padded_string.h"
#include "simdjson/implementation.h"
#include "simdjson/document.h"
#include "simdjson/document_stream.h"
#include "simdjson/jsonminifier.h"

// Deprecated API
#include "simdjson/parsedjsoniterator.h"
#include "simdjson/jsonparser.h"
#include "simdjson/parsedjson.h"
#include "simdjson/jsonstream.h"
#include "simdjson/document_iterator.h"

// Inline functions
#include "simdjson/inline/document.h"
#include "simdjson/inline/document_iterator.h"
#include "simdjson/inline/document_stream.h"
#include "simdjson/inline/error.h"
#include "simdjson/inline/jsonstream.h"
#include "simdjson/inline/padded_string.h"

#endif // SIMDJSON_H
