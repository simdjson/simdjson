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

#include "jsonparser/common_defs.h"

int parse_error;
char *fullpath;
enum { PARSE_WARNING, PARSE_ERROR };

size_t float_count;
size_t int_count;
size_t invalid_count;

// strings that start with these should not be parsed as numbers
const char *really_bad[] = {"013}", "0x14", "0e]", "0e+]", "0e+-1]"};

bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre), lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}
bool is_in_bad_list(char *buf) {
  for (size_t i = 0; i < sizeof(really_bad) / sizeof(really_bad[0]); i++)
    if (startsWith(really_bad[i], buf))
      return true;
  return false;
}

inline void foundInvalidNumber(const u8 *buf) {
  invalid_count++;
  char *endptr;
  double expected = strtod((char *)buf, &endptr);
  if (endptr != (char *)buf) {
    if (!is_in_bad_list((char *)buf)) {
      printf(
          "Warning: foundInvalidNumber %.32s whereas strtod parses it to %f, ",
          buf, expected);
      printf(" while parsing %s \n", fullpath);
      parse_error |= PARSE_WARNING;
    }
  }
}

inline void foundInteger(int64_t result, const u8 *buf) {
  int_count++;
  char *endptr;
  long long expected = strtoll((char *)buf, &endptr, 10);
  if ((endptr == (char *)buf) || (expected != result)) {
    printf("Error: parsed %" PRId64 " out of %.32s, ", result, buf);
    printf(" while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
}

inline void foundFloat(double result, const u8 *buf) {
  char *endptr;
  float_count++;
  double expected = strtod((char *)buf, &endptr);
  if (endptr == (char *)buf) {
    printf("parsed %f from %.32s whereas strtod refuses to parse a float, ",
           result, buf);
    printf(" while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
  // we want to get some reasonable relative accuracy
  if (fabs(expected - result) / fmin(fabs(expected), fabs(result)) >
      0.000000000000001) {
    printf("parsed %.32f from \n", result);
    printf("       %.32s whereas strtod gives\n", buf);
    printf("       %.32f,", expected);
    printf(" while parsing %s \n", fullpath);
    parse_error |= PARSE_ERROR;
  }
}

#include "jsonparser/jsonparser.h"
#include "src/stage34_unified.cpp"

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

  // init_state_machine(); // no longer necessary
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
      std::pair<u8 *, size_t> p = get_corpus(fullpath);
      // terrible hack but just to get it working
      ParsedJson *pj_ptr = allocate_ParsedJson(p.second);
      if (pj_ptr == NULL) {
        std::cerr << "can't allocate memory" << std::endl;
        return false;
      }
      float_count = 0;
      int_count = 0;
      invalid_count = 0;
      total_count += float_count + int_count + invalid_count;
      ParsedJson &pj(*pj_ptr);
      bool isok = json_parse(p.first, p.second, pj);
      if (int_count + float_count + invalid_count > 0) {
        printf("File %40s %s --- integers: %10zu floats: %10zu invalid: %10zu "
               "total numbers: %10zu \n",
               name, isok ? " is valid     " : " is not valid ", int_count,
               float_count, invalid_count,
               int_count + float_count + invalid_count);
      }
      free(p.first);
      free(fullpath);
      deallocate_ParsedJson(pj_ptr);
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
    std::cout << "We are going to assume you mean to use the 'jsonchecker' and "
                 "'jsonexamples' directories."
              << std::endl;
    return validate("jsonchecker/") && validate("jsonexamples/") ? EXIT_SUCCESS
                                                                 : EXIT_FAILURE;
  }
  return validate(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
