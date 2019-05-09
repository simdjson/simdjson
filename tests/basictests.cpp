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
  ParsedJson pj;
  if (!pj.allocateCapacity(maxsize)) {
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
    if (ok1 != 0 || !pj.isValid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
    auto ok2 = json_parse(rec, pj);
    if (ok2 != 0 || !pj.isValid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
  }
  printf("\n");
  return true;
}

int main() {
  std::cout << "Running basic tests." << std::endl;
  if (!skyprophet_test())
    return EXIT_FAILURE;
  std::cout << "Basic tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
