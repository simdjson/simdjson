#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "NullBuffer.h"

// example from doc/basics.md#tree-walking-and-json-element-types
static void print_json(std::ostream& os, simdjson::dom::element element) {
  const char endl='\n';
  switch (element.type()) {
  case simdjson::dom::element_type::ARRAY:
    os << "[";
    for (simdjson::dom::element child : element.get<simdjson::dom::array>().first) {
      print_json(os, child);
      os << ",";
    }
    os << "]";
    break;
  case simdjson::dom::element_type::OBJECT:
    os << "{";
    for (simdjson::dom::key_value_pair field : element.get<simdjson::dom::object>().first) {
      os << "\"" << field.key << "\": ";
      print_json(os, field.value);
    }
    os << "}";
    break;
  case simdjson::dom::element_type::INT64:
    os << element.get<int64_t>().first << endl;
    break;
  case simdjson::dom::element_type::UINT64:
    os << element.get<uint64_t>().first << endl;
    break;
  case simdjson::dom::element_type::DOUBLE:
    os << element.get<double>().first << endl;
    break;
  case simdjson::dom::element_type::STRING:
    os << element.get<std::string_view>().first << endl;
    break;
  case simdjson::dom::element_type::BOOL:
    os << element.get<bool>().first << endl;
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
