#include <unistd.h>

#include "simdjson/jsonparser.h"


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
#include "cJSON.h"
#include "cJSON.c"
#include "jsmn.h"
#include "jsmn.c"
}
#include "json/json.h"
#include "jsoncpp.cpp"
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



using namespace rapidjson;
using namespace std;

int main(int argc, char *argv[]) {
  bool verbose = false;
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
  bool ours_correct = json_parse(p, pj);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  bool rapid_correct = (d.Parse((const char *)buffer).HasParseError() == false);
  bool rapid_correct_checkencoding = (d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError() == false);
  bool sajson_correct = sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(p.size(), buffer)).is_valid();
  std::string json11err;
  bool dropbox_correct = (( json11::Json::parse(buffer,json11err).is_null() ) || ( ! json11err.empty() )) == false;
  bool fastjson_correct =  fastjson_parse(buffer);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  bool gason_correct = (jsonParse(buffer, &endptr, &value, allocator) == JSON_OK);
  void *state;
  bool ultrajson_correct = ((UJDecode(buffer, p.size(), NULL, &state) == NULL) == false);
  
  jsmntok_t * tokens = new jsmntok_t[p.size()];
  bool jsmn_correct = false; 
  if(tokens == NULL) {
    printf("Failed to alloc memory for jsmn\n");
  } else {
    jsmn_parser parser;
    jsmn_init(&parser);
    memcpy(buffer, p.data(), p.size());
    buffer[p.size()] = '\0';
    int r = jsmn_parse(&parser, buffer, p.size(), tokens, p.size());
    delete[] tokens;
    tokens = NULL;
    jsmn_correct = (r > 0);
  }

  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  cJSON * tree = cJSON_Parse(buffer);
  bool cjson_correct = (tree != NULL);
  if (tree != NULL) {
        cJSON_Delete(tree);
  }

  Json::CharReaderBuilder b;
  Json::CharReader * jsoncppreader = b.newCharReader();
  Json::Value root;
  Json::String errs;
  bool isjsoncppok = jsoncppreader->parse(buffer,buffer+p.size(),&root,&errs);
  delete jsoncppreader;


  printf("our parser                 : %s \n", ours_correct ?   "correct":"invalid");
  printf("rapid                      : %s \n", rapid_correct ?  "correct":"invalid");
  printf("rapid (check encoding)     : %s \n", rapid_correct_checkencoding ?  "correct":"invalid");
  printf("sajson                     : %s \n", sajson_correct ? "correct":"invalid");
  printf("dropbox                    : %s \n", dropbox_correct ? "correct":"invalid");
  printf("fastjson                   : %s \n", fastjson_correct ? "correct":"invalid");
  printf("gason                      : %s \n", gason_correct ? "correct":"invalid");
  printf("ultrajson                  : %s \n", ultrajson_correct ? "correct":"invalid");
  printf("jsmn                       : %s \n", jsmn_correct ? "correct":"invalid");
  printf("cjson                      : %s \n", cjson_correct ? "correct":"invalid");
  printf("jsoncpp                    : %s \n", isjsoncppok ? "correct":"invalid");

  aligned_free((void*)p.data());       
  free(buffer);
  return EXIT_SUCCESS;
}
