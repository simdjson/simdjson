#ifndef SIMDJSON_H
#define SIMDJSON_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/lemire/simdjson/blob/master/README.md#simdjson--parsing-gigabytes-of-json-per-second).
 * 
 * Sample code. See https://github.com/simdjson/simdjson/blob/master/doc/basics.md for more examples.

    #include "simdjson.h"

    int main(void) {
      // load from `twitter.json` file:
      simdjson::dom::parser parser;
      simdjson::dom::element tweets = parser.load("twitter.json");
      std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

      // Parse and iterate through an array of objects
      auto abstract_json = R"( [
        {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
        {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
        ] )"_padded;
  
      for (simdjson::dom::object obj : parser.parse(abstract_json)) {
        for(const auto& key_value : obj) {
          cout << "key: " << key_value.key << " : ";
          simdjson::dom::object innerobj = key_value.value;
          cout << "a: " << double(innerobj["a"]) << ", ";
          cout << "b: " << double(innerobj["b"]) << ", ";
          cout << "c: " << int64_t(innerobj["c"]) << endl;
        }
      }
    }
 */

#include "simdjson/compiler_check.h"
#include "simdjson/common_defs.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdjson/simdjson_version.h"
#include "simdjson/error.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/implementation.h"
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
#include "simdjson/error-inl.h"
#include "simdjson/dom/object-inl.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/dom/parsedjson_iterator-inl.h"
#include "simdjson/dom/parser-inl.h"
#include "simdjson/internal/tape_ref-inl.h"
#include "simdjson/dom/serialization-inl.h"

// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDJSON_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdjson/internal/isadetection.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

// Implementations
#include "simdjson/arm64.h"
#include "simdjson/haswell.h"
#include "simdjson/westmere.h"
#include "simdjson/fallback.h"
#include "simdjson/builtin.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_H
