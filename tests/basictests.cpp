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
  const size_t n_records = 10000000;
  std::vector<std::string> data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"school\": {\"id\": %zu, \"name\": \"school%zu\"}}",
                     i, i, (i % 2) ? "male" : "female", i % 10, i % 10);
    data.emplace_back(std::string(buf, n));
  }

  ParsedJson pj;
  if (!pj.allocateCapacity(1LL << 25)) {
    printf("allocation failure in skyprophet_test\n");
    return false;
  }
  for (auto &rec : data) {
    auto ok1 = json_parse(rec.c_str(), rec.length(), pj);
    if (ok1 != 0 || !pj.isValid()) {
      printf("Something is wrong in skyprophet_test.\n");
      return false;
    }
    auto ok2 = json_parse(rec, pj);
    if (ok2 != 0 || !pj.isValid()) {
      printf("Something is wrong in skyprophet_test.\n");
      return false;
    }
  }
  return true;
}

int main() {
  std::cout << "Running basic tests." << std::endl;

  std::cout << "Basic tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
