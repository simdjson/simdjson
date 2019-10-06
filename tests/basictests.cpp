#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "simdjson/jsonparser.h"


// returns true if successful
bool navigate_test() {
  std::string json = "{"
        "\"Image\": {"
            "\"Width\":  800,"
            "\"Height\": 600,"
            "\"Title\":  \"View from 15th Floor\","
            "\"Thumbnail\": {"
            "    \"Url\":    \"http://www.example.com/image/481989943\","
            "    \"Height\": 125,"
            "    \"Width\":  100"
            "},"
            "\"Animated\" : false,"
            "\"IDs\": [116, 943, 234, 38793]"
          "}"
      "}";

  simdjson::ParsedJson pj = simdjson::build_parsed_json(json);
  if (!pj.is_valid()) {
      printf("Something is wrong in navigate: %s.\n", json.c_str());
      return false;
  }
  simdjson::ParsedJson::Iterator pjh(pj);
  if(!pjh.is_object()) {
    printf("Root should be object\n");
    return false;
  }
  if(!pjh.down()) {
    printf("Root should not be emtpy\n");
    return false;
  }
  if(!pjh.is_string()) {
    printf("Object should start with string key\n");
    return false;
  }
  if(pjh.prev()) {
    printf("We should not be able to go back from the start of the scope.\n");
    return false;
  }
  if(strcmp(pjh.get_string(),"Image")!=0) {
    printf("There should be a single key, image.\n");
    return false;
  }
  pjh.move_to_value();
  if(!pjh.is_object()) {
    printf("Value of image should be object\n");
    return false;
  }
  if(!pjh.down()) {
    printf("Image key should not be emtpy\n");
    return false;
  }
  if(!pjh.next()) {
    printf("key should have a value\n");
    return false;
  }
  if(!pjh.prev()) {
    printf("We should go back to the key.\n");
    return false;
  }
  if(strcmp(pjh.get_string(),"Width")!=0) {
    printf("There should be a  key Width.\n");
    return false;
  }
  return true;
}

// returns true if successful
bool skyprophet_test() {
  const size_t n_records = 100000;
  std::vector<std::string> data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"school\": {\"id\": %zu, \"name\": \"school%zu\"}}",
                     i, i, (i % 2) ? "male" : "female", i % 10, i % 10);
    data.emplace_back(std::string(buf, n));
  }
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf, "{\"counter\": %f, \"array\": [%s]}", i * 3.1416,
                     (i % 2) ? "true" : "false");
    data.emplace_back(std::string(buf, n));
  }
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf, "{\"number\": %e}", i * 10000.31321321);
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
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(maxsize)) {
    printf("allocation failure in skyprophet_test\n");
    return false;
  }
  size_t counter = 0;
  for (auto &rec : data) {
    if ((counter % 10000) == 0) {
      printf(".");
      fflush(NULL);
    }
    counter++;
    auto ok1 = json_parse(rec.c_str(), rec.length(), pj);
    if (ok1 != 0 || !pj.is_valid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
    auto ok2 = json_parse(rec, pj);
    if (ok2 != 0 || !pj.is_valid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
  }
  printf("\n");
  return true;
}

int main() {
  std::cout << "Running basic tests." << std::endl;
  // if (!navigate_test())
  //   return EXIT_FAILURE;
  if (!skyprophet_test())
    return EXIT_FAILURE;
  std::cout << "Basic tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
