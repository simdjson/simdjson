#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "simdjson/jsonparser.h"

static inline uint64_t splitmix64_stateless(uint64_t index) {
  uint64_t z = (index + UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}


// inspired by: https://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/


// we should be able to get fast and perfect
// parsing for s * p where 
// 10^0 <= p < 10^22 and
// 0 <= s < 10^15 where s is not divisible by 10
bool fast_path_test(uint64_t index) {
  uint64_t random = splitmix64_stateless(index);
  uint64_t base = random % UINT64_C(1000000000000000);
  if((base % 10 ) == 0) base = (base + 1) % UINT64_C(0x01fffffffffffff);
  double s = (double)(base);
  if((uint64_t)s != base) {
    printf("logic failure in fast_path_test\n");
    return false;    
  }
  int exponent = (random>>53) % 23;
  double p = pow(10, exponent);
  double val = s * p;
  char buf[1024];
  char * end = buf + sprintf(buf,"%.15g", val);
  *end = '\0'; // to be sure
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(end - buf)) {
    printf("allocation failure in fast_path_test\n");
    return false;
  }
  bool ok = json_parse(buf, end - buf, pj);
  if (ok != 0 || !pj.is_valid()) {
      printf("Something is wrong in fast_path_test: %s.\n", buf);
      return false;
  }
  simdjson::ParsedJson::Iterator pjh(pj);
  if (!pjh.is_ok()) {
      printf("Something is wrong in fast_path_test: %s.\n", buf);
      return false;
  }
  if(pjh.is_double()) {
    double recovered_value = pjh.get_double();
    return recovered_value == val;
  } else {
    double recovered_value = (double) pjh.get_integer();
    return recovered_value == val;
  }
}

// we should be able to get fast and perfect
// parsing for s * p where 
// 10^-22 <= p < 10^0 and
// 0 <= s < 10^15  where s is not divisible by 10
bool fast_path_negexp_test(uint64_t index) {
  uint64_t random = splitmix64_stateless(index);
  uint64_t base = random % UINT64_C(1000000000000000);
  if((base % 10 ) == 0) base = (base + 1) % UINT64_C(0x01fffffffffffff);
  double s = (double)(base);
  if((uint64_t)s != base) {
    printf("logic failure in fast_path_test\n");
    return false;    
  }
  int exponent = (random>>53) % 23;
  double p = pow(10, -exponent);
  double val = s * p;
  char buf[1024];
  char * end = buf + sprintf(buf,"%.15g", val);
  *end = '\0'; // to be sure
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(end - buf)) {
    printf("allocation failure in fast_path_negexp_test\n");
    return false;
  }
  bool ok = json_parse(buf, end - buf, pj);
  if (ok != 0 || !pj.is_valid()) {
      printf("Something is wrong in fast_path_negexp_test: %s.\n", buf);
      return false;
  }
  simdjson::ParsedJson::Iterator pjh(pj);
  if (!pjh.is_ok()) {
      printf("Something is wrong in fast_path_negexp_test: %s.\n", buf);
      return false;
  }
  if(pjh.is_double()) {
    double recovered_value = pjh.get_double();
    return recovered_value == val;
  } else {
    double recovered_value = (double) pjh.get_integer();
    return recovered_value == val;
  }
}



// we should be able to get fast and perfect
// parsing for s * p where 
// 10^0 <= p < 10^22 and
// 0 <= -s < 10^15 where s is not divisible by 10
bool fast_negpath_test(uint64_t index) {
  uint64_t random = splitmix64_stateless(index);
  uint64_t base = random % UINT64_C(1000000000000000);
  if((base % 10 ) == 0) base = (base + 1) % UINT64_C(0x01fffffffffffff);
  double s = (double)(base);
  if((uint64_t)s != base) {
    printf("logic failure in fast_negpath_test\n");
    return false;    
  }
  int exponent = (random>>53) % 23;
  double p = pow(10, exponent);
  double val = s * p;
  char buf[1024];
  char * end = buf + sprintf(buf,"%.15g", val);
  *end = '\0'; // to be sure
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(end - buf)) {
    printf("allocation failure in fast_negpath_test\n");
    return false;
  }
  bool ok = json_parse(buf, end - buf, pj);
  if (ok != 0 || !pj.is_valid()) {
      printf("Something is wrong in fast_negpath_test: %s.\n", buf);
      return false;
  }
  simdjson::ParsedJson::Iterator pjh(pj);
  if (!pjh.is_ok()) {
      printf("Something is wrong in fast_negpath_test: %s.\n", buf);
      return false;
  }
  if(pjh.is_double()) {
    double recovered_value = pjh.get_double();
    return recovered_value == val;
  } else {
    double recovered_value = (double) pjh.get_integer();
    return recovered_value == val;
  }
}

// we should be able to get fast and perfect
// parsing for s * p where 
// 10^-22 <= p < 10^0 and
// 0 <= -s < 10^15  where s is not divisible by 10
bool fast_negpath_negexp_test(uint64_t index) {
  uint64_t random = splitmix64_stateless(index);
  uint64_t base = random % UINT64_C(1000000000000000);
  if((base % 10 ) == 0) base = (base + 1) % UINT64_C(0x01fffffffffffff);
  double s = (double)(base);
  if((uint64_t)s != base) {
    printf("logic failure in fast_negpath_negexp_test\n");
    return false;    
  }
  int exponent = (random>>53) % 23;
  double p = pow(10, -exponent);
  double val = s * p;
  char buf[1024];
  char * end = buf + sprintf(buf,"%.15g", val);
  *end = '\0'; // to be sure
  simdjson::ParsedJson pj;
  if (!pj.allocate_capacity(end - buf)) {
    printf("allocation failure in fast_negpath_negexp_test\n");
    return false;
  }
  bool ok = json_parse(buf, end - buf, pj);
  if (ok != 0 || !pj.is_valid()) {
      printf("Something is wrong in fast_negpath_negexp_test: %s.\n", buf);
      return false;
  }
  simdjson::ParsedJson::Iterator pjh(pj);
  if (!pjh.is_ok()) {
      printf("Something is wrong in fast_negpath_negexp_test: %s.\n", buf);
      return false;
  }
  if(pjh.is_double()) {
    double recovered_value = pjh.get_double();
    return recovered_value == val;
  } else {
    double recovered_value = (double) pjh.get_integer();
    return recovered_value == val;
  }
}


int main() {
  std::cout << "Running fast_path_test." << std::endl;
  for(uint64_t index = 0; index < 100000; index ++) {
    if((index % 10000) == 0) {
      printf(".");
      fflush(NULL);
    }
    if(!fast_path_test(index)) {
      std::cerr << "error" << std::endl;
      return EXIT_FAILURE;
    }
    if(!fast_path_negexp_test(index)) {
      std::cerr << "error" << std::endl;
      return EXIT_FAILURE;
    }
    if(!fast_negpath_test(index)) {
      std::cerr << "error" << std::endl;
      return EXIT_FAILURE;
    }
    if(!fast_negpath_negexp_test(index)) {
      std::cerr << "error" << std::endl;
      return EXIT_FAILURE;
    }
  }
  std::cout << std::endl;
  std::cout << "fast_path_test is ok." << std::endl;
  return EXIT_SUCCESS;
}
