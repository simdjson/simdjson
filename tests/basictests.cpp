#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <string_view>

#include "simdjson.h"

#ifndef JSON_TEST_PATH
#define JSON_TEST_PATH "jsonexamples/twitter.json"
#endif

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


bool number_test_small_integers() {
  char buf[1024];
  simdjson::document::parser parser;
  for (int m = 10; m < 20; m++) {
    for (int i = -1024; i < 1024; i++) {
      auto n = sprintf(buf, "%*d", m, i);
      buf[n] = '\0';
      fflush(NULL);
      auto ok1 = json_parse(buf, n, parser);
      if (ok1 != 0 || !parser.is_valid()) {
        printf("Could not parse '%s': %s\n", buf, simdjson::error_message(ok1).c_str());
        return false;
      }
      simdjson::document::iterator iter(parser);
      if(!iter.is_number()) {
        printf("Root should be number\n");
        return false;
      }
      if(!iter.is_integer()) {
        printf("Root should be an integer\n");
        return false;
      }
      int64_t x = iter.get_integer();
      if(x != i) {
        printf("failed to parse %s. \n", buf);
        return false;
      }
    } 
  }
  printf("Small integers can be parsed.\n");
  return true;
}


bool number_test_powers_of_two() {
  char buf[1024];
  simdjson::document::parser parser;
  int maxulp = 0;
  for (int i = -1075; i < 1024; ++i) {// large negative values should be zero.
    double expected = pow(2, i);
    auto n = sprintf(buf, "%.*e", std::numeric_limits<double>::max_digits10 - 1, expected);
    buf[n] = '\0';
    fflush(NULL);
    auto ok1 = json_parse(buf, n, parser);
    if (ok1 != 0 || !parser.is_valid()) {
      printf("Could not parse: %s.\n", buf);
      return false;
    }
    simdjson::document::iterator iter(parser);
    if(!iter.is_number()) {
      printf("Root should be number\n");
      return false;
    }
    if(iter.is_integer()) {
      int64_t x = iter.get_integer();
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
    } else if(iter.is_unsigned_integer()) {
      uint64_t x = iter.get_unsigned_integer();
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
      double x = iter.get_double();
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
  simdjson::document::parser parser;
  for (int i = -1000000; i <= 308; ++i) {// large negative values should be zero.
    auto n = sprintf(buf,"1e%d", i);
    buf[n] = '\0';
    fflush(NULL);
    auto ok1 = json_parse(buf, n, parser);
    if (ok1 != 0 || !parser.is_valid()) {
      printf("Could not parse: %s.\n", buf);
      return false;
    }
    simdjson::document::iterator iter(parser);
    if(!iter.is_number()) {
      printf("Root should be number\n");
      return false;
    }
    if(iter.is_integer()) {
      int64_t x = iter.get_integer();
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
    } else if(iter.is_unsigned_integer()) {
      uint64_t x = iter.get_unsigned_integer();
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
      double x = iter.get_double();
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
  simdjson::document::parser parser = simdjson::build_parsed_json(badjson);
  if(parser.is_valid()) {
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
  simdjson::document::parser parser = simdjson::build_parsed_json(json);
  std::ostringstream myStream;
  if( ! parser.print_json(myStream) ) {
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

static bool parse_json_message_issue467(char const* message, std::size_t len, size_t expectedcount) {
    simdjson::document::parser parser;
    size_t count = 0;
    simdjson::padded_string str(message,len);
    for (auto [doc, error] : parser.parse_many(str, len)) {
      if (error) {
          std::cerr << "Failed with simdjson error= " << error << std::endl;
          return false;
      }
      count++;
    }
    if(count != expectedcount) {
        std::cerr << "bad count" << std::endl;
        return false;
    }
    return true;
}

bool json_issue467() {
    printf("Running json_issue467.\n");
    const char * single_message = "{\"error\":[],\"result\":{\"token\":\"xxx\"}}";
    const char* two_messages = "{\"error\":[],\"result\":{\"token\":\"xxx\"}}{\"error\":[],\"result\":{\"token\":\"xxx\"}}";

    if(!parse_json_message_issue467(single_message, strlen(single_message),1)) {
      return false;
    }
    if(!parse_json_message_issue467(two_messages, strlen(two_messages),2)) {
      return false;
    }
    return true;
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

  simdjson::document::parser parser = simdjson::build_parsed_json(json);
  if (!parser.is_valid()) {
      printf("Something is wrong in navigate: %s.\n", json.c_str());
      return false;
  }
  simdjson::document::iterator iter(parser);
  if(!iter.is_object()) {
    printf("Root should be object\n");
    return false;
  }
  if(iter.move_to_key("bad key")) {
    printf("We should not move to a non-existing key\n");
    return false;    
  }
  if(!iter.is_object()) {
    printf("We should have remained at the object.\n");
    return false;
  }
  if(iter.move_to_key_insensitive("bad key")) {
    printf("We should not move to a non-existing key\n");
    return false;    
  }
  if(!iter.is_object()) {
    printf("We should have remained at the object.\n");
    return false;
  }
  if(iter.move_to_key("bad key", 7)) {
    printf("We should not move to a non-existing key\n");
    return false;    
  }
  if(!iter.is_object()) {
    printf("We should have remained at the object.\n");
    return false;
  }
  if(!iter.down()) {
    printf("Root should not be emtpy\n");
    return false;
  }
  if(!iter.is_string()) {
    printf("Object should start with string key\n");
    return false;
  }
  if(iter.prev()) {
    printf("We should not be able to go back from the start of the scope.\n");
    return false;
  }
  if(strcmp(iter.get_string(),"Image")!=0) {
    printf("There should be a single key, image.\n");
    return false;
  }
  iter.move_to_value();
  if(!iter.is_object()) {
    printf("Value of image should be object\n");
    return false;
  }
  if(!iter.down()) {
    printf("Image key should not be emtpy\n");
    return false;
  }
  if(!iter.next()) {
    printf("key should have a value\n");
    return false;
  }
  if(!iter.prev()) {
    printf("We should go back to the key.\n");
    return false;
  }
  if(strcmp(iter.get_string(),"Width")!=0) {
    printf("There should be a  key Width.\n");
    return false;
  }
  if(!iter.up()) {
    return false;
  }
  if(!iter.move_to_key("IDs")) {
    printf("We should be able to move to an existing key\n");
    return false;    
  }
  if(!iter.is_array()) {
    printf("Value of IDs should be array, it is %c \n", iter.get_type());
    return false;
  }
  if(iter.move_to_index(4)) {
    printf("We should not be able to move to a non-existing index\n");
    return false;    
  }
  if(!iter.is_array()) {
    printf("We should have remained at the array\n");
    return false;
  }
  return true;
}

// returns true if successful
bool JsonStream_utf8_test() {
  printf("Running JsonStream_utf8_test");
  fflush(NULL);
  const size_t n_records = 10000;
  std::string data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"été\": {\"id\": %zu, \"name\": \"éventail%zu\"}}",
                     i, i, (i % 2) ? "⺃" : "⺕", i % 10, i % 10);
    data += std::string(buf, n);
  }
  const size_t batch_size = 1000;
  printf(".");
  fflush(NULL);
  simdjson::padded_string str(data);
  simdjson::JsonStream<simdjson::padded_string> js{str, batch_size};
  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
  size_t count = 0;
  simdjson::document::parser parser;
  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
    parse_res = js.json_parse(parser);
    simdjson::document::iterator iter(parser);
    if(!iter.is_object()) {
      printf("Root should be object\n");
      return false;
    }
    if(!iter.down()) {
      printf("Root should not be emtpy\n");
      return false;
    }
    if(!iter.is_string()) {
      printf("Object should start with string key\n");
      return false;
    }
    if(strcmp(iter.get_string(),"id")!=0) {
      printf("There should a single key, id.\n");
      return false;
    }
    iter.move_to_value();
    if(!iter.is_integer()) {
      printf("Value of image should be integer\n");
      return false;
    }
    int64_t keyid = iter.get_integer();
    if(keyid != (int64_t)count) {
      printf("key does not match %d, expected %d\n",(int) keyid, (int) count);
      return false;
    }
    count++;
  }
  if(count != n_records) {
    printf("Something is wrong in JsonStream_utf8_test at window size = %zu.\n", batch_size);
    return false;
  }
  printf("ok\n");
  return true;
}

// returns true if successful
bool JsonStream_test() {
  printf("Running JsonStream_test");
  fflush(NULL);
  const size_t n_records = 10000;
  std::string data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                     i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
    data += std::string(buf, n);
  }
  const size_t batch_size = 1000;
  printf(".");
  fflush(NULL);
  simdjson::padded_string str(data);
  simdjson::JsonStream<simdjson::padded_string> js{str, batch_size};
  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
  size_t count = 0;
  simdjson::document::parser parser;
  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
    parse_res = js.json_parse(parser);
    simdjson::document::iterator iter(parser);
    if(!iter.is_object()) {
      printf("Root should be object\n");
      return false;
    }
    if(!iter.down()) {
      printf("Root should not be emtpy\n");
      return false;
    }
    if(!iter.is_string()) {
      printf("Object should start with string key\n");
      return false;
    }
    if(strcmp(iter.get_string(),"id")!=0) {
      printf("There should a single key, id.\n");
      return false;
    }
    iter.move_to_value();
    if(!iter.is_integer()) {
      printf("Value of image should be integer\n");
      return false;
    }
    int64_t keyid = iter.get_integer();
    if(keyid != (int64_t)count) {
      printf("key does not match %d, expected %d\n",(int) keyid, (int) count);
      return false;
    }
    count++;
  }
  if(count != n_records) {
    printf("Something is wrong in JsonStream_test at window size = %zu.\n", batch_size);
    return false;
  }
  printf("ok\n");
  return true;
}

// returns true if successful
bool document_stream_test() {
  printf("Running document_stream_test");
  fflush(NULL);
  const size_t n_records = 10000;
  std::string data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                     i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
    data += std::string(buf, n);
  }
  for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
    printf(".");
    fflush(NULL);
    simdjson::padded_string str(data);
    simdjson::document::parser parser;
    size_t count = 0;
    for (auto [doc, error] : parser.parse_many(str, batch_size)) {
      if (error) {
        printf("Error at on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error));
        return false;
      }

      auto [keyid, error2] = doc["id"].as_int64_t();
      if (error2) {
        printf("Error getting id as int64 on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error2));
        return false;
      }

      if (keyid != int64_t(count)) {
        printf("key does not match %ld, expected %zd on document %zd at batch size %zu\n", keyid, count, count, batch_size);
        return false;
      }

      count++;
    }
    if(count != n_records) {
      printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
      return false;
    }
  }
  printf("ok\n");
  return true;
}

// returns true if successful
bool document_stream_utf8_test() {
  printf("Running document_stream_utf8_test");
  fflush(NULL);
  const size_t n_records = 10000;
  std::string data;
  char buf[1024];
  for (size_t i = 0; i < n_records; ++i) {
    auto n = sprintf(buf,
                     "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                     "\"été\": {\"id\": %zu, \"name\": \"éventail%zu\"}}",
                     i, i, (i % 2) ? "⺃" : "⺕", i % 10, i % 10);
    data += std::string(buf, n);
  }
  for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
    printf(".");
    fflush(NULL);
    simdjson::padded_string str(data);
    simdjson::document::parser parser;
    size_t count = 0;
    for (auto [doc, error] : parser.parse_many(str, batch_size)) {
      if (error) {
        printf("Error at on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error));
        return false;
      }

      auto [keyid, error2] = doc["id"].as_int64_t();
      if (error2) {
        printf("Error getting id as int64 on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error2));
        return false;
      }

      if (keyid != int64_t(count)) {
        printf("key does not match %ld, expected %zd on document %zd at batch size %zu\n", keyid, count, count, batch_size);
        return false;
      }

      count++;
    }
    if(count != n_records) {
      printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
      return false;
    }
  }
  printf("ok\n");
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
  simdjson::document::parser parser;
  size_t counter = 0;
  for (auto &rec : data) {
    if ((counter % 10000) == 0) {
      printf(".");
      fflush(NULL);
    }
    counter++;
    auto ok1 = json_parse(rec.c_str(), rec.length(), parser);
    if (ok1 != 0 || !parser.is_valid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
    auto ok2 = json_parse(rec, parser);
    if (ok2 != 0 || !parser.is_valid()) {
      printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
      return false;
    }
  }
  printf("\n");
  return true;
}

namespace dom_api {
  using namespace std;
  using namespace simdjson;
  bool object_iterator() {
    string json(R"({ "a": 1, "b": 2, "c": 3 })");
    const char* expected_key[] = { "a", "b", "c" };
    uint64_t expected_value[] = { 1, 2, 3 };
    int i = 0;

    document doc = document::parse(json);
    for (auto [key, value] : document::object(doc)) {
      if (key != expected_key[i] || uint64_t(value) != expected_value[i]) { cerr << "Expected " << expected_key[i] << " = " << expected_value[i] << ", got " << key << "=" << uint64_t(value) << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool array_iterator() {
    string json(R"([ 1, 10, 100 ])");
    uint64_t expected_value[] = { 1, 10, 100 };
    int i=0;

    document doc = document::parse(json);
    for (uint64_t value : doc.as_array()) {
      if (value != expected_value[i]) { cerr << "Expected " << expected_value[i] << ", got " << value << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool object_iterator_empty() {
    string json(R"({})");
    int i = 0;

    document doc = document::parse(json);
    for (auto [key, value] : doc.as_object()) {
      cout << "Unexpected " << key << " = " << uint64_t(value) << endl;
      i++;
    }
    if (i > 0) { cout << "Expected 0 values, got " << i << endl; return false; }
    return true;
  }

  bool array_iterator_empty() {
    string json(R"([])");
    int i=0;

    document doc = document::parse(json);
    for (uint64_t value : doc.as_array()) {
      cout << "Unexpected value " << value << endl;
      i++;
    }
    if (i > 0) { cout << "Expected 0 values, got " << i << endl; return false; }
    return true;
  }

  bool string_value() {
    string json(R"([ "hi", "has backslash\\" ])");
    document doc = document::parse(json);
    auto val = document::array(doc).begin();
    if (strcmp((const char*)*val, "hi")) { cerr << "Expected const char*(\"hi\") to be \"hi\", was " << (const char*)*val << endl; return false; }
    if (string_view(*val) != "hi") { cerr << "Expected string_view(\"hi\") to be \"hi\", was " << string_view(*val) << endl; return false; }
    ++val;
    if (strcmp((const char*)*val, "has backslash\\")) { cerr << "Expected const char*(\"has backslash\\\\\") to be \"has backslash\\\", was " << (const char*)*val << endl; return false; }
    if (string_view(*val) != "has backslash\\") { cerr << "Expected string_view(\"has backslash\\\\\") to be \"has backslash\\\", was " << string_view(*val) << endl; return false; }
    return true;
  }

  bool numeric_values() {
    string json(R"([ 0, 1, -1, 1.1 ])");
    document doc = document::parse(json);
    auto val = document::array(doc).begin();
    if (uint64_t(*val) != 0) { cerr << "Expected uint64_t(0) to be 0, was " << uint64_t(*val) << endl; return false; }
    if (int64_t(*val) != 0) { cerr << "Expected int64_t(0) to be 0, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != 0) { cerr << "Expected double(0) to be 0, was " << double(*val) << endl; return false; }
    ++val;
    if (uint64_t(*val) != 1) { cerr << "Expected uint64_t(1) to be 1, was " << uint64_t(*val) << endl; return false; }
    if (int64_t(*val) != 1) { cerr << "Expected int64_t(1) to be 1, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != 1) { cerr << "Expected double(1) to be 1, was " << double(*val) << endl; return false; }
    ++val;
    if (int64_t(*val) != -1) { cerr << "Expected int64_t(-1) to be -1, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != -1) { cerr << "Expected double(-1) to be -1, was " << double(*val) << endl; return false; }
    ++val;
    if (double(*val) != 1.1) { cerr << "Expected double(1.1) to be 1.1, was " << double(*val) << endl; return false; }
    return true;
  }

  bool boolean_values() {
    string json(R"([ true, false ])");
    document doc = document::parse(json);
    auto val = document::array(doc).begin();
    if (bool(*val) != true) { cerr << "Expected bool(true) to be true, was " << bool(*val) << endl; return false; }
    ++val;
    if (bool(*val) != false) { cerr << "Expected bool(false) to be false, was " << bool(*val) << endl; return false; }
    return true;
  }

  bool null_value() {
    string json(R"([ null ])");
    document doc = document::parse(json);
    auto val = document::array(doc).begin();
    if (!(*val).is_null()) { cerr << "Expected null to be null!" << endl; return false; }
    return true;
  }

  bool document_object_index() {
    string json(R"({ "a": 1, "b": 2, "c": 3})");
    document doc = document::parse(json);
    if (uint64_t(doc["a"]) != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << uint64_t(doc["a"]) << endl; return false; }
    if (uint64_t(doc["b"]) != 2) { cerr << "Expected uint64_t(doc[\"b\"]) to be 2, was " << uint64_t(doc["b"]) << endl; return false; }
    if (uint64_t(doc["c"]) != 3) { cerr << "Expected uint64_t(doc[\"c\"]) to be 3, was " << uint64_t(doc["c"]) << endl; return false; }
    // Check all three again in backwards order, to ensure we can go backwards
    if (uint64_t(doc["c"]) != 3) { cerr << "Expected uint64_t(doc[\"c\"]) to be 3, was " << uint64_t(doc["c"]) << endl; return false; }
    if (uint64_t(doc["b"]) != 2) { cerr << "Expected uint64_t(doc[\"b\"]) to be 2, was " << uint64_t(doc["b"]) << endl; return false; }
    if (uint64_t(doc["a"]) != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << uint64_t(doc["a"]) << endl; return false; }

    auto [val, error] = doc["d"];
    if (error != simdjson::NO_SUCH_FIELD) { cerr << "Expected NO_SUCH_FIELD error for uint64_t(doc[\"d\"]), got " << error << endl; return false; }
    return true;
  }

  bool object_index() {
    string json(R"({ "obj": { "a": 1, "b": 2, "c": 3 } })");
    document doc = document::parse(json);
    if (uint64_t(doc["obj"]["a"]) != 1) { cerr << "Expected uint64_t(doc[\"obj\"][\"a\"]) to be 1, was " << uint64_t(doc["obj"]["a"]) << endl; return false; }
    document::object obj = doc["obj"];
    if (uint64_t(obj["a"]) != 1) { cerr << "Expected uint64_t(obj[\"a\"]) to be 1, was " << uint64_t(obj["a"]) << endl; return false; }
    if (uint64_t(obj["b"]) != 2) { cerr << "Expected uint64_t(obj[\"b\"]) to be 2, was " << uint64_t(obj["b"]) << endl; return false; }
    if (uint64_t(obj["c"]) != 3) { cerr << "Expected uint64_t(obj[\"c\"]) to be 3, was " << uint64_t(obj["c"]) << endl; return false; }
    // Check all three again in backwards order, to ensure we can go backwards
    if (uint64_t(obj["c"]) != 3) { cerr << "Expected uint64_t(obj[\"c\"]) to be 3, was " << uint64_t(obj["c"]) << endl; return false; }
    if (uint64_t(obj["b"]) != 2) { cerr << "Expected uint64_t(obj[\"b\"]) to be 2, was " << uint64_t(obj["b"]) << endl; return false; }
    if (uint64_t(obj["a"]) != 1) { cerr << "Expected uint64_t(obj[\"a\"]) to be 1, was " << uint64_t(obj["a"]) << endl; return false; }

    auto [val, error] = obj["d"];
    if (error != simdjson::NO_SUCH_FIELD) { cerr << "Expected NO_SUCH_FIELD error for uint64_t(obj[\"d\"]), got " << error << endl; return false; }
    return true;
  }

  bool twitter_count() {
    // Prints the number of results in twitter.json
    document doc = document::parse(get_corpus(JSON_TEST_PATH));
    uint64_t result_count = doc["search_metadata"]["count"];
    if (result_count != 100) { cerr << "Expected twitter.json[metadata_count][count] = 100, got " << result_count << endl; return false; }
    return true;
  }

  bool twitter_default_profile() {
    // Print users with a default profile.
    set<string_view> default_users;
    document doc = document::parse(get_corpus(JSON_TEST_PATH));
    for (document::object tweet : doc["statuses"].as_array()) {
      document::object user = tweet["user"];
      if (user["default_profile"]) {
        default_users.insert(user["screen_name"]);
      }
    }
    if (default_users.size() != 86) { cerr << "Expected twitter.json[statuses][user] to contain 86 default_profile users, got " << default_users.size() << endl; return false; }
    return true;
  }

  bool twitter_image_sizes() {
    // Print image names and sizes
    set<tuple<uint64_t, uint64_t>> image_sizes;
    document doc = document::parse(get_corpus(JSON_TEST_PATH));
    for (document::object tweet : doc["statuses"].as_array()) {
      auto [media, not_found] = tweet["entities"]["media"];
      if (!not_found) {
        for (document::object image : media.as_array()) {
          for (auto [key, size] : image["sizes"].as_object()) {
            image_sizes.insert({ size["w"], size["h"] });
          }
        }
      }
    }
    if (image_sizes.size() != 15) { cerr << "Expected twitter.json[statuses][entities][media][sizes] to contain 15 different sizes, got " << image_sizes.size() << endl; return false; }
    return true;
  }

  bool run_tests() {
    if (!object_iterator()) { return false; }
    if (!array_iterator()) { return false; }
    if (!object_iterator_empty()) { return false; }
    if (!array_iterator_empty()) { return false; }
    if (!string_value()) { return false; }
    if (!numeric_values()) { return false; }
    if (!boolean_values()) { return false; }
    if (!null_value()) { return false; }
    if (!document_object_index()) { return false; }
    if (!object_index()) { return false; }
    if (!twitter_count()) { return false; }
    if (!twitter_default_profile()) { return false; }
    if (!twitter_image_sizes()) { return false; }
    return true;
  }
}

bool error_messages_in_correct_order() {
  using namespace simdjson;
  using namespace simdjson::internal;
  using namespace std;
  if ((sizeof(error_codes)/sizeof(error_code_info)) != NUM_ERROR_CODES) {
    cerr << "error_codes does not have all codes in error_code enum (or too many)" << endl;
    return false;
  }
  for (int i=0; i<NUM_ERROR_CODES; i++) {
    if (error_codes[i].code != i) {
      cerr << "Error " << int(error_codes[i].code) << " at wrong position (" << i << "): " << error_codes[i].message << endl;
      return false;
    }
  }
  return true;
}

int main() {
  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") { 
    printf("unsupported CPU\n"); 
  }
  std::cout << "Running basic tests." << std::endl;
  if(!json_issue467())
    return EXIT_FAILURE;
  if(!number_test_small_integers())
    return EXIT_FAILURE;
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
  if (!dom_api::run_tests())
    return EXIT_FAILURE;
  if(!document_stream_test())
    return EXIT_FAILURE;
  if(!document_stream_utf8_test())
    return EXIT_FAILURE;
  if(!JsonStream_test())
    return EXIT_FAILURE;
  if(!JsonStream_utf8_test())
    return EXIT_FAILURE;
  if(!error_messages_in_correct_order())
    return EXIT_FAILURE;
  std::cout << "Basic tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
