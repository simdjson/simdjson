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
        for(const auto key_value : obj) {
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
#include "simdjson/padded_string_view.h"
#include "simdjson/implementation.h"

// Inline functions
#include "simdjson/error-inl.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/padded_string_view-inl.h"

// DOM
#include "simdjson/dom.h"
// Implementations
#include "simdjson/implementations.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_H
