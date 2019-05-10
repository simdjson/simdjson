#include <unistd.h>

#include "simdjson/jsonparser.h"

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "fastjson.cpp"
#include "fastjson_dom.cpp"
#include "gason.cpp"
#include "json11.cpp"
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "sajson.h"
extern "C" {
#include "cJSON.c"
#include "cJSON.h"
#include "jsmn.c"
#include "jsmn.h"
#include "ujdecode.h"
#include "ultrajsondec.c"
}
#include "jsoncpp.cpp"
#include "json/json.h"

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

using namespace rapidjson;

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool justfavorites = false;
  int c;
  while ((c = getopt(argc, argv, "vm")) != -1)
    switch (c) {
    case 'v':
      verbose = true;
      break;
    case 'm':
      justfavorites = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "Or " << argv[0] << " -v <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  padded_string p;
  try {
    get_corpus(filename).swap(p);
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
  size_t maxdepth = 1024 * 4;
  bool allocok = pj.allocateCapacity(p.size(), maxdepth);
  if (!allocok) {
    std::cerr << "can't allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  int oursreturn = json_parse(p, pj);
  bool ours_correct = (oursreturn == 0); // returns 0 on success

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  bool rapid_correct_checkencoding =
      (d.Parse<kParseValidateEncodingFlag>((const char *)buffer)
           .HasParseError() == false);
  bool sajson_correct =
      sajson::parse(sajson::dynamic_allocation(),
                    sajson::mutable_string_view(p.size(), buffer))
          .is_valid();
  if (justfavorites) {
    printf("our parser                 : %s \n",
           ours_correct ? "correct" : "invalid");
    printf("rapid (check encoding)     : %s \n",
           rapid_correct_checkencoding ? "correct" : "invalid");
    printf("sajson                     : %s \n",
           sajson_correct ? "correct" : "invalid");
    if(oursreturn == simdjson::DEPTH_ERROR) {
       printf("simdjson encountered a DEPTH_ERROR, it was parametrized to reject documents with depth exceeding %zu.\n", maxdepth);
    }
    if ((ours_correct != rapid_correct_checkencoding) ||
        (rapid_correct_checkencoding != sajson_correct) ||
        (ours_correct != sajson_correct)) {
      printf("WARNING: THEY DISAGREE\n\n");
      return EXIT_FAILURE;
    }
    free(buffer);
    return EXIT_SUCCESS;
  }
  bool rapid_correct = (d.Parse((const char *)buffer).HasParseError() == false);

  std::string json11err;
  bool dropbox_correct = ((json11::Json::parse(buffer, json11err).is_null()) ||
                          (!json11err.empty())) == false;
  bool fastjson_correct = fastjson_parse(buffer);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  bool gason_correct =
      (jsonParse(buffer, &endptr, &value, allocator) == JSON_OK);
  void *state;
  bool ultrajson_correct =
      ((UJDecode(buffer, p.size(), NULL, &state) == NULL) == false);

  auto tokens = std::make_unique<jsmntok_t[]>(p.size());
  bool jsmn_correct = false;
  if (tokens == nullptr) {
    printf("Failed to alloc memory for jsmn\n");
  } else {
    jsmn_parser parser;
    jsmn_init(&parser);
    memcpy(buffer, p.data(), p.size());
    buffer[p.size()] = '\0';
    int r = jsmn_parse(&parser, buffer, p.size(), tokens.get(), p.size());
    tokens = nullptr;
    jsmn_correct = (r > 0);
  }

  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  cJSON *tree = cJSON_Parse(buffer);
  bool cjson_correct = (tree != NULL);
  if (tree != NULL) {
    cJSON_Delete(tree);
  }

  Json::CharReaderBuilder b;
  Json::CharReader *jsoncppreader = b.newCharReader();
  Json::Value root;
  Json::String errs;
  bool isjsoncppok =
      jsoncppreader->parse(buffer, buffer + p.size(), &root, &errs);
  delete jsoncppreader;

  printf("our parser                 : %s \n",
         ours_correct ? "correct" : "invalid");
  printf("rapid                      : %s \n",
         rapid_correct ? "correct" : "invalid");
  printf("rapid (check encoding)     : %s \n",
         rapid_correct_checkencoding ? "correct" : "invalid");
  printf("sajson                     : %s \n",
         sajson_correct ? "correct" : "invalid");
  printf("dropbox                    : %s \n",
         dropbox_correct ? "correct" : "invalid");
  printf("fastjson                   : %s \n",
         fastjson_correct ? "correct" : "invalid");
  printf("gason                      : %s \n",
         gason_correct ? "correct" : "invalid");
  printf("ultrajson                  : %s \n",
         ultrajson_correct ? "correct" : "invalid");
  printf("jsmn                       : %s \n",
         jsmn_correct ? "correct" : "invalid");
  printf("cjson                      : %s \n",
         cjson_correct ? "correct" : "invalid");
  printf("jsoncpp                    : %s \n",
         isjsoncppok ? "correct" : "invalid");

  free(buffer);
  return EXIT_SUCCESS;
}
