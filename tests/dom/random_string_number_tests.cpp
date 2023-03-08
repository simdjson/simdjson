#include <cstring>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <random>
#include <climits>
#include <unistd.h>

#include "simdjson.h"


/**
 * Some systems have bad floating-point parsing. We want to exclude them.
 */
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined (__linux__) || defined (__APPLE__) || defined(__FreeBSD__)
// Ok. So under Visual Studio, linux, apple and freebsd systems, we have a good chance of having a decent
// enough strtod. It is not certain, but it is maybe a good enough heuristics. We exclude systems like msys2
// or cygwin.
//
// Finally, we want to exclude legacy 32-bit systems.
#if SIMDJSON_IS_32BITS
// we omit 32-bit tests
#else
// So we only run some of the floating-point tests under 64-bit linux, apple, regular visual studio, freebsd.
#define TEST_FLOATS
// Apple and freebsd need a special header, typically.
#if defined __APPLE__ || defined(__FreeBSD__)
#  include <xlocale.h>
#endif

#endif

#endif


struct RandomEngine {
   RandomEngine() = delete;
   RandomEngine(uint32_t seed) : one_zero_generator(0,1), digit_generator(0,9),  nonzero_digit_generator(1,9), digit_count_generator (1,40),exp_count_generator (1,3), generator(seed) {}
   std::uniform_int_distribution<int> one_zero_generator;
   std::uniform_int_distribution<int> digit_generator;
   std::uniform_int_distribution<int> nonzero_digit_generator;

   std::uniform_int_distribution<int> digit_count_generator;
   std::uniform_int_distribution<int> exp_count_generator;
   bool next_bool() { return one_zero_generator(generator); }
   int next_digit() { return digit_generator(generator); }
   int next_nonzero_digit() { return nonzero_digit_generator(generator); }
   int next_digit_count() { return digit_count_generator(generator); }
   int next_exp_count() { return exp_count_generator(generator); }

   std::mt19937 generator;
};

size_t build_random_string(RandomEngine &rand, char *buffer) {
  size_t pos{0};
  if (rand.next_bool()) {
    buffer[pos++] = '-';
  }
  size_t number_of_digits = size_t(rand.next_digit_count());
  std::uniform_int_distribution<int> decimal_generator(1,int(number_of_digits));
  size_t location_of_decimal_separator = size_t(decimal_generator(rand.generator));
  for (size_t i = 0; i < number_of_digits; i++) {
    if (i == location_of_decimal_separator) {
      buffer[pos++] = '.';
    }
    if (( i == 0) && (location_of_decimal_separator != 1)) {
      buffer[pos++] = char(rand.next_nonzero_digit() + '0');
    } else {
      buffer[pos++] = char(rand.next_digit() + '0');
    }
  }
  if (rand.next_bool()) {
    if (rand.next_bool()) {
      buffer[pos++] = 'e';
    } else {
      buffer[pos++] = 'E';
    }
    if (rand.next_bool()) {
      buffer[pos++] = '-';
    } else {
      if (rand.next_bool()) {
        buffer[pos++] = '+';
      }
    }
    number_of_digits = rand.next_exp_count();
    size_t i = 0;
    if(number_of_digits > 0) {
        buffer[pos++] = char(rand.next_nonzero_digit() + '0');
        i++;
    }
    for (; i < number_of_digits; i++) {
      buffer[pos++] = char(rand.next_digit() + '0');
    }
  }
  buffer[pos] = '\0'; // null termination
  return pos;
}


#ifndef TEST_FLOATS
// We do not recognize the system, so we do not verify our results.
bool check_float(double , const char *) {
    return true;
}
#else
bool check_float(double result, const char *buf) {
  char *endptr;
#ifdef _WIN32
  static _locale_t c_locale = _create_locale(LC_ALL, "C");
  double expected = _strtod_l((const char *)buf, &endptr, c_locale);
#else
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
  double expected = strtod_l((const char *)buf, &endptr, c_locale);
#endif
  if (endptr == (const char *)buf) {
    fprintf(stderr,
            "parsed %f from %.32s whereas strtod refuses to parse a float, ",
            result, buf);
    return false;
  }
  if (expected != result) {
    std::cerr << std::hexfloat << " parsed " << result << " from "
              << buf << " whereas strtod gives " << expected << std::endl;
    std::cerr << std::defaultfloat;
    return false;
  }
  return true;
}
#endif


/**
 * We generate random strings and we try to parse them,
 * and we verify that we get the same answer.
 */
bool tester(int seed, size_t volume) {
  std::vector<char> buffer(1024); // large buffer (can't overflow)
  simdjson::dom::parser parser;
  RandomEngine rand(seed);
  double result{};
  for (size_t i = 0; i < volume; i++) {
    if((i%100000) == 0) { std::cout << "."; std::cout.flush(); }
    size_t length = build_random_string(rand, buffer.data());
    auto error = parser.parse(buffer.data(), length).get(result);
    // When we parse a (finite) number, it better match strtod.
    if ((!error) && (!check_float(result, buffer.data()))) { return false; }
  }
  return true;
}

int main(int argc, char *argv[]) {
  // We test 1,000,000 random strings by default.
  // You can specify more tests with the '-m' flag if you want.
  size_t howmany = 1000000;

  int c;
  while ((c = getopt(argc, argv, "a:m:h")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::get_available_implementations()[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      if(!impl->supported_by_runtime_system()) {
        fprintf(stderr, "The selected implementation does not match your current CPU: -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::get_active_implementation() = impl;
      break;
    }
    case 'h': {
      std::cout << "-a to select an architecture" << std::endl;
      std::cout << "-m to select a number of tests" << std::endl;
      return EXIT_SUCCESS;
    }
    case 'm': {
      long long requested_howmany = atoll(optarg);
      if(requested_howmany <= 0) {
        fprintf(stderr, "Please provide a positive number of tests -m %s no larger than %lld \n", optarg, LLONG_MAX);
        return EXIT_FAILURE;
      }
      howmany = size_t(requested_howmany);
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }
  if (tester(1234344, howmany)) {
    std::cout << "All tests ok." << std::endl;
    return EXIT_SUCCESS;
  }
  std::cout << "Failure." << std::endl;
  return EXIT_FAILURE;
}
