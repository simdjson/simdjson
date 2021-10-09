#include "simdjson.cpp"
#include "simdjson.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Please specify at least one file name and" << std::endl;
    std::cerr << "up to two files." << std::endl;
    std::cerr << "The first file should be a JSON document." << std::endl;
    std::cerr << "The second file should container many JSON documents."
              << std::endl;
    std::cerr << "Try the test files: jsonexamples/twitter.json "
                 "jsonexamples/amazon_cellphones.ndjson"
              << std::endl;
    return EXIT_FAILURE;
  }
  const char *filename = argv[1];

  simdjson::padded_string json;
  std::cout << "loading: " << filename << std::endl;
  auto error = simdjson::padded_string::load(filename).get(json);
  if (error) {
    std::cout << "could not load the file " << filename << std::endl;
    std::cout << "error code: " << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "loaded: " << json.size() << " bytes." << std::endl;
  }
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  error = parser.iterate(json).get(doc);
  if (error) {
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::ondemand::json_type type;
  error = doc.type().get(type);
  if (error) {
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "if valid, the document has the following type at the root: " << type
            << std::endl;

  if (argc == 2) {
    return EXIT_SUCCESS;
  }

  // iterate_many
  const char *filename2 = argv[2];
  std::cout << "loading: " << filename2 << std::endl;
  simdjson::padded_string json2;
  error = simdjson::padded_string::load(filename2).get(json2);
  if (error) {
    std::cout << "could not load the file " << filename2 << std::endl;
    std::cout << "error code: " << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "loaded: " << json2.size() << " bytes." << std::endl;
  }
  simdjson::ondemand::document_stream stream;
  error = parser.iterate_many(json2).get(stream);
  size_t counter{0};
  if (!error) {
    for (auto result : stream) {
      error = result.error();
      counter++;
    }
  }
  if (error) {
    std::cout << "parse_many failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "I found " << counter << " potential JSON documents." << std::endl;
  }
  std::cout << "For more information on how simdjson works, please refer to our documentation." << std::endl;
  std::cout << "https://github.com/simdjson/simdjson/blob/master/doc/basics.md" << std::endl;
  return EXIT_SUCCESS;
}
