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

void BM_wildcard_dot_top_level_elements(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all the top level elements
    auto result = parsed_json.at_path_with_wildcard("$.*");
    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_dot_top_level_elements)->UseManualTime();

void BM_wildcard_bracket_top_level_elements(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all the top level elements
    auto result = parsed_json.at_path_with_wildcard("$[*]");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_bracket_top_level_elements)->UseManualTime();

void BM_wildcard_dot_element_properties_address(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all address properties - $.address.*
    auto result = parsed_json.at_path_with_wildcard("$.address.*");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_dot_element_properties_address)->UseManualTime();

void BM_wildcard_bracket_element_properties_address(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all address properties - $["address"].*
    auto result = parsed_json.at_path_with_wildcard("$['address'].*");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_bracket_element_properties_address)->UseManualTime();

void BM_wildcard_bracket_element_properties_address_bracket(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all address properties - $["address"][*]
    auto result = parsed_json.at_path_with_wildcard("$['address'][*]");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_bracket_element_properties_address_bracket)->UseManualTime();

void BM_wildcard_dot_element_properties_phoneNumbers(benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects all phoneNumbers properties - $.phoneNumbers.*
    auto result = parsed_json.at_path_with_wildcard("$.phoneNumbers.*");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_dot_element_properties_phoneNumbers)->UseManualTime();

void BM_wildcard_bracket_element_nested_properties_streetAddress(
    benchmark::State &state) {
  for (auto _ : state) {
    auto start = std::chrono::high_resolution_clock::now();

    // selects nested properties - $.*.streetAddress
    auto result = parsed_json.at_path_with_wildcard("$.*.streetAddress");

    std::vector<simdjson::dom::element> values = result.value();

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(BM_wildcard_bracket_element_nested_properties_streetAddress)
    ->UseManualTime();

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

BENCHMARK_MAIN();
