#include <unistd.h>
#include "jsonparser/jsonparser.h"

#include "benchmark.h"

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "json11.cpp"
#include "sajson.h"
#include "fastjson.cpp"
#include "fastjson_dom.cpp"
#include "gason.cpp"
extern "C"
{
#include "ultrajsondec.c"
#include "ujdecode.h"

}
using namespace rapidjson;
using namespace std;


// fastjson has a tricky interface
void on_json_error( void *, const fastjson::ErrorContext& ec) {
  //std::cerr<<"ERROR: "<<ec.mesg<<std::endl;
}
bool fastjson_parse(const char *input) {
  fastjson::Token token;
  fastjson::dom::Chunk chunk;
  return fastjson::dom::parse_string(input, &token, &chunk, 0, &on_json_error, NULL);
}
// end of fastjson stuff

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool all = false; 
  int c;
  while ((c = getopt (argc, argv, "va")) != -1)
    switch (c)
      {
      case 'v':
        verbose = true;
        break;
      case 'a':
        all = true;
        break;
      default:
        abort ();
      }
  if (optind >= argc) {
    cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    cerr << "Or " << argv[0] << " -v <jsonfile>\n";
    cerr << "To enable parsers that are not standard compliant, use the -a flag\n";
    exit(1);
  }
  const char * filename = argv[optind];
  if(optind + 1 < argc) {
    cerr << "warning: ignoring everything after " << argv[optind  + 1] << endl;
  }
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
  ParsedJson *pj_ptr = allocate_ParsedJson(p.second, 1024);
  if (pj_ptr == NULL) {
    std::cerr << "can't allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  ParsedJson &pj(*pj_ptr);

  int repeat = 10;
  int volume = p.second;
  BEST_TIME("simdjson", json_parse(p.first, p.second, pj), true, , repeat, volume, true);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.second + 1);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';

  BEST_TIME("RapidJSON", 
      d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(),
      false, memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME("RapidJSON Insitu", d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  BEST_TIME("sajson (dynamic mem)", sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);

  size_t astbuffersize = p.second;
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  BEST_TIME("sajson (static alloc)", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);
  std::string json11err;
  if(all) BEST_TIME("dropbox (json11)     ",  (( json11::Json::parse(buffer,json11err).is_null() ) || ( ! json11err.empty() )), false, memcpy(buffer, p.first, p.second), repeat, volume, true);

  if(all) BEST_TIME("fastjson             ", fastjson_parse(buffer), true, memcpy(buffer, p.first, p.second), repeat, volume, true);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  if(all) BEST_TIME("gason             ", jsonParse(buffer, &endptr, &value, allocator), JSON_OK, memcpy(buffer, p.first, p.second), repeat, volume, true);
  void *state;
  if(all) BEST_TIME("ultrajson         ", (UJDecode(buffer, p.second, NULL, &state) == NULL), false, memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME("memcpy            ", (memcpy(buffer, p.first, p.second) == buffer), true, , repeat, volume, true);
  free(p.first);
  free(ast_buffer);
  deallocate_ParsedJson(pj_ptr);
}
