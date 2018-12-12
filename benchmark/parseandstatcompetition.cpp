#include <unistd.h>
#include "simdjson/jsonparser.h"

#include "benchmark.h"

// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h" 
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

using namespace rapidjson;
using namespace std;

struct stat_s {
  size_t number_count;
  size_t object_count;
  size_t array_count;
  size_t null_count;
  size_t true_count;
  size_t false_count;
  bool valid;
};

typedef struct stat_s stat_t;

stat_t simdjson_computestats(const std::string_view & p) {
  stat_t answer;
  ParsedJson pj = build_parsed_json(p);
  answer.valid = pj.isValid();
  if(!answer.valid) {
    return answer;
  }
  answer.number_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
  size_t tapeidx = 0;
  u64 tape_val = pj.tape[tapeidx++];
  u8 type = (tape_val >> 56);
  size_t howmany = 0;
  assert (type == 'r');
  howmany = tape_val & JSONVALUEMASK;
  tapeidx++;
  for (; tapeidx < howmany; tapeidx++) {
      tape_val = pj.tape[tapeidx];
      u64 payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      switch (type) {
      case 'l': // we have a long int
        answer.number_count++;
        tapeidx++; // skipping the integer
        break;
      case 'd': // we have a double
        answer.number_count++;
        tapeidx++; // skipping the double
        break;
      case 'n': // we have a null
        answer.null_count++;
        break;
      case 't': // we have a true
        answer.true_count++;
        break;
      case 'f': // we have a false
        answer.false_count ++;
        break;
      case '{': // we have an object
        answer.object_count ++;
        break;
      case '}': // we end an object
        break;
      case '[': // we start an array
        answer.array_count ++;
        break;
      case ']': // we end an array
        break;
      default:
        answer.valid = false;
        return answer;
      }
    }
    return answer;
}

stat_t rapid_computestats(const std::string_view & p) {
  stat_t answer;
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(p.data());
  answer.valid = ! d.HasParseError();
  if(d.HasParseError()) {

  }
  if(!answer.valid) {
    return answer;
  }
  answer.number_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
}


int main(int argc, char *argv[]) {
  bool verbose = false;
  bool all = false; 
  int c;
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
    cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    cerr << "Or " << argv[0] << " -v <jsonfile>\n";
    exit(1);
  }
  const char * filename = argv[optind];
  if(optind + 1 < argc) {
    cerr << "warning: ignoring everything after " << argv[optind  + 1] << endl;
  }
  std::string_view p;
  try {
    p = get_corpus(filename);
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
  ParsedJson pj;
  bool allocok = pj.allocateCapacity(p.size(), 1024);

  if (!allocok) {
    std::cerr << "can't allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  int repeat = 10;
  int volume = p.size();
  BEST_TIME("simdjson (dynamic mem) ", build_parsed_json(p).isValid(), true, , repeat, volume, true);

  BEST_TIME("simdjson (static alloc) ", json_parse(p, pj), true, , repeat, volume, true);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';

  BEST_TIME("RapidJSON", 
      d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError(),
      false, memcpy(buffer, p.data(), p.size()), repeat, volume, true);
  BEST_TIME("RapidJSON (insitu)", d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError(), false,
            memcpy(buffer, p.data(), p.size()), repeat, volume, true);

  BEST_TIME("sajson (dynamic mem, insitu)", sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(p.size(), buffer)).is_valid(), true, memcpy(buffer, p.data(), p.size()), repeat, volume, true);

  size_t astbuffersize = p.size();
  size_t * ast_buffer = (size_t *) malloc(astbuffersize * sizeof(size_t));

  BEST_TIME("sajson (static alloc, insitu)", sajson::parse(sajson::bounded_allocation(ast_buffer, astbuffersize), sajson::mutable_string_view(p.size(), buffer)).is_valid(), true, memcpy(buffer, p.data(), p.size()), repeat, volume, true);
  std::string json11err;
  if(all) BEST_TIME("dropbox (json11)     ",  (( json11::Json::parse(buffer,json11err).is_null() ) || ( ! json11err.empty() )), false, memcpy(buffer, p.data(), p.size()), repeat, volume, true);

  if(all) BEST_TIME("fastjson             ", fastjson_parse(buffer), true, memcpy(buffer, p.data(), p.size()), repeat, volume, true);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  if(all) BEST_TIME("gason             ", jsonParse(buffer, &endptr, &value, allocator), JSON_OK, memcpy(buffer, p.data(), p.size()), repeat, volume, true);
  void *state;
  if(all) BEST_TIME("ultrajson         ", (UJDecode(buffer, p.size(), NULL, &state) == NULL), false, memcpy(buffer, p.data(), p.size()), repeat, volume, true);
  BEST_TIME("memcpy            ", (memcpy(buffer, p.data(), p.size()) == buffer), true, , repeat, volume, true);
  free((void*)p.data());
  free(ast_buffer);
  free(buffer);
}

