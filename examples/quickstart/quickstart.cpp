#include "simdjson.h"
#include <benchmark/benchmark.h>
#include <chrono>
#include <ratio>

simdjson::padded_string json_string = R"(
  {
    "firstName": "John",
    "lastName" : "doe",
    "age"      : 26,
    "address"  : {
      "streetAddress": "naist street",
      "city"         : "Nara",
      "postalCode"   : "630-0192"
    },
    "phoneNumbers": [
      {
        "type"  : "iPhone",
        "numbers": [
          "0123-4567-8888",
          "0123-4567-8788",
          "0123-4567-8887"
        ]
      },
      {
        "type"  : "home",
        "numbers": [
          "0123-4567-8910",
          "0123-4267-8910",
          "0103-4567-8910"
        ]
      }
    ]
  })"_padded;

simdjson::dom::parser parser{};
simdjson::dom::element parsed_json = parser.parse(json_string);

void print_result(std::vector<simdjson::dom::element> &values) {
  std::string string_result = "[";
  for (int i = 0; i < values.size(); ++i) {
    simdjson::internal::string_builder<> sb;
    sb.append(values[i]);
    string_result = string_result +=
        std::string(i == 0 ? "" : ",") + "\n\t" + std::string(sb.str());
  }
  string_result += "\n]";
  std::cout << string_result << std::endl;
}

void wildcard_dot_top_level_elements() {
  // selects all the top level elements

  std::cout << "Result for $.*" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$.*");
  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_bracket_top_level_elements() {
  // selects all the top level elements

  std::cout << "Result for $[*]" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$[*]");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_dot_element_properties_address() {
  // selects all address properties - $.address.*
  std::cout << "Result for $.address.*" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$.address.*");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_bracket_element_properties_address() {
  // selects all address properties - $["address"].*
  std::cout << "Result for $['address'].*" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$['address'].*");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_bracket_element_properties_address_bracket() {
  // selects all address properties - $["address"].*
  std::cout << "Result for $['address'][*]" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$['address'][*]");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_dot_element_properties_phoneNumbers() {
  // selects all phoneNumbers properties - $.phoneNumbers.*
  std::cout << "Result for $.phoneNumbers.*" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$.phoneNumbers.*");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

void wildcard_bracket_element_nested_properties_streetAddress() {
  // selects nested properties - $.*.streetAddress
  std::cout << "Result for $.*.streetAddress" << "\n";
  auto result = parsed_json.at_path_with_wildcard("$.*.streetAddress");

  std::vector<simdjson::dom::element> values = result.value();
  print_result(values);
}

int main(int argc, char **argv) {
  wildcard_dot_top_level_elements();
  wildcard_bracket_top_level_elements();
  wildcard_dot_element_properties_address();
  wildcard_bracket_element_properties_address();
  wildcard_bracket_element_properties_address_bracket();
  wildcard_dot_element_properties_phoneNumbers();
  wildcard_bracket_element_nested_properties_streetAddress();
  return 0;
}
