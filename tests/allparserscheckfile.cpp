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
  std::pair<u8 *, size_t> p;
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

  bool ours_correct = json_parse(p.first, p.second, pj);

  rapidjson::Document d;

  char *buffer = (char *)malloc(p.second + 1);
  memcpy(buffer, p.first, p.second);
  buffer[p.second] = '\0';
  bool rapid_correct = (d.Parse((const char *)buffer).HasParseError() == false);
  bool rapid_correct_checkencoding = (d.Parse<kParseValidateEncodingFlag>((const char *)buffer).HasParseError() == false);
  bool sajson_correct = sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(p.second, buffer)).is_valid();
  std::string json11err;
  bool dropbox_correct = (( json11::Json::parse(buffer,json11err).is_null() ) || ( ! json11err.empty() )) == false;
  bool fastjson_correct =  fastjson_parse(buffer);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  bool gason_correct = (jsonParse(buffer, &endptr, &value, allocator) == JSON_OK);
  void *state;
  bool ultrajson_correct = ((UJDecode(buffer, p.second, NULL, &state) == NULL) == false);
  printf("our parser                 : %s \n", ours_correct ?   "correct":"invalid");
  printf("rapid                      : %s \n", rapid_correct ?  "correct":"invalid");
  printf("rapid (check encoding)     : %s \n", rapid_correct_checkencoding ?  "correct":"invalid");
  printf("sajson                     : %s \n", sajson_correct ? "correct":"invalid");
  printf("dropbox                    : %s \n", dropbox_correct ? "correct":"invalid");
  printf("fastjson                   : %s \n", fastjson_correct ? "correct":"invalid");
  printf("gason                      : %s \n", gason_correct ? "correct":"invalid");
  printf("ultrajson                  : %s \n", ultrajson_correct ? "correct":"invalid");
       
  free(buffer);
  free(p.first);
  deallocate_ParsedJson(pj_ptr);
  return EXIT_SUCCESS;
}
