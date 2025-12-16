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
#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson_twitter_data.h"
#endif
#if SIMDJSON_BENCH_CPP_REFLECT
#include <rfl.hpp>
#include <rfl/json.hpp>
void bench_reflect_cpp(TwitterData &data) {
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


void bench_rust(serde_benchmark::TwitterData *data) {
  const char * output = serde_benchmark::str_from_twitter(data);
  size_t output_volume = strlen(output);
  printf("# output volume: %zu bytes\n", output_volume);
  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_rust",
               bench([&data, &measured_volume, &output_volume]() {
                 const char * output = serde_benchmark::str_from_twitter(data);
                 serde_benchmark::free_string(output);
               }));
}
#endif

template <class T> void bench_simdjson_static_reflection(T &data) {
  simdjson::builder::string_builder sb;
  simdjson::builder::append(sb, data);
  std::string_view p;
  if(sb.view().get(p)) {
    std::cerr << "Error!" << std::endl;
  }
  size_t output_volume = p.size();
  sb.clear();
  printf("# output volume: %zu bytes\n", output_volume);

  volatile size_t measured_volume = 0;
  pretty_print(sizeof(data), output_volume, "bench_simdjson_static_reflection",
               bench([&data, &measured_volume, &output_volume, &sb]() {
                 sb.clear();
                 simdjson::builder::append(sb, data);
                 std::string_view p;
                 if(sb.view().get(p)) {
                   std::cerr << "Error!" << std::endl;
                 }
                 measured_volume = sb.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}

#if SIMDJSON_STATIC_REFLECTION
template <class T> void bench_simdjson_to(T &data) {
  // First run to determine size
  std::string output;
  simdjson::builder::to_json(data, output);
  size_t output_volume = output.size();
  printf("# output volume: %zu bytes\n", output_volume);

  // Pre-allocate string with sufficient capacity to avoid reallocation
  output.reserve(output_volume * 2);  // Reserve extra space to avoid any reallocation

  volatile size_t measured_volume = 0;
  pretty_print(sizeof(data), output_volume, "bench_simdjson_to",
               bench([&data, &measured_volume, &output_volume, &output]() {
                 // Reuse the pre-allocated string - avoids allocation
                 simdjson::builder::to_json(data, output);
                 measured_volume = output.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}
#endif

void bench_nlohmann(TwitterData &data) {
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

#ifdef SIMDJSON_COMPETITION_YYJSON
void bench_yyjson(TwitterData &data) {
  std::string output = yyjson_serialize(data);
  size_t output_volume = output.size();
  printf("# output volume: %zu bytes\n", output_volume);

  volatile size_t measured_volume = 0;
  pretty_print(1, output_volume, "bench_yyjson",
               bench([&data, &measured_volume, &output_volume]() {
                 std::string output = yyjson_serialize(data);
                 measured_volume = output.size();
                 if (measured_volume != output_volume) {
                   printf("mismatch\n");
                 }
               }));
}
#endif

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

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
  // Testing correctness of round-trip (serialization + deserialization)
  std::string json_str = read_file(JSON_FILE);

  // Loading up the data into a structure.
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  if(parser.iterate(simdjson::pad(json_str)).get(doc)) {
    std::cerr << "Error loading the document!" << std::endl;
    return EXIT_FAILURE;
  }
  TwitterData my_struct;
  if(doc.get<TwitterData>().get(my_struct)) {
    std::cerr << "Error loading TwitterData!" << std::endl;
    return EXIT_FAILURE;
  }

  // Benchmarking the serialization
  if (matches_filter("nlohmann", filter)) {
    bench_nlohmann(my_struct);
  }
#ifdef SIMDJSON_COMPETITION_YYJSON
  if (matches_filter("yyjson", filter)) {
    bench_yyjson(my_struct);
  }
#endif
  if (matches_filter("simdjson_static_reflection", filter)) {
    bench_simdjson_static_reflection(my_struct);
  }
#if SIMDJSON_STATIC_REFLECTION
  if (matches_filter("simdjson_to", filter)) {
    bench_simdjson_to(my_struct);
  }
#endif
#ifdef SIMDJSON_RUST_VERSION
  if (matches_filter("rust", filter)) {
    printf("# Note: Rust/Serde structures updated to closely match C++ (indices field remains as array).\n");
    serde_benchmark::TwitterData * td = serde_benchmark::twitter_from_str(json_str.c_str(), json_str.size());
    bench_rust(td);
    serde_benchmark::free_twitter(td);
  }
#endif
#if SIMDJSON_BENCH_CPP_REFLECT
  if (matches_filter("reflect_cpp", filter)) {
    bench_reflect_cpp(my_struct);
  }
#endif
  return EXIT_SUCCESS;
}