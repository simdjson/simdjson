#include <unistd.h>

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "fastjson/core.h"
#include "fastjson/dom.h"
#include "fastjson/fastjson.h"

#include "gason.h"

#include "json11.hpp"

#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

#include "cJSON.h"

#include "jsmn.h"

#include "ujdecode.h"
extern "C" {
#include "ultrajson.h"
}

#include "json/json.h"

SIMDJSON_POP_DISABLE_WARNINGS

// fastjson has a tricky interface
void on_json_error(void *, simdjson_unused const fastjson::ErrorContext &ec) {
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
  bool just_favorites = false;
  int c;
  while ((c = getopt(argc, argv, "vm")) != -1)
    switch (c) {
    case 'v':
      verbose = true;
      break;
    case 'm':
      just_favorites = true;
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
  simdjson::padded_string p;
  auto error = simdjson::padded_string::load(filename).get(p);
  if (error) {
    std::cerr << "Could not load the file " << filename << ": " << error << std::endl;
    return EXIT_FAILURE;
  }
  if (verbose) {
    std::cout << "Input has ";
    if (p.size() > 1000 * 1000)
      std::cout << p.size() / (1000 * 1000) << " MB ";
    else if (p.size() > 1000)
      std::cout << p.size() / 1000 << " KB ";
    else
      std::cout << p.size() << " B ";
    std::cout << std::endl;
  }
  simdjson::dom::parser parser;
  error = parser.parse(p).error();

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
  if (just_favorites) {
    printf("our parser                 : %s \n",
           (error == simdjson::error_code::SUCCESS) ? "correct" : "invalid");
    printf("rapid (check encoding)     : %s \n",
           rapid_correct_checkencoding ? "correct" : "invalid");
    printf("sajson                     : %s \n",
           sajson_correct ? "correct" : "invalid");
    if (error == simdjson::DEPTH_ERROR) {
      printf("simdjson encountered a DEPTH_ERROR, it was parametrized to "
             "reject documents with depth exceeding %zu.\n",
             parser.max_depth());
    }
    if (((error == simdjson::error_code::SUCCESS) != rapid_correct_checkencoding) ||
        (rapid_correct_checkencoding != sajson_correct) ||
        ((error == simdjson::SUCCESS) != sajson_correct)) {
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
    jsmn_parser jsmnparser;
    jsmn_init(&jsmnparser);
    memcpy(buffer, p.data(), p.size());
    buffer[p.size()] = '\0';
    int r = jsmn_parse(&jsmnparser, buffer, p.size(), tokens.get(), static_cast<unsigned int>(p.size()));
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
  Json::CharReader *json_cpp_reader = b.newCharReader();
  Json::Value root;
  Json::String errs;
  bool is_json_cpp_ok =
      json_cpp_reader->parse(buffer, buffer + p.size(), &root, &errs);
  delete json_cpp_reader;

  printf("our parser                 : %s \n",
         (err == simdjson::error_code::SUCCESS) ? "correct" : "invalid");
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
         is_json_cpp_ok ? "correct" : "invalid");

  free(buffer);
  return EXIT_SUCCESS;
}
