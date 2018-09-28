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
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    cerr << "Or " << argv[0] << " -v <jsonfile>\n";
    exit(1);
  }
  bool verbose = false;
  if (argc > 2) {
    if (strcmp(argv[1], "-v"))
      verbose = true;
  }
  pair<u8 *, size_t> p = get_corpus(argv[argc - 1]);
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
  char *buffer = (char *)malloc(p.second + 1);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';

  int repeat = 10;
  int volume = p.second;

  size_t strlength = rapidstringme((char *)p.first).size();
  if (verbose)
    std::cout << "input length is " << p.second << " stringified length is "
              << strlength << std::endl;
  BEST_TIME_NOCHECK(rapidstringme((char *)p.first), , repeat, volume, true);
  BEST_TIME_NOCHECK(rapidstringmeInsitu((char *)buffer),
                    memcpy(buffer, p.first, p.second), repeat, volume, true);
  memcpy(buffer, p.first, p.second);

  size_t outlength =
      jsonminify((const uint8_t *)buffer, p.second, (uint8_t *)buffer);
  if (verbose)
    std::cout << "jsonminify length is " << outlength << std::endl;

  uint8_t *cbuffer = (uint8_t *)buffer;
  BEST_TIME(jsonminify(cbuffer, p.second, cbuffer), outlength,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  /***
   * Is it worth it to minify before parsing?
   ***/
  rapidjson::Document d;
  printf("\n");
  printf("parsing with RapidJSON before despacing:\n");
  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  printf("parsing with RapidJSON after despacing:\n");
  char *minibuffer = (char *)malloc(p.second + 1);
  size_t minisize = jsonminify((const uint8_t *)p.first, p.second, (uint8_t*) minibuffer);
  minibuffer[minisize] = '\0';
  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, minibuffer, p.second),
            repeat, volume, true);

  printf("\n");
  size_t astbuffersize = p.second * 2;
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  printf("parsing with sajson before despacing:\n");
  BEST_TIME(sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);




  printf("parsing with sajson after despacing:\n");
  BEST_TIME(sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(minisize, buffer)).is_valid(), true, memcpy(buffer, minibuffer, p.second), repeat, volume, true);

  printf("parsing before despacing:\n");
  ParsedJson *pj_ptr = allocate_ParsedJson(p.second);
  ParsedJson &pj(*pj_ptr);
  BEST_TIME(json_parse(p.first, p.second, pj), true, , repeat, volume, true);
  
  printf("parsing after despacing:\n");
  ParsedJson *pj_ptr2 = allocate_ParsedJson(minisize);
  ParsedJson &pj2(*pj_ptr2);

  BEST_TIME(json_parse((const u8*)minibuffer, minisize, pj2), true, , repeat, volume, true);

  free(buffer);
  free(p.first);
  free(ast_buffer);
  free(minibuffer);
  deallocate_ParsedJson(pj_ptr);
  deallocate_ParsedJson(pj_ptr2);


}
