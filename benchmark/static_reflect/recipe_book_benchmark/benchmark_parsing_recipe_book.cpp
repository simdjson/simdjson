#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <simdjson.h>
#include <sstream>
#include <string>
#include "recipe_book_data.h"
#include "../benchmark_utils/benchmark_helper.h"
#ifdef SIMDJSON_COMPETITION_GLAZE
#include <glaze/glaze.hpp>
#endif

void bench_simdjson_static_reflection_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);
  simdjson::padded_string padded(json_str);
  simdjson::ondemand::parser parser;
  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_static_reflection_parsing",
               bench([&]() {
                 simdjson::ondemand::document doc;
                 if (parser.iterate(padded).get(doc)) {
                   result = false;
                   return;
                 }
                 RecipeBook book;
                 if (doc.get<RecipeBook>().get(book)) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}

#if SIMDJSON_STATIC_REFLECTION
void bench_simdjson_from_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);
  simdjson::padded_string padded(json_str);
  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_from_parsing", bench([&]() {
                 RecipeBook book;
                 auto err = simdjson::from(padded).get(book);
                 if (err) {
                   result = false;
                   printf("parse error: %s\n", simdjson::error_message(err));
                 }
               }));
}
#endif

#ifdef SIMDJSON_COMPETITION_GLAZE
void bench_glaze_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);
  volatile bool result = true;
  pretty_print(1, input_volume, "bench_glaze_parsing", bench([&]() {
                 RecipeBook book;
                 auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(
                     book, json_str);
                 if (ec) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}
#endif

static std::string read_file(const std::string &filename) {
  printf("# Reading file %s\n", filename.c_str());
  std::ifstream stream(filename, std::ios::binary);
  if (!stream) {
    std::cerr << "Cannot open " << filename << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::stringstream ss;
  ss << stream.rdbuf();
  return ss.str();
}

// Checks whether the benchmark name matches any of the comma-separated filters
static bool matches_filter(const std::string &name, const std::string &filter) {
  if (filter.empty()) return true;
  size_t start = 0;
  while (true) {
    size_t end = filter.find(',', start);
    std::string token = filter.substr(start, end - start);
    if (name.find(token) != std::string::npos) return true;
    if (end == std::string::npos) return false;
    start = end + 1;
  }
}

int main(int argc, char *argv[]) {
  std::string filter;
  for (int i = 1; i < argc; ++i) {
    if ((!strcmp(argv[i], "-f") || !strcmp(argv[i], "--filter")) && i + 1 < argc) {
      filter = argv[++i];
    } else {
      std::cerr << "usage: " << argv[0] << " [-f <filter>]\n";
      return EXIT_FAILURE;
    }
  }

  std::string json_str = read_file(JSON_FILE);

  // Fail early if simdjson cannot read the file into the structure.
  {
    simdjson::padded_string padded(json_str);
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    RecipeBook book;
    if (parser.iterate(padded).get(doc) || doc.get<RecipeBook>().get(book)) {
      std::cerr << "Error loading the recipe book!" << std::endl;
      return EXIT_FAILURE;
    }
  }

#ifdef SIMDJSON_COMPETITION_GLAZE
  if (matches_filter("glaze", filter)) {
    bench_glaze_parsing(json_str);
  }
#endif
  if (matches_filter("simdjson_static_reflection", filter)) {
    bench_simdjson_static_reflection_parsing(json_str);
  }
#if SIMDJSON_STATIC_REFLECTION
  if (matches_filter("simdjson_from", filter)) {
    bench_simdjson_from_parsing(json_str);
  }
#endif
  return EXIT_SUCCESS;
}
