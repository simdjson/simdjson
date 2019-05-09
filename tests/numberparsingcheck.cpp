#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef JSON_TEST_NUMBERS
#define JSON_TEST_NUMBERS
#endif

#include "simdjson/common_defs.h"

int parse_error;
char *fullpath;
enum { PARSE_WARNING, PARSE_ERROR };

size_t float_count;
size_t int_count;
size_t invalid_count;

// strings that start with these should not be parsed as numbers
const char *really_bad[] = {"013}", "0x14", "0e]", "0e+]", "0e+-1]"};

bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre);
  return strncmp(pre, str, lenpre) == 0;
}

bool is_in_bad_list(const char *buf) {
  if(buf[0] != '0') return false;
  for (size_t i = 0; i < sizeof(really_bad) / sizeof(really_bad[0]); i++)
    if (startsWith(really_bad[i], buf))
      return true;
  return false;
}

inline void foundInvalidNumber(const uint8_t *buf) {
  invalid_count++;
  char *endptr;
  double expected = strtod((const char *)buf, &endptr);
  if (endptr != (const char *)buf) {
    if (!is_in_bad_list((const char *)buf)) {
      printf(
          "Warning: foundInvalidNumber %.32s whereas strtod parses it to %f, ",
          buf, expected);
      printf(" while parsing %s \n", fullpath);
      parse_error |= PARSE_WARNING;
    }
  }
}

inline void foundInteger(int64_t result, const uint8_t *buf) {
  int_count++;
  char *endptr;
  long long expected = strtoll((const char *)buf, &endptr, 10);
  if ((endptr == (const char *)buf) || (expected != result)) {
    fprintf(stderr, "Error: parsed %" PRId64 " out of %.32s, ", result, buf);
    fprintf(stderr, " while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
}

inline void foundFloat(double result, const uint8_t *buf) {
  char *endptr;
  float_count++;
  double expected = strtod((const char *)buf, &endptr);
  if (endptr == (const char *)buf) {
    fprintf(stderr, "parsed %f from %.32s whereas strtod refuses to parse a float, ",
           result, buf);
    fprintf(stderr, " while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
  if( fpclassify(expected) != fpclassify(result) ) {
    fprintf(stderr, "floats not in the same category expected: %f observed: %f \n", expected, result);
    fprintf(stderr, "%.32s\n", buf);
    parse_error |= PARSE_ERROR;
  }
  // we want to get some reasonable relative accuracy
  else if (fabs(expected - result)  >
      1e-14 * fmin(fabs(expected), fabs(result))) {
    fprintf(stderr, "parsed %.128e from \n", result);
    fprintf(stderr, "       %.32s whereas strtod gives\n", buf);
    fprintf(stderr, "       %.128e,", expected);
    fprintf(stderr, " while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
}

#include "simdjson/jsonparser.h"
#include "src/stage2_build_tape.cpp"

/**
 * Does the file filename ends with the given extension.
 */
static bool hasExtension(const char *filename, const char *extension) {
  const char *ext = strrchr(filename, '.');
  return (ext && !strcmp(ext, extension));
}

bool validate(const char *dirname) {
  parse_error = 0;
  size_t total_count = 0;
  const char *extension = ".json";
  size_t dirlen = strlen(dirname);
  struct dirent **entry_list;
  int c = scandir(dirname, &entry_list, 0, alphasort);
  if (c < 0) {
    printf("error accessing %s \n", dirname);
    return false;
  }
  if (c == 0) {
    printf("nothing in dir %s \n", dirname);
    return false;
  }
  bool needsep = (strlen(dirname) > 1) && (dirname[strlen(dirname) - 1] != '/');
  for (int i = 0; i < c; i++) {
    const char *name = entry_list[i]->d_name;
    if (hasExtension(name, extension)) {
      size_t filelen = strlen(name);
      fullpath = (char *)malloc(dirlen + filelen + 1 + 1);
      strcpy(fullpath, dirname);
      if (needsep) {
        fullpath[dirlen] = '/';
        strcpy(fullpath + dirlen + 1, name);
      } else {
        strcpy(fullpath + dirlen, name);
      }
      padded_string p;
      try {
        get_corpus(fullpath).swap(p);
      } catch (const std::exception& e) { 
        std::cout << "Could not load the file " << fullpath << std::endl;
        return EXIT_FAILURE;
      }
      // terrible hack but just to get it working
      ParsedJson pj;
      bool allocok = pj.allocateCapacity(p.size(), 1024);
      if (!allocok) {
        std::cerr << "can't allocate memory" << std::endl;
        return false;
      }
      float_count = 0;
      int_count = 0;
      invalid_count = 0;
      total_count += float_count + int_count + invalid_count;
      bool isok = json_parse(p, pj);
      if (int_count + float_count + invalid_count > 0) {
        printf("File %40s %s --- integers: %10zu floats: %10zu invalid: %10zu "
               "total numbers: %10zu \n",
               name, isok ? " is valid     " : " is not valid ", int_count,
               float_count, invalid_count,
               int_count + float_count + invalid_count);
      }
      free(fullpath);
    }
  }
  if ((parse_error & PARSE_ERROR) != 0) {
    printf("NUMBER PARSING FAILS?\n");
  } else {
    printf("All ok.\n");
  }
  for (int i = 0; i < c; ++i)
    free(entry_list[i]);
  free(entry_list);
  return ((parse_error & PARSE_ERROR) == 0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <directorywithjsonfiles>"
              << std::endl;
#if defined(SIMDJSON_TEST_DATA_DIR) &&  defined(SIMDJSON_BENCHMARK_DATA_DIR) 
    std::cout
        << "We are going to assume you mean to use the '"<< SIMDJSON_TEST_DATA_DIR <<"'  and  '"<< SIMDJSON_BENCHMARK_DATA_DIR <<"'directories."
        << std::endl;
    return validate(SIMDJSON_TEST_DATA_DIR) && validate(SIMDJSON_BENCHMARK_DATA_DIR) ? EXIT_SUCCESS
                                                                 : EXIT_FAILURE;
#else
    std::cout << "We are going to assume you mean to use the 'jsonchecker' and "
                 "'jsonexamples' directories."
              << std::endl;
    return validate("jsonchecker/") && validate("jsonexamples/") ? EXIT_SUCCESS
                                                                 : EXIT_FAILURE;
#endif
  }
  return validate(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
