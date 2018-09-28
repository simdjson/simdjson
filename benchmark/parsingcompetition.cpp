
#include "jsonparser/jsonparser.h"

#include "benchmark.h"

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"




using namespace rapidjson;
using namespace std;

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
  ParsedJson *pj_ptr = allocate_ParsedJson(p.second);
  if (pj_ptr == NULL) {
    std::cerr << "can't allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  ParsedJson &pj(*pj_ptr);

  int repeat = 10;
  int volume = p.second;
  BEST_TIME(json_parse(p.first, p.second, pj), true, , repeat, volume, true);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.second + 1);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';

  BEST_TIME(
      d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(),
      false, memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME(d.Parse((const char *)buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);
  BEST_TIME(d.ParseInsitu(buffer).HasParseError(), false,
            memcpy(buffer, p.first, p.second), repeat, volume, true);

  BEST_TIME(sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);

  size_t astbuffersize = p.second;
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  BEST_TIME(sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.second, buffer)).is_valid(), true, memcpy(buffer, p.first, p.second), repeat, volume, true);

  free(buffer);
  free(p.first);
  free(ast_buffer);
  deallocate_ParsedJson(pj_ptr);
}
