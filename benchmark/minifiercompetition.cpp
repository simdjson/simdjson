#include <unistd.h>
#include <iostream>

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


using namespace rapidjson;

std::string rapidstringmeInsitu(char *json) {
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

std::string rapidstringme(char *json) {
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
  bool justdata = false;

  while ((c = getopt (argc, argv, "vt")) != -1)
    switch (c)
      {
      case 't':
        justdata = true;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        abort ();
      }
  if (optind >= argc) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char * filename = argv[optind];
  padded_string p;
  try {
    get_corpus(filename).swap(p);
  } catch (const std::exception& e) { // caught by reference to base
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
  char *buffer = allocate_padded_buffer(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';

  int repeat = 50;
  int volume = p.size();
  if(justdata) {
    printf("name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  size_t strlength = rapidstringme((char *)p.data()).size();
  if (verbose)
    std::cout << "input length is " << p.size() << " stringified length is "
              << strlength << std::endl;
  BEST_TIME_NOCHECK("despacing with RapidJSON", rapidstringme((char *)p.data()), , repeat, volume, !justdata);
  BEST_TIME_NOCHECK("despacing with RapidJSON Insitu", rapidstringmeInsitu((char *)buffer),
                    memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  memcpy(buffer, p.data(), p.size());

  size_t outlength =
      jsonminify((const uint8_t *)buffer, p.size(), (uint8_t *)buffer);
  if (verbose)
    std::cout << "jsonminify length is " << outlength << std::endl;

  uint8_t *cbuffer = (uint8_t *)buffer;
  BEST_TIME("jsonminify", jsonminify(cbuffer, p.size(), cbuffer), outlength,
            memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  printf("minisize = %zu, original size = %zu  (minified down to %.2f percent of original) \n", outlength, p.size(), outlength * 100.0 / p.size());

  /***
   * Is it worth it to minify before parsing?
   ***/
  rapidjson::Document d;
  BEST_TIME("RapidJSON Insitu orig", d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);

  char *minibuffer = allocate_padded_buffer(p.size() + 1);
  size_t minisize = jsonminify((const uint8_t *)p.data(), p.size(), (uint8_t*) minibuffer);
  minibuffer[minisize] = '\0';

  BEST_TIME("RapidJSON Insitu despaced", d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, minibuffer, p.size()),
            repeat, volume, !justdata);

  size_t astbuffersize = p.size() * 2;
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  BEST_TIME("sajson orig", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.size(), buffer)).is_valid(), true, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);


  BEST_TIME("sajson despaced", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(minisize, buffer)).is_valid(), true, memcpy(buffer, minibuffer, p.size()), repeat, volume, !justdata);

  ParsedJson pj;
  bool isallocok = pj.allocateCapacity(p.size(), 1024);
  if(!isallocok) {
    fprintf(stderr, "failed to allocate memory\n");
    return EXIT_FAILURE;
  } 
  bool automated_reallocation = false; 
  BEST_TIME("simdjson orig", json_parse((const uint8_t*)buffer, p.size(), pj, automated_reallocation), true, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  
  ParsedJson pj2;
  bool isallocok2 = pj2.allocateCapacity(p.size(), 1024);
  if(!isallocok2) {
    fprintf(stderr, "failed to allocate memory\n");
    return EXIT_FAILURE;
  } 
  automated_reallocation = false; 
  BEST_TIME("simdjson despaced", json_parse((const uint8_t*)buffer, minisize, pj2, automated_reallocation), true, memcpy(buffer, minibuffer, p.size()), repeat, volume, !justdata);
  free(buffer);
  free(ast_buffer);
  free(minibuffer);


}
