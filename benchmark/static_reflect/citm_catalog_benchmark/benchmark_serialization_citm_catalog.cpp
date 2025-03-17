#include <cassert>
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>
#include "citm_catalog_data.h"
#include "nlohmann_citm_catalog_data.h"
#include "../benchmark_utils/benchmark_helper.h"

#if SIMDJSON_BENCH_CPP_REFLECT
#include <rfl.hpp>
#include <rfl/json.hpp>
void bench_reflect_cpp(CitmCatalog &data) {
  std::string output = rfl::json::write(data);
  size_t output_volume = output.size();
  printf("# output volume: %zu bytes\n", output_volume);

  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_reflect_cpp",
               bench([&data, &measured_volume, &output_volume]() {
                 std::string output = rfl::json::write(data);
                 measured_volume = output.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}
#endif // SIMDJSON_BENCH_CPP_REFLECT

#ifdef SIMDJSON_RUST_VERSION
#include "../serde-benchmark/serde_benchmark.h"

void bench_rust(serde_benchmark::CitmCatalog *data) {
  const char * output = serde_benchmark::str_from_citm(data);
  size_t output_volume = strlen(output);
  printf("# output volume: %zu bytes\n", output_volume);
  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_rust",
               bench([&data, &measured_volume, &output_volume]() {
                 const char * output = serde_benchmark::str_from_citm(data);
                 measured_volume = strlen(output);
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
                 // Note: not freeing the string to avoid affecting the timing
               }));
  // Free Rust-allocated string after benchmarking
  serde_benchmark::free_str(const_cast<char*>(output));
}
#endif // SIMDJSON_RUST_VERSION

void bench_nlohmann(CitmCatalog &data) {
  std::string output = nlohmann_serialize(data);
  size_t output_volume = output.size();
  printf("# output volume: %zu bytes\n", output_volume);

  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_nlohmann",
               bench([&data, &measured_volume, &output_volume]() {
                 std::string output = nlohmann_serialize(data);
                 measured_volume = output.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}

void bench_simdjson_static_reflection(CitmCatalog &data) {
  std::string output = simdjson::to_json(data);
  size_t output_volume = output.size();
  printf("# output volume: %zu bytes\n", output_volume);

  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_simdjson_static_reflection",
               bench([&data, &measured_volume, &output_volume]() {
                 std::string output = simdjson::to_json(data);
                 measured_volume = output.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}

std::string read_file(const std::string &file_path, size_t read_size = 65536) {
  std::ifstream stream(file_path, std::ios::binary);
  if(!stream) {
    std::cerr << "Could not open file '" << file_path << "'" << std::endl;
    exit(EXIT_FAILURE);
  }
  stream.exceptions(std::ios_base::badbit);
  std::string out;
  std::string buf(read_size, '\0');
  while (stream.read(&buf[0], read_size)) {
    out.append(buf, 0, size_t(stream.gcount()));
  }
  out.append(buf, 0, size_t(stream.gcount()));
  return out;
}

int main() {
  // Testing correctness of round-trip (serialization + deserialization)
  std::string json_str = read_file(JSON_FILE);

  // Loading up the data into a structure.
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  if(parser.iterate(simdjson::pad(json_str)).get(doc)) {
    std::cerr << "Error loading the document!" << std::endl;
    return EXIT_FAILURE;
  }
  CitmCatalog my_struct;
  if(doc.get<CitmCatalog>().get(my_struct)) {
    std::cerr << "Error loading CitmCatalog!" << std::endl;
    return EXIT_FAILURE;
  }

  // Benchmarking the serialization
  bench_nlohmann(my_struct);
  bench_simdjson_static_reflection(my_struct);
#ifdef SIMDJSON_RUST_VERSION
  printf("# WARNING: The Rust benchmark may not be directly comparable since it does not use an equivalent data structure.\n");
  serde_benchmark::CitmCatalog * cc = serde_benchmark::citm_from_str(json_str.c_str(), json_str.size());
  bench_rust(cc);
  serde_benchmark::free_citm(cc);
#endif
#if SIMDJSON_BENCH_CPP_REFLECT
  bench_reflect_cpp(my_struct);
#endif
  return EXIT_SUCCESS;
}