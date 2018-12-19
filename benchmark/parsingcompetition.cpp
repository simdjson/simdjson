#include "simdjson/jsonparser.h"
#include <unistd.h>

#include "benchmark.h"

// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "fastjson.cpp"
#include "fastjson_dom.cpp"
#include "gason.cpp"
#include "json11.cpp"
#include "sajson.h"
extern "C" {
#include "ujdecode.h"
#include "ultrajsondec.c"
}
using namespace rapidjson;
using namespace std;

// fastjson has a tricky interface
void on_json_error(void *, const fastjson::ErrorContext &ec) {
  // std::cerr<<"ERROR: "<<ec.mesg<<std::endl;
}
bool fastjson_parse(const char *input) {
  fastjson::Token token;
  fastjson::dom::Chunk chunk;
  return fastjson::dom::parse_string(input, &token, &chunk, 0, &on_json_error,
                                     NULL);
}
// end of fastjson stuff

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool justdata = false;
  bool all = false;
  int c;
  while ((c = getopt(argc, argv, "vat")) != -1)
    switch (c) {
    case 't':
      justdata = true;
      break;
    case 'v':
      verbose = true;
      break;
    case 'a':
      all = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    cerr << "Or " << argv[0] << " -v <jsonfile>\n";
    cerr << "To enable parsers that are not standard compliant, use the -a "
            "flag\n";
    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    cerr << "warning: ignoring everything after " << argv[optind + 1] << endl;
  }
  std::string_view p;
  try {
    p = get_corpus(filename);
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
  ParsedJson pj;
  bool allocok = pj.allocateCapacity(p.size(), 1024);

  if (!allocok) {
    std::cerr << "can't allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  int repeat = 10;
  int volume = p.size();
  if(!justdata) BEST_TIME("simdjson (dynamic mem) ", build_parsed_json(p).isValid(), true, ,
            repeat, volume, !justdata);
  // (static alloc) 
  BEST_TIME("simdjson ", json_parse(p, pj), true, , repeat,
            volume, !justdata);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';

  if(!justdata) BEST_TIME(
      "RapidJSON",
      d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(),
      false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  if(!justdata) BEST_TIME("RapidJSON (insitu)",
            d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError(),
            false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  typedef rapidjson::GenericDocument<UTF8<>, rapidjson::MemoryPoolAllocator<>,
                                     rapidjson::MemoryPoolAllocator<>>
      RapidDocumentType;
  size_t rapidvaallocsize = p.size() * 128; // allocate plenty of memory
  size_t rapidallocsize = p.size() * 4096;  // allocate plenty of memory
  char *rapidvalueBuffer = (char *)malloc(rapidvaallocsize);
  char *rapidparseBuffer = (char *)malloc(rapidallocsize);
  if ((rapidvalueBuffer != NULL) && (rapidvalueBuffer != NULL)) {
    rapidjson::MemoryPoolAllocator<> valueAllocator(rapidvalueBuffer,
                                                    rapidvaallocsize);
    rapidjson::MemoryPoolAllocator<> parseAllocator(rapidparseBuffer,
                                                    rapidallocsize);
    RapidDocumentType preallocedd(&valueAllocator, rapidvaallocsize,
                                  &parseAllocator);

    if(!justdata) BEST_TIME(
        "RapidJSON (static alloc)",
        preallocedd.Parse<kParseValidateEncodingFlag>((const char *)buffer)
            .HasParseError(),
        false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
    //  (static alloc, insitu)
    BEST_TIME("RapidJSON",
              preallocedd.ParseInsitu<kParseValidateEncodingFlag>(buffer)
                  .HasParseError(),
              false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
    assert(valueAllocator.Size() <= rapidvaallocsize);
    assert(parseAllocator.Size() <= rapidallocsize);
  }
  free(rapidvalueBuffer);
  free(rapidparseBuffer);
  if(!justdata) BEST_TIME("sajson (dynamic mem, insitu)",
            sajson::parse(sajson::dynamic_allocation(),
                          sajson::mutable_string_view(p.size(), buffer))
                .is_valid(),
            true, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);

  size_t astbuffersize = p.size();
  size_t *ast_buffer = (size_t *)malloc(astbuffersize * sizeof(size_t));
  //  (static alloc, insitu)
  BEST_TIME("sajson",
            sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize),
                          sajson::mutable_string_view(p.size(), buffer))
                .is_valid(),
            true, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  std::string json11err;
  if (all)
    BEST_TIME("dropbox (json11)     ",
              ((json11::Json::parse(buffer, json11err).is_null()) ||
               (!json11err.empty())),
              false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);

  if (all)
    BEST_TIME("fastjson             ", fastjson_parse(buffer), true,
              memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  if (all)
    BEST_TIME("gason             ",
              jsonParse(buffer, &endptr, &value, allocator), JSON_OK,
              memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  void *state;
  if (all)
    BEST_TIME("ultrajson         ",
              (UJDecode(buffer, p.size(), NULL, &state) == NULL), false,
              memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  if(!justdata) BEST_TIME("memcpy            ",
            (memcpy(buffer, p.data(), p.size()) == buffer), true, , repeat,
            volume, !justdata);
  free((void *)p.data());
  free(ast_buffer);
  free(buffer);
}
