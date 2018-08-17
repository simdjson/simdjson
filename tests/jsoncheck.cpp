#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common_defs.h"
#include "jsonioutil.h"
#include "simdjson_internal.h"
#include "stage1_find_marks.h"
#include "stage2_flatten.h"
#include "stage3_ape_machine.h"
#include "stage4_shovel_machine.h"

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
  init_state_machine(); // to be safe
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
      printf("validating: file %s \n", name);
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
      ParsedJson *pj_ptr = new ParsedJson;
      ParsedJson &pj(*pj_ptr);
      if (posix_memalign((void **)&pj.structurals, 8,
                         ROUNDUP_N(p.second, 64) / 8)) {
        std::cerr << "Could not allocate memory" << std::endl;
        return false;
      };
      pj.n_structural_indexes = 0;
      u32 max_structures = ROUNDUP_N(p.second, 64) + 2 + 7;
      pj.structural_indexes = new u32[max_structures];
      bool isok = find_structural_bits(p.first, p.second, pj);
      if (isok) {
        isok = flatten_indexes(p.second, pj);
      }
      if (isok) {
        isok = ape_machine(p.first, p.second, pj);
      }
      if (isok) {
        isok = shovel_machine(p.first, p.second, pj);
      }
      if (startsWith("pass", name)) {
        if (!isok) {
          printf("warning: file %s should pass but it fails.\n", name);
          everythingfine = false;
        }
      } else if (startsWith("fail", name)) {
        if (isok) {
          printf("warning: file %s should fail but it passes.\n", name);
          everythingfine = false;
        }
      } else {
        printf("File %s %s.\n", name,
               isok ? " is valid JSON " : " is not valid JSON");
      }
      free(pj.structurals);
      free(p.first);
      delete[] pj.structural_indexes;
      free(fullpath);
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
