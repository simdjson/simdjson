#include "simdjson.h"
#include <benchmark/benchmark.h>
#include <chrono>
#include <ratio>

simdjson::dom::parser parser;
simdjson::dom::element parsed_json;

simdjson::simdjson_result<std::vector<simdjson::dom::element>> result;

static void DoSetup(const benchmark::State &state) {
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

  parsed_json = parser.parse(json_string);
}

void wildcard_dot_top_level_elements() {
  // selects all the top level elements
  auto result = parsed_json.at_path_with_wildcard("$.*");
}

void wildcard_bracket_top_level_elements() {
  // selects all the top level elements
  auto result = parsed_json.at_path_with_wildcard("$[*]");
}

void wildcard_dot_element_properties_address() {
  // selects all address properties - $.address.*
  auto result = parsed_json.at_path_with_wildcard("$.address.*");
}

void wildcard_bracket_element_properties_address() {
  // selects all address properties - $["address"].*
  auto result = parsed_json.at_path_with_wildcard("$['address'].*");
}
void wildcard_dot_element_properties_phoneNumbers() {
  // selects all phoneNumbers properties - $.phoneNumbers.*
  auto result = parsed_json.at_path_with_wildcard("$.phoneNumbers.*");
}

void wildcard_bracket_element_nested_properties_streetAddress() {
  // selects nested properties - $.*.streetAddress
  auto result = parsed_json.at_path_with_wildcard("$.*.streetAddress");
}

static void BM_json_path_with_wildcard(benchmark::State &state, Func func) {
  int microseconds = state.range(0);

  std::chrono::duration<double, std::micro> sleep_duration{
      static_cast<double>(microseconds)};

  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    func();
    std::vector<simdjson::dom::element> values = result.value();

    std::string string_result;
    string_result = "[";
    for (int i = 0; i < values.size(); ++i) {
      simdjson::internal::string_builder<> sb;
      sb.append(values[i]);
      string_result = string_result +=
          std::string(i == 0 ? "" : ",") + "\n\t" + std::string(sb.str());
    }
    string_result += "\n]";
    std::cout << string_result << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_dot_top_level_elements_test, &wildcard_dot_top_level_elements);

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_bracket_top_level_elements_test, &wildcard_bracket_top_level_elements);

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_dot_element_properties_address_test, &wildcard_dot_element_properties_address);

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_bracket_element_properties_address_test, &wildcard_bracket_element_properties_address);

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_dot_element_properties_phoneNumbers_test, &wildcard_dot_element_properties_phoneNumbers);

BENCHMARK_CAPTURE(BM_json_path_with_wildcard, wildcard_bracket_element_nested_properties_streetAddress_test, &wildcard_bracket_element_nested_properties_streetAddress);

BENCHMARK_MAIN();
}
