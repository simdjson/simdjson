#include <unistd.h>
#include <iostream>

#include "benchmark.h"
#include "jsonparser/jsonioutil.h"
#include "jsonparser/jsonminifier.h"
#include "jsonparser/jsonparser.h"

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "sajson.h"


using namespace rapidjson;
using namespace std;

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
  while ((c = getopt (argc, argv, "v")) != -1)
    switch (c)
      {
      case 'v':
        verbose = true;
        break;
      default:
        abort ();
      }
  if (optind >= argc) {
    cerr << "Usage: " << argv[0] << " <jsonfile>" << endl;
    exit(1);
  }
  const char * filename = argv[optind];
  pair<u8 *, size_t> p;
  try {
    p = get_corpus(filename);
  } catch (const std::exception& e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if (verbose) {
    std::cout << "Input has ";
    if (p.second > 1024 * 1024)
      std::cout << p.second / (1024 * 1024) << " MB ";
    else if (p.second > 1024)
      std::cout << p.second / 1024 << " KB ";
    else
      std::cout << p.second << " B ";
    std::cout << std::endl;
  }
  char *buffer = allocate_aligned_buffer(p.second + 1);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';

  int repeat = 10;
  int volume = p.second;

  size_t strlength = rapidstringme((char *)p.first).size();
  if (verbose)
    std::cout << "input length is " << p.second << " stringified length is "
              << strlength << std::endl;
  BEST_TIME_NOCHECK("despacing with RapidJSON", rapidstringme((char *)p.first), , repeat, volume, true);
  BEST_TIME_NOCHECK("despacing with RapidJSON Insitu", rapidstringmeInsitu((char *)buffer),
                    memcpy(buffer, p.first, p.second), repeat, volume, true);
  memcpy(buffer, p.first, p.second);

  size_t outlength =
      jsonminify((const uint8_t *)buffer, p.second, (uint8_t *)buffer);
  if (verbose)
    std::cout << "jsonminify length is " << outlength << std::endl;

  uint8_t *cbuffer = (uint8_t *)buffer;
  BEST_TIME("jsonminify", jsonminify(cbuffer, p.second, cbuffer), outlength,
            memcpy(buffer, p.first, p.second), repeat, volume, true);
  printf("minisize = %zu, original size = %zu  (minified down to %.2f percent of original) \n", outlength, p.second, outlength * 100.0 / p.second);

  /***
   * Is it worth it to minify before parsing?
   ***/
  rapidjson::Document d;
  BEST_TIME("RapidJSON Insitu orig", d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  char *minibuffer = allocate_aligned_buffer(p.second + 1);
  size_t minisize = jsonminify((const uint8_t *)p.first, p.second, (uint8_t*) minibuffer);
  minibuffer[minisize] = '\0';

  BEST_TIME("RapidJSON Insitu despaced", d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, minibuffer, p.second),
            repeat, volume, true);

  size_t astbuffersize = p.second * 2;
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  BEST_TIME("sajson orig", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);


  BEST_TIME("sajson despaced", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(minisize, buffer)).is_valid(), true, memcpy(buffer, minibuffer, p.second), repeat, volume, true);

  ParsedJson *pj_ptr = allocate_ParsedJson(p.second, 1024);
  ParsedJson &pj(*pj_ptr);
  BEST_TIME("json_parse orig", json_parse((const u8*)buffer, p.second, pj), true, memcpy(buffer, p.first, p.second), repeat, volume, true);
  
  ParsedJson *pj_ptr2 = allocate_ParsedJson(p.second, 1024);
  ParsedJson &pj2(*pj_ptr2);


  BEST_TIME("json_parse despaced", json_parse((const u8*)buffer, minisize, pj2), true, memcpy(buffer, minibuffer, p.second), repeat, volume, true);

  free(buffer);
  free(p.first);
  free(ast_buffer);
  free(minibuffer);
  deallocate_ParsedJson(pj_ptr);
  deallocate_ParsedJson(pj_ptr2);


}
