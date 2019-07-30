#include <iostream>
#include <unistd.h>

#include "benchmark.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/jsonminifier.h"
#include "simdjson/jsonparser.h"

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "sajson.h"

using namespace simdjson;
using namespace rapidjson;

std::string rapid_stringme_insitu(char *json) {
  Document d;
  d.ParseInsitu(json);
  if (d.HasParseError()) {
    std::cerr << "problem!" << std::endl;
    return ""; // should do something
  }
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  return buffer.GetString();
}

std::string rapid_stringme(char *json) {
  Document d;
  d.Parse(json);
  if (d.HasParseError()) {
    std::cerr << "problem!" << std::endl;
    return ""; // should do something
  }
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  return buffer.GetString();
}

int main(int argc, char *argv[]) {
  int c;
  bool verbose = false;
  bool just_data = false;

  while ((c = getopt(argc, argv, "vt")) != -1)
    switch (c) {
    case 't':
      just_data = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  simdjson::padded_string p;
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if (verbose) {
    std::cout << "Input has ";
    if (p.size() > 1024 * 1024)
      std::cout << p.size() / (1024 * 1024) << " MB ";
    else if (p.size() > 1024)
      std::cout << p.size() / 1024 << " KB ";
    else
      std::cout << p.size() << " B ";
    std::cout << std::endl;
  }
  char *buffer = simdjson::allocate_padded_buffer(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';

  int repeat = 50;
  int volume = p.size();
  if (just_data) {
    printf(
        "name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  size_t strlength = rapid_stringme((char *)p.data()).size();
  if (verbose)
    std::cout << "input length is " << p.size() << " stringified length is "
              << strlength << std::endl;
  BEST_TIME_NOCHECK("despacing with RapidJSON",
                    rapid_stringme((char *)p.data()), , repeat, volume,
                    !just_data);
  BEST_TIME_NOCHECK(
      "despacing with RapidJSON Insitu", rapid_stringme_insitu((char *)buffer),
      memcpy(buffer, p.data(), p.size()), repeat, volume, !just_data);
  memcpy(buffer, p.data(), p.size());

  size_t outlength = simdjson::json_minify((const uint8_t *)buffer, p.size(),
                                           (uint8_t *)buffer);
  if (verbose)
    std::cout << "json_minify length is " << outlength << std::endl;

  uint8_t *cbuffer = (uint8_t *)buffer;
  BEST_TIME("json_minify", simdjson::json_minify(cbuffer, p.size(), cbuffer),
            outlength, memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);
  printf("minisize = %zu, original size = %zu  (minified down to %.2f percent "
         "of original) \n",
         outlength, p.size(), outlength * 100.0 / p.size());

  /***
   * Is it worth it to minify before parsing?
   ***/
  rapidjson::Document d;
  BEST_TIME("RapidJSON Insitu orig", d.ParseInsitu(buffer).HasParseError(),
            false, memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);

  char *mini_buffer = simdjson::allocate_padded_buffer(p.size() + 1);
  size_t minisize = simdjson::json_minify((const uint8_t *)p.data(), p.size(),
                                          (uint8_t *)mini_buffer);
  mini_buffer[minisize] = '\0';

  BEST_TIME("RapidJSON Insitu despaced", d.ParseInsitu(buffer).HasParseError(),
            false, memcpy(buffer, mini_buffer, p.size()), repeat, volume,
            !just_data);

  size_t ast_buffer_size = p.size() * 2;
  size_t *ast_buffer = (size_t *)malloc(ast_buffer_size * sizeof(size_t));

  BEST_TIME(
      "sajson orig",
      sajson::parse(sajson::bounded_allocation(ast_buffer, ast_buffer_size),
                    sajson::mutable_string_view(p.size(), buffer))
          .is_valid(),
      true, memcpy(buffer, p.data(), p.size()), repeat, volume, !just_data);

  BEST_TIME(
      "sajson despaced",
      sajson::parse(sajson::bounded_allocation(ast_buffer, ast_buffer_size),
                    sajson::mutable_string_view(minisize, buffer))
          .is_valid(),
      true, memcpy(buffer, mini_buffer, p.size()), repeat, volume, !just_data);

  simdjson::ParsedJson pj;
  bool is_alloc_ok = pj.allocate_capacity(p.size(), 1024);
  if (!is_alloc_ok) {
    fprintf(stderr, "failed to allocate memory\n");
    return EXIT_FAILURE;
  }
  bool automated_reallocation = false;
  BEST_TIME("simdjson orig",
            simdjson::json_parse((const uint8_t *)buffer, p.size(), pj,
                                 automated_reallocation),
            true, memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);

  simdjson::ParsedJson pj2;
  bool is_alloc_ok2 = pj2.allocate_capacity(p.size(), 1024);
  if (!is_alloc_ok2) {
    fprintf(stderr, "failed to allocate memory\n");
    return EXIT_FAILURE;
  }
  automated_reallocation = false;
  BEST_TIME("simdjson despaced",
            simdjson::json_parse((const uint8_t *)buffer, minisize, pj2,
                                 automated_reallocation),
            true, memcpy(buffer, mini_buffer, p.size()), repeat, volume,
            !just_data);
  free(buffer);
  free(ast_buffer);
  free(mini_buffer);
}
