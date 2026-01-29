#include <cassert>
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>
#include "twitter_data.h"
#include "nlohmann_twitter_data.h"
#include "../benchmark_utils/benchmark_helper.h"
#ifdef SIMDJSON_COMPETITION_RAPIDJSON
#include "rapidjson_twitter_data.h"
#endif
#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson_twitter_data.h"
#endif

#ifdef SIMDJSON_RUST_VERSION
#include "../serde-benchmark/serde_benchmark.h"

void bench_rust_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_rust_parsing",
               bench([&json_str, &result]() {
                 serde_benchmark::TwitterData *td = serde_benchmark::twitter_from_str(json_str.c_str(), json_str.size());
                 result = (td != nullptr);
                 if (td) {
                   serde_benchmark::free_twitter(td);
                 }
                 if (!result) {
                   printf("parse error\n");
                 }
               }));
}
#endif

// OPTIMIZED VERSION: Reuses parser across iterations
template <class T>
void bench_simdjson_static_reflection_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  // Pre-allocate padded buffer outside the benchmark loop
  simdjson::padded_string padded = simdjson::padded_string(json_str);

  // CRITICAL: Create parser OUTSIDE the loop for reuse
  simdjson::ondemand::parser parser;

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_static_reflection_parsing",
               bench([&padded, &result, &parser]() {
                 // Reuse the same parser instance
                 simdjson::ondemand::document doc;
                 if(parser.iterate(padded).get(doc)) {
                   result = false;
                   return;
                 }
                 T my_struct;
                 if(doc.get<T>().get(my_struct)) {
                   result = false;
                 }
                 if (!result) {
                   printf("parse error\n");
                 }
               }));
}

#if SIMDJSON_STATIC_REFLECTION
template <class T>
void bench_simdjson_from_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  // Pre-allocate padded buffer outside the benchmark loop
  simdjson::padded_string padded = simdjson::padded_string(json_str);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_from_parsing",
               bench([&padded, &result]() {
                 try {
                   // Using simdjson::from API directly with padded string
                   // This will throw an exception if parsing fails
                   T my_struct = simdjson::from(padded);
                   result = true;
                 } catch (const std::exception& e) {
                   result = false;
                   printf("parse error: %s\n", e.what());
                 }
               }));
}
#endif

// Nlohmann parsing disabled - deserialization functions not implemented
// void bench_nlohmann_parsing(const std::string &json_str) {
//   size_t input_volume = json_str.size();
//   printf("# input volume: %zu bytes\n", input_volume);
//
//   volatile bool result = true;
//   pretty_print(1, input_volume, "bench_nlohmann_parsing",
//                bench([&json_str, &result]() {
//                  try {
//                    TwitterData data = nlohmann_deserialize(json_str);
//                    result = true;
//                  } catch (...) {
//                    result = false;
//                    printf("parse error\n");
//                  }
//                }));
// }

#ifdef SIMDJSON_COMPETITION_RAPIDJSON
void bench_rapidjson_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_rapidjson_parsing",
               bench([&json_str, &result]() {
                 try {
                   TwitterData data = rapidjson_deserialize(json_str);
                   result = true;
                 } catch (...) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}
#endif

#ifdef SIMDJSON_COMPETITION_YYJSON
void bench_yyjson_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_yyjson_parsing",
               bench([&json_str, &result]() {
                 try {
                   TwitterData data = yyjson_deserialize(json_str);
                   result = true;
                 } catch (...) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}
#endif

std::string read_file(std::string filename) {
  printf("# Reading file %s\n", filename.c_str());
  constexpr size_t read_size = 4096;
  auto stream = std::ifstream(filename.c_str());
  stream.exceptions(std::ios_base::badbit);
  std::string out;
  std::string buf(read_size, '\0');
  while (stream.read(&buf[0], read_size)) {
    out.append(buf, 0, size_t(stream.gcount()));
  }
  out.append(buf, 0, size_t(stream.gcount()));
  return out;
}

// Function to check if benchmark name matches any of the comma-separated filters
bool matches_filter(const std::string& benchmark_name, const std::string& filter) {
  if (filter.empty()) return true;

  // Split filter by comma
  size_t start = 0;
  size_t end = filter.find(',');
  while (end != std::string::npos) {
    std::string token = filter.substr(start, end - start);
    if (benchmark_name.find(token) != std::string::npos) {
      return true;
    }
    start = end + 1;
    end = filter.find(',', start);
  }
  // Check last token
  std::string token = filter.substr(start);
  return benchmark_name.find(token) != std::string::npos;
}

int main(int argc, char* argv[]) {
  std::string filter;

  // Parse command-line arguments
  for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) {
          if (i + 1 < argc) {
              filter = argv[++i];
          } else {
              std::cerr << "Error: -f/--filter requires an argument" << std::endl;
              return EXIT_FAILURE;
          }
      }
  }

  // Load the JSON data
  std::string json_str = read_file(JSON_FILE);

  // Benchmarking the parsing
  // Nlohmann parsing disabled - deserialization functions not implemented
  // if (matches_filter("nlohmann", filter)) {
  //   bench_nlohmann_parsing(json_str);
  // }
#ifdef SIMDJSON_COMPETITION_RAPIDJSON
  if (matches_filter("rapidjson", filter)) {
    bench_rapidjson_parsing(json_str);
  }
#endif
#ifdef SIMDJSON_COMPETITION_YYJSON
  if (matches_filter("yyjson", filter)) {
    bench_yyjson_parsing(json_str);
  }
#endif
  if (matches_filter("simdjson_static_reflection", filter)) {
    bench_simdjson_static_reflection_parsing<TwitterData>(json_str);
  }
#if SIMDJSON_STATIC_REFLECTION
  if (matches_filter("simdjson_from", filter)) {
    bench_simdjson_from_parsing<TwitterData>(json_str);
  }
#endif
#ifdef SIMDJSON_RUST_VERSION
  if (matches_filter("rust", filter)) {
    printf("# Note: Rust/Serde parsing test\n");
    bench_rust_parsing(json_str);
  }
#endif
  return EXIT_SUCCESS;
}