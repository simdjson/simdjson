#include <assert.h>
#include <cstring>
#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#else
// Microsoft can't be bothered to provide standard utils.
#include <dirent_portable.h>
#endif
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "simdjson/jsonparser.h"

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

bool contains(const char *pre, const char *str) {
  return (strstr(str, pre) != NULL);
}


bool validate(const char *dirname) {
  bool everythingfine = true;
  const char *extension = ".json";
  size_t dirlen = strlen(dirname);
  struct dirent **entry_list;
  int c = scandir(dirname, &entry_list, 0, alphasort);
  if (c < 0) {
    fprintf(stderr, "error accessing %s \n", dirname);
    return false;
  }
  if (c == 0) {
    printf("nothing in dir %s \n", dirname);
    return false;
  }
  bool * isfileasexpected = new bool[c];
  for(int i = 0; i < c; i++) isfileasexpected[i] = true;
  size_t howmany = 0;
  bool needsep = (strlen(dirname) > 1) && (dirname[strlen(dirname) - 1] != '/');
  for (int i = 0; i < c; i++) {
    const char *name = entry_list[i]->d_name;
    if (hasExtension(name, extension)) {
      printf("validating: file %s ", name);
      fflush(NULL);
      size_t filelen = strlen(name);
      char *fullpath = (char *)malloc(dirlen + filelen + 1 + 1);
      strcpy(fullpath, dirname);
      if (needsep) {
        fullpath[dirlen] = '/';
        strcpy(fullpath + dirlen + 1, name);
      } else {
        strcpy(fullpath + dirlen, name);
      }
      std::string_view p;
      try {
        p = get_corpus(fullpath);
      } catch (const std::exception& e) {
        std::cerr << "Could not load the file " << fullpath << std::endl;
        return EXIT_FAILURE;
      }
      ParsedJson pj;
      bool allocok = pj.allocateCapacity(p.size(), 1024);
      if(!allocok) {
        std::cerr << "can't allocate memory"<<std::endl;
        return false;
      }
      ++howmany;
      bool isok = json_parse(p, pj);
      aligned_free((void*)p.data());
      printf("%s\n", isok ? "ok" : "invalid");
      if(contains("EXCLUDE",name)) {
        // skipping
        howmany--;
      } else if (startsWith("pass", name)) {
        if (!isok) {
          isfileasexpected[i] = false;
          printf("warning: file %s should pass but it fails.\n", name);
          everythingfine = false;
        }
      } else if (startsWith("fail", name)) {
        if (isok) {
          isfileasexpected[i] = false;
          printf("warning: file %s should fail but it passes.\n", name);
          everythingfine = false;
        }
      }
      free(fullpath);
    }
  }
  printf("%zu files checked.\n", howmany);
  if(everythingfine) {
    printf("All ok!\n");
  } else {
    fprintf(stderr, "There were problems! Consider reviewing the following files:\n");
    for(int i = 0; i < c; i++) {
      if(!isfileasexpected[i]) fprintf(stderr, "%s \n", entry_list[i]->d_name);
    }
  }
  for (int i = 0; i < c; ++i)
    free(entry_list[i]);
  free(entry_list);
  delete[] isfileasexpected;
  return everythingfine;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <directorywithjsonfiles>"
              << std::endl;
#ifndef SIMDJSON_TEST_DATA_DIR
    std::cout
        << "We are going to assume you mean to use the 'jsonchecker' directory."
        << std::endl;
    return validate("jsonchecker/") ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    std::cout
        << "We are going to assume you mean to use the '"<< SIMDJSON_TEST_DATA_DIR <<"' directory."
        << std::endl;
    return validate(SIMDJSON_TEST_DATA_DIR) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
  }
  return validate(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
