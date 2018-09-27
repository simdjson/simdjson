#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifndef JSON_TEST_NUMBERS
#define JSON_TEST_NUMBERS
#endif 

#include "jsonparser/common_defs.h"


inline void foundInvalidNumber(const u8 * buf) {
  printf("foundInvalidNumber %.64s \n", buf);
}

inline void foundInteger(int64_t result, const u8 * buf) {
  printf("parsed %" PRId64 " out of %.64s \n", result, buf);
}

inline void foundFloat(double result, const u8 * buf) {
  printf("parsed %f from %.64s \n", result, buf);
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

bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre), lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool validate(const char *dirname) {
  bool everythingfine = true;
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
      printf("validating numbers in file %s \n", name);
      size_t filelen = strlen(name);
      char *fullpath = (char *)malloc(dirlen + filelen + 1 + 1);
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
      if(pj_ptr == NULL) {
        std::cerr<< "can't allocate memory"<<std::endl;
        return false;
      }
      ParsedJson &pj(*pj_ptr);
      bool isok = json_parse(p.first, p.second, pj);
      printf("File %s %s.\n", name,
               isok ? " is valid JSON " : " is not valid JSON");
      free(p.first);
      free(fullpath);
      deallocate_ParsedJson(pj_ptr);
    }
  }
  for (int i = 0; i < c; ++i)
    free(entry_list[i]);
  free(entry_list);
  return everythingfine;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <directorywithjsonfiles>"
              << std::endl;
    std::cout
        << "We are going to assume you mean to use the 'jsonchecker' directory."
        << std::endl;
    return validate("jsonchecker/") ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  return validate(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
