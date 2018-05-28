#include "avxprocessing.h"

#include "avxminifier.h"
#include "benchmark.h"
#include "jsonstruct.h"
// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "scalarprocessing.h"
#include "util.h"

// colorfuldisplay(ParsedJson & pj, const u8 * buf)

using namespace rapidjson;
using namespace std;

std::string rapidstringme(char * json) {
    Document d;
    d.Parse(json);
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
  ParsedJson pj;
  std::cout << "Input has ";
  if (p.second > 1024 * 1024)
    std::cout << p.second / (1024 * 1024) << " MB ";
  else if (p.second > 1024)
    std::cout << p.second / 1024 << " KB ";
  else
    std::cout << p.second << " B ";
  std::cout << std::endl;

  if (posix_memalign((void **)&pj.structurals, 8,
                     ROUNDUP_N(p.second, 64) / 8)) {
    throw "Allocation failed";
  };

  pj.n_structural_indexes = 0;
  // we have potentially 1 structure per byte of input
  // as well as a dummy structure and a root structure
  // we also potentially write up to 7 iterations beyond
  // in our 'cheesy flatten', so make some worst-case
  // sapce for that too
  u32 max_structures = ROUNDUP_N(p.second, 64) + 2 + 7;
  pj.structural_indexes = new u32[max_structures];
  pj.nodes = new JsonNode[max_structures];
  if (verbose) {
    std::cout << "Parsing SIMD (once) " << std::endl;
    avx_json_parse(p.first, p.second, pj);
    colorfuldisplay(pj, p.first);
    debugdisplay(pj, p.first);
    std::cout << "Parsing scalar (once) " << std::endl;
    scalar_json_parse(p.first, p.second, pj);
    colorfuldisplay(pj, p.first);
    debugdisplay(pj, p.first);
  }

  int repeat = 10;
  int volume = p.second;
  BEST_TIME_NOCHECK(avx_json_parse(p.first, p.second, pj), , repeat, volume,
                    true);
  BEST_TIME_NOCHECK(scalar_json_parse(p.first, p.second, pj), , repeat, volume,
                    true);

  rapidjson::Document d;

  char * buffer = (char *) malloc(p.second);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';

  BEST_TIME(d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(), false,
          memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME(d.Parse((const char *)buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);
  size_t strlength = rapidstringme((char*) p.first).size();
  std::cout << "input length is "<< p.second << " stringified length is " << strlength << std::endl;
  BEST_TIME_NOCHECK(rapidstringme((char*) p.first), , repeat, volume,
                    true);
  memcpy(buffer, p.first, p.second);
  size_t outlength = copy_without_useless_spaces((const uint8_t *)buffer, p.second,(uint8_t *) buffer);
  printf("these should match: %zu %zu \n", strlength, outlength);


  uint8_t * cbuffer = (uint8_t *)buffer;
  BEST_TIME(copy_without_useless_spaces(cbuffer, p.second,cbuffer), outlength,
            memcpy(buffer, p.first, p.second), repeat, volume, true);
  buffer[outlength] = '\0';

  free(buffer);
}
