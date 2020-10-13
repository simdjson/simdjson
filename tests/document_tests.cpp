#include <string>
#include <vector>
#include <unistd.h>

#include "simdjson.h"
#include "test_macros.h"

namespace document_tests {
  bool issue938() {
    std::vector<std::string> json_strings{"[true,false]", "[1,2,3,null]",
                                        R"({"yay":"json!"})"};
    simdjson::dom::parser parser1;
    for (simdjson::padded_string str : json_strings) {
      simdjson::dom::element element;
      ASSERT_SUCCESS( parser1.parse(str).get(element) );
      std::cout << element << std::endl;
    }
    std::vector<std::string> file_paths{
      ADVERSARIAL_JSON,      FLATADVERSARIAL_JSON, DEMO_JSON,
      TWITTER_TIMELINE_JSON, REPEAT_JSON,         SMALLDEMO_JSON,
      TRUENULL_JSON};
    for (auto path : file_paths) {
      simdjson::dom::parser parser2;
      simdjson::dom::element element;
      std::cout << "file: " << path << std::endl;
      ASSERT_SUCCESS( parser2.load(path).get(element) );
      std::cout << element.type() << std::endl;
    }
    simdjson::dom::parser parser3;
    for (auto path : file_paths) {
      simdjson::dom::element element;
      std::cout << "file: " << path << std::endl;
      ASSERT_SUCCESS( parser3.load(path).get(element) );
      std::cout << element.type() << std::endl;
    }
    return true;
  }

  // adversarial example that once triggred overruns, see https://github.com/lemire/simdjson/issues/345
  bool bad_example() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string badjson = "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6"_padded;
    simdjson::dom::parser parser;
    ASSERT_ERROR( parser.parse(badjson), simdjson::TAPE_ERROR );
    return true;
  }
  bool count_array_example() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string smalljson = "[1,2,3]"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::array array;
    ASSERT_SUCCESS( parser.parse(smalljson).get(array) );
    ASSERT_EQUAL( array.size(), 3 );
    return true;
  }
  bool count_object_example() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string smalljson = "{\"1\":1,\"2\":1,\"3\":1}"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::object object;
    ASSERT_SUCCESS( parser.parse(smalljson).get(object) );
    ASSERT_EQUAL( object.size(), 3 );
    return true;
  }
  bool padded_with_open_bracket() {
    std::cout << __func__ << std::endl;
    simdjson::dom::parser parser;
    // This is an invalid document padded with open braces.
    ASSERT_ERROR( parser.parse("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[", 2, false), simdjson::TAPE_ERROR);
    // This is a valid document padded with open braces.
    ASSERT_SUCCESS( parser.parse("[][[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[", 2, false) );
    return true;
  }
  // returns true if successful
  bool stable_test() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string json = "{"
          "\"Image\":{"
              "\"Width\":800,"
              "\"Height\":600,"
              "\"Title\":\"View from 15th Floor\","
              "\"Thumbnail\":{"
              "\"Url\":\"http://www.example.com/image/481989943\","
              "\"Height\":125,"
              "\"Width\":100"
              "},"
              "\"Animated\":false,"
              "\"IDs\":[116,943.3,234,38793]"
            "}"
        "}"_padded;
    simdjson::dom::parser parser;
    std::ostringstream myStream;
#if SIMDJSON_EXCEPTIONS
    myStream << parser.parse(json);
#else
    simdjson::dom::element doc;
    simdjson_unused auto error = parser.parse(json).get(doc);
    myStream << doc;
#endif
    std::string newjson = myStream.str();
    if(static_cast<std::string>(json) != newjson) {
      std::cout << "serialized json differs!" << std::endl;
      std::cout << static_cast<std::string>(json) << std::endl;
      std::cout << newjson << std::endl;
    }
    return newjson == static_cast<std::string>(json);
  }
  // returns true if successful
  bool skyprophet_test() {
    std::cout << "Running " << __func__ << std::endl;
    const size_t n_records = 100000;
    std::vector<std::string> data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf, sizeof(buf),
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"school\": {\"id\": %zu, \"name\": \"school%zu\"}}",
                      i, i, (i % 2) ? "male" : "female", i % 10, i % 10);
      if (n >= sizeof(buf)) { abort(); }
      data.emplace_back(std::string(buf, n));
    }
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf, sizeof(buf), "{\"counter\": %f, \"array\": [%s]}", static_cast<double>(i) * 3.1416,
                        (i % 2) ? "true" : "false");
      if (n >= sizeof(buf)) { abort(); }
      data.emplace_back(std::string(buf, n));
    }
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf, sizeof(buf), "{\"number\": %e}", static_cast<double>(i) * 10000.31321321);
      if (n >= sizeof(buf)) { abort(); }
      data.emplace_back(std::string(buf, n));
    }
    data.emplace_back(std::string("true"));
    data.emplace_back(std::string("false"));
    data.emplace_back(std::string("null"));
    data.emplace_back(std::string("0.1"));
    size_t maxsize = 0;
    for (auto &s : data) {
      if (maxsize < s.size())
        maxsize = s.size();
    }
    simdjson::dom::parser parser;
    size_t counter = 0;
    for (auto &rec : data) {
      if ((counter % 10000) == 0) {
        printf(".");
        fflush(NULL);
      }
      counter++;
      auto error = parser.parse(rec.c_str(), rec.length()).error();
      if (error != simdjson::error_code::SUCCESS) {
        printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
        printf("Parsing failed. Error is %s\n", simdjson::error_message(error));
        return false;
      }
      error = parser.parse(rec.c_str(), rec.length()).error();
      if (error != simdjson::error_code::SUCCESS) {
        printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
        printf("Parsing failed. Error is %s\n", simdjson::error_message(error));
        return false;
      }
    }
    printf("\n");
    return true;
  }
  bool lots_of_brackets() {
    std::string input;
    for(size_t i = 0; i < 200; i++) {
      input += "[";
    }
    for(size_t i = 0; i < 200; i++) {
      input += "]";
    }
    simdjson::dom::parser parser;
    auto error = parser.parse(input).error();
    if (error) { std::cerr << "Error: " << simdjson::error_message(error) << std::endl; return false; }
    return true;
  }
  bool run() {
    return issue938() &&
           padded_with_open_bracket() &&
           bad_example() &&
           count_array_example() &&
           count_object_example() &&
           stable_test() &&
           skyprophet_test() &&
           lots_of_brackets();
  }
}




int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::available_implementations[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      if(!impl->supported_by_runtime_system()) {
        fprintf(stderr, "The selected implementation does not match your current CPU: -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::active_implementation = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  std::cout << "Running tests against this implementation: " << simdjson::active_implementation->name();
  std::cout << "(" << simdjson::active_implementation->description() << ")" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running document tests." << std::endl;
  if (document_tests::run()) {
    std::cout << "document tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}