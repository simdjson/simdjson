#include "simdjson/jsonparser.h"
#ifndef _MSC_VER
#include <unistd.h>
#include "linux-perf-events.h"
#ifdef __linux__
#include <libgen.h>
#endif //__linux__
#endif // _MSC_VER

#include "benchmark.h"


// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

#ifdef ALLPARSER
#include "fastjson.cpp"
#include "fastjson_dom.cpp"
#include "gason.cpp"
#include "json11.cpp"
extern "C" {
#include "ujdecode.h"
#include "ultrajsondec.c"
}
#endif 

using namespace rapidjson;
using namespace std;


#ifdef ALLPARSER
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
#endif

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool justdata = false;
  int c;
  while ((c = getopt(argc, argv, "vt")) != -1)
    switch (c) {
    case 't':
      justdata = true;
      break;
    case 'v':
      verbose = true;
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
  int repeat = 50;
  int volume = p.size();
  if(justdata) {
    printf("name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
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
      "RapidJSON (doc reused) ",
      d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(),
      false, memcpy(buffer, p.data(), p.size()), repeat, volume, !justdata);
  BEST_TIME("RapidJSON",
            d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError(),
            false, memcpy(buffer, p.data(), p.size()) && (buffer[p.size()] = '\0'), repeat, volume, !justdata);
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
#ifdef __linux__
  if(!justdata) {
      vector<int> evts;
      evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
      evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
      evts.push_back(PERF_COUNT_HW_BRANCH_MISSES);
      evts.push_back(PERF_COUNT_HW_CACHE_REFERENCES);
      evts.push_back(PERF_COUNT_HW_CACHE_MISSES);
      LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
      vector<unsigned long long> results;
      vector<unsigned long long> stats;
      results.resize(evts.size());
      stats.resize(evts.size());
      std::fill(stats.begin(), stats.end(), 0);// unnecessary
      for(size_t i = 0; i < repeat; i++) {
        unified.start();
        if(json_parse(p, pj) != true) printf("bug\n");
        unified.end(results);
        std::transform (stats.begin(), stats.end(), results.begin(), stats.begin(), std::plus<unsigned long long>());
      }
      printf("simdjson: cycles %f instructions %f branchmisses %f cacheref %f cachemisses %f \n",
      stats[0] * 1.0 / repeat, stats[1] * 1.0 / repeat, stats[2] * 1.0 / repeat, stats[3] * 1.0 / repeat,  stats[4] * 1.0 / repeat );

      std::fill(stats.begin(), stats.end(), 0);
      for(size_t i = 0; i < repeat; i++) {
        memcpy(buffer, p.data(), p.size());
        buffer[p.size()] = '\0';
        unified.start();
        if(d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError() != false) printf("bug\n");
        unified.end(results);
        std::transform (stats.begin(), stats.end(), results.begin(), stats.begin(), std::plus<unsigned long long>());
      }
      printf("RapidJSON: cycles %f instructions %f branchmisses %f cacheref %f cachemisses %f \n",
      stats[0] * 1.0 / repeat, stats[1] * 1.0 / repeat, stats[2] * 1.0 / repeat, stats[3] * 1.0 / repeat,  stats[4] * 1.0 / repeat );

      std::fill(stats.begin(), stats.end(), 0);// unnecessary
      for(size_t i = 0; i < repeat; i++) {
        memcpy(buffer, p.data(), p.size());
        unified.start();
        if(sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize),
                          sajson::mutable_string_view(p.size(), buffer))
                .is_valid() != true) printf("bug\n");
        unified.end(results);
        std::transform (stats.begin(), stats.end(), results.begin(), stats.begin(), std::plus<unsigned long long>());
      }
      printf("sajson: cycles %f instructions %f branchmisses %f cacheref %f cachemisses %f \n",
      stats[0] * 1.0 / repeat, stats[1] * 1.0 / repeat, stats[2] * 1.0 / repeat, stats[3] * 1.0 / repeat,  stats[4] * 1.0 / repeat );
  }
#endif//  __linux__
#ifdef ALLPARSER

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
#endif
  if(!justdata) BEST_TIME("memcpy            ",
            (memcpy(buffer, p.data(), p.size()) == buffer), true, , repeat,
            volume, !justdata);
  aligned_free((void *)p.data());
  free(ast_buffer);
  free(buffer);
}
