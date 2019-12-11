#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "simdjson/jsonparser.h"

// ulp distance
// Marc B. Reynolds, 2016-2019
// Public Domain under http://unlicense.org, see link for details.
// adapted by D. Lemire
inline uint64_t f64_ulp_dist(double a, double b) {
  uint64_t ua, ub;
  memcpy(&ua, &a, sizeof(ua));
  memcpy(&ub, &b, sizeof(ub));
  if ((int64_t)(ub ^ ua) >= 0)
    return (int64_t)(ua - ub) >= 0 ? (ua - ub) : (ub - ua);
  return ua + ub + 0x80000000;
}

bool number_test_powers_of_two() {
  char buf[1024];
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(1024)) {
    printf("allocation failure in number_test\n");
    return false;
  }
  int maxulp = 0;
  for (int i = -1075; i < 1024; ++i) {// large negative values should be zero.
    double expected = pow(2, i);
    auto n = sprintf(buf, "%.*e", std::numeric_limits<double>::max_digits10 - 1, expected);
    buf[n] = '\0';
    fflush(NULL);
    auto ok1 = json_parse(buf, n, pj);
    if (ok1 != 0 || !pj.is_valid()) {
      printf("Could not parse: %s.\n", buf);
      return false;
    }
    simdjson::ParsedJson::Iterator pjh(pj);
    if(!pjh.is_number()) {
      printf("Root should be number\n");
      return false;
    }
    if(pjh.is_integer()) {
      int64_t x = pjh.get_integer();
      int power = 0;
      while(x > 1) {
         if((x % 2) != 0) {
            printf("failed to parse %s. \n", buf);
            return false;
         }
         x = x / 2;
         power ++;
      }
      if(power != i)  {
         printf("failed to parse %s. \n", buf);
         return false;
      }
    } else if(pjh.is_unsigned_integer()) {
      uint64_t x = pjh.get_unsigned_integer();
      int power = 0;
      while(x > 1) {
         if((x % 2) != 0) {
           printf("failed to parse %s. \n", buf);
           return false;
         }
         x = x / 2;
         power ++;
      }
      if(power != i) {
         printf("failed to parse %s. \n", buf);
         return false;
      }
    } else {
      double x = pjh.get_double();
      int ulp = f64_ulp_dist(x,expected);  
      if(ulp > maxulp) maxulp = ulp;
      if(ulp > 3) {
         printf("failed to parse %s. ULP = %d i = %d \n", buf, ulp, i);
         return false;
      }
    }
  }
  printf("Powers of 2 can be parsed, maxulp = %d.\n", maxulp);
  return true;
}

bool number_test_powers_of_ten() {
  char buf[1024];
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(1024)) {
    printf("allocation failure in number_test\n");
    return false;
  }
  for (int i = -1000000; i <= 308; ++i) {// large negative values should be zero.
    auto n = sprintf(buf,"1e%d", i);
    buf[n] = '\0';
    fflush(NULL);
    auto ok1 = json_parse(buf, n, pj);
    if (ok1 != 0 || !pj.is_valid()) {
      printf("Could not parse: %s.\n", buf);
      return false;
    }
    simdjson::ParsedJson::Iterator pjh(pj);
    if(!pjh.is_number()) {
      printf("Root should be number\n");
      return false;
    }
    if(pjh.is_integer()) {
      int64_t x = pjh.get_integer();
      int power = 0;
      while(x > 1) {
         if((x % 10) != 0) {
            printf("failed to parse %s. \n", buf);
            return false;
         }
         x = x / 10;
         power ++;
      }
      if(power != i)  {
         printf("failed to parse %s. \n", buf);
         return false;
      }
    } else if(pjh.is_unsigned_integer()) {
      uint64_t x = pjh.get_unsigned_integer();
      int power = 0;
      while(x > 1) {
         if((x % 10) != 0) {
           printf("failed to parse %s. \n", buf);
           return false;
         }
         x = x / 10;
         power ++;
      }
      if(power != i) {
         printf("failed to parse %s. \n", buf);
         return false;
      }
    } else {
      double x = pjh.get_double();
      double expected = std::pow(10, i);
      int ulp = (int) f64_ulp_dist(x, expected);
      if(ulp > 1) {
         printf("failed to parse %s. \n", buf);
         printf("actual: %.20g expected: %.20g \n", x, expected);
         printf("ULP: %d \n", ulp);
         return false;
      }
    }
  }
  printf("Powers of 10 can be parsed.\n");
  return true;
}


// adversarial example that once triggred overruns, see https://github.com/lemire/simdjson/issues/345
bool bad_example() {
  std::string badjson = "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6";
  simdjson::ParsedJson pj = simdjson::build_parsed_json(badjson);
  if(pj.is_valid()) {
    printf("This json should not be valid %s.\n", badjson.c_str());
    return false;
  }
  return true;
}
// returns true if successful
bool stable_test() {
  std::string json = "{"
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
      "}";
  simdjson::ParsedJson pj = simdjson::build_parsed_json(json);
  std::ostringstream myStream;
  if( ! pj.print_json(myStream) ) {
    std::cout << "cannot print it out? " << std::endl;
    return false;
  }
  std::string newjson = myStream.str();
  if(json != newjson) {
    std::cout << "serialized json differs!" << std::endl;
    std::cout << json << std::endl;
    std::cout << newjson << std::endl;
  }
  return newjson == json;
}

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
  if(!stable_test())
    return EXIT_FAILURE;
  if(!bad_example())
    return EXIT_FAILURE;
  if(!number_test_powers_of_two())
    return EXIT_FAILURE;
  if(!number_test_powers_of_ten())
    return EXIT_FAILURE;
  if (!navigate_test())
    return EXIT_FAILURE;
  if (!skyprophet_test())
    return EXIT_FAILURE;
  std::cout << "Basic tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
