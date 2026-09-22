#include <unistd.h>

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS

// #define RAPIDJSON_SSE2 // bad
// #define RAPIDJSON_SSE42 // bad
#include "rapidjson/document.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

SIMDJSON_POP_DISABLE_WARNINGS

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

  printf("our parser                 : %s \n",
         (error == simdjson::error_code::SUCCESS) ? "correct" : "invalid");
  printf("rapid                      : %s \n",
         rapid_correct ? "correct" : "invalid");
  printf("rapid (check encoding)     : %s \n",
         rapid_correct_checkencoding ? "correct" : "invalid");
  printf("sajson                     : %s \n",
         sajson_correct ? "correct" : "invalid");

  free(buffer);
  return EXIT_SUCCESS;
}
