#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "NullBuffer.h"

// example from doc/basics.md#tree-walking-and-json-element-types
/***
 * Important: This function should compile without support for exceptions.
 */
static void print_json(std::ostream& os, simdjson::dom::element element) noexcept {
  const char endl='\n';
  switch (element.type()) {
  case simdjson::dom::element_type::ARRAY:
    os << "[";
    {
      simdjson::dom::array array = element.get_array().value_unsafe();
      for (simdjson::dom::element child : array) {
        print_json(os, child);
        os << ",";
      }
    }
    os << "]";
    break;
  case simdjson::dom::element_type::OBJECT:
    os << "{";
    {
      simdjson::dom::object object = element.get_object().value_unsafe();
      for (simdjson::dom::key_value_pair field : object) {
        os << "\"" << field.key << "\": ";
        print_json(os, field.value);
      }
    }
    os << "}";
    break;
  case simdjson::dom::element_type::INT64:
    os << element.get_int64().value_unsafe() << endl;
    break;
  case simdjson::dom::element_type::UINT64:
    os << element.get_uint64().value_unsafe() << endl;
    break;
  case simdjson::dom::element_type::DOUBLE:
    os << element.get_double().value_unsafe() << endl;
    break;
  case simdjson::dom::element_type::STRING:
    os << element.get_string().value_unsafe() << endl;
    break;
  case simdjson::dom::element_type::BOOL:
    os << element.get_bool().value_unsafe() << endl;
    break;
  case simdjson::dom::element_type::NULL_VALUE:
    os << "null" << endl;
    break;
  }
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  simdjson::dom::parser parser;
  simdjson::dom::element elem;
  auto error = parser.parse(Data, Size).get(elem);

  if (error) { return 0; }
  NulOStream os;
  //std::ostream& os(std::cout);
  print_json(os,elem);
  return 0;
}
