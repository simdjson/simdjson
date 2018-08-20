//#include "avxprocessing.h"

//#include "avxminifier.h"
//#include "scalarminifier.h"
#include <iostream>

#include "benchmark.h"
#include "jsonioutil.h"
#include "jsonminifier.h"

#include "jsonminifier.h"
//#include "jsonstruct.h"
// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
//#include "util.h"

// colorfuldisplay(ParsedJson & pj, const u8 * buf)

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

  printf("parsing with RapidJSON before despacing:\n");
  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  printf("parsing with RapidJSON after despacing:\n");

  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            cbuffer[jsonminify((const uint8_t *)p.first, p.second, cbuffer)] =
                '\0',
            repeat, volume, true);
  free(buffer);
  free(p.first);
}
