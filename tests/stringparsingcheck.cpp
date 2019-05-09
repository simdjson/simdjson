#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#ifndef JSON_TEST_STRINGS
#define JSON_TEST_STRINGS
#endif

#include "simdjson/common_defs.h"

char *fullpath;

size_t bad_string;
size_t good_string;
size_t empty_string;

size_t total_string_length;
bool probable_bug;
// borrowed code (sajson?)

static inline bool read_hex(const char *p, unsigned &u) {
  unsigned v = 0;
  int i = 4;
  while (i--) {
    unsigned char c = *p++;
    if (c >= '0' && c <= '9') {
      c -= '0';
    } else if (c >= 'a' && c <= 'f') {
      c = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      c = c - 'A' + 10;
    } else {
      return false;
    }
    v = (v << 4) + c;
  }

  u = v;
  return true;
}

static inline void write_utf8(unsigned codepoint, char *&end) {
  if (codepoint < 0x80) {
    *end++ = codepoint;
  } else if (codepoint < 0x800) {
    *end++ = 0xC0 | (codepoint >> 6);
    *end++ = 0x80 | (codepoint & 0x3F);
  } else if (codepoint < 0x10000) {
    *end++ = 0xE0 | (codepoint >> 12);
    *end++ = 0x80 | ((codepoint >> 6) & 0x3F);
    *end++ = 0x80 | (codepoint & 0x3F);
  } else {
    assert(codepoint < 0x200000);
    *end++ = 0xF0 | (codepoint >> 18);
    *end++ = 0x80 | ((codepoint >> 12) & 0x3F);
    *end++ = 0x80 | ((codepoint >> 6) & 0x3F);
    *end++ = 0x80 | (codepoint & 0x3F);
  }
}

static bool parse_string(const char *p, char *output, char **end) {
  if (*p != '"')
    return false;
  p++;

  for (;;) {

    if ((*p >= 0 && *p < 0x20)) {
      return false; // unescaped
    }

    switch (*p) {
    case '"':
      *output = '\0'; // end
      *end = output;
      return true;
    case '\\':
      ++p;

      char replacement;
      switch (*p) {
      case '"':
        replacement = '"';
        goto replace;
      case '\\':
        replacement = '\\';
        goto replace;
      case '/':
        replacement = '/';
        goto replace;
      case 'b':
        replacement = '\b';
        goto replace;
      case 'f':
        replacement = '\f';
        goto replace;
      case 'n':
        replacement = '\n';
        goto replace;
      case 'r':
        replacement = '\r';
        goto replace;
      case 't':
        replacement = '\t';
        goto replace;
      replace:
        *output++ = replacement;
        ++p;
        break;
      case 'u': {
        ++p;
        unsigned u;
        if (!read_hex(p, u))
          return false;

        p += 4;
        if (u >= 0xD800 && u <= 0xDBFF) {
          char p0 = p[0];
          char p1 = p[1];
          if (p0 != '\\' || p1 != 'u') {
            return false;
          }
          p += 2;
          unsigned v;
          if (!read_hex(p, v))
            return false;

          p += 4;

          if (v < 0xDC00 || v > 0xDFFF) {
            return false;
          }
          u = 0x10000 + (((u - 0xD800) << 10) | (v - 0xDC00));
        }
        write_utf8(u, output);
        break;
      }
      default:
        return false;
      }
      break;

    default:
      // validate UTF-8
      unsigned char c0 = p[0];
      if (c0 < 128) {
        *output++ = *p++;
      } else if (c0 < 224) {
        unsigned char c1 = p[1];
        if (c1 < 128 || c1 >= 192) {
          return false;
        }
        output[0] = c0;
        output[1] = c1;
        output += 2;
        p += 2;
      } else if (c0 < 240) {
        unsigned char c1 = p[1];
        if (c1 < 128 || c1 >= 192) {
          return false;
        }
        unsigned char c2 = p[2];
        if (c2 < 128 || c2 >= 192) {
          return false;
        }
        output[0] = c0;
        output[1] = c1;
        output[2] = c2;
        output += 3;
        p += 3;
      } else if (c0 < 248) {
        unsigned char c1 = p[1];
        if (c1 < 128 || c1 >= 192) {
          return false;
        }
        unsigned char c2 = p[2];
        if (c2 < 128 || c2 >= 192) {
          return false;
        }
        unsigned char c3 = p[3];
        if (c3 < 128 || c3 >= 192) {
          return false;
        }
        output[0] = c0;
        output[1] = c1;
        output[2] = c2;
        output[3] = c3;
        output += 4;
        p += 4;
      } else {
        return false;
      }
      break;
    }
  }
}
// end of borrowed code
char * bigbuffer; // global variable

inline void foundBadString(const uint8_t *buf) {
  bad_string++;
  char *end;
  if (parse_string((const char *)buf, bigbuffer, &end)) {
    printf("WARNING: Sajson-like parser seems to think that the string is "
           "valid %32s \n",
           buf);
    probable_bug = true;
  }
}

void print_hex(const char *s, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%02x ", s[i] & 0xFF);
  }
}

void print_cmp_hex(const char *s1, const char *s2, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%02x ", (s1[i] ^ s2[i]) & 0xFF);
  }
}

inline void foundString(const uint8_t *buf, const uint8_t *parsed_begin,
                        const uint8_t *parsed_end) {
  size_t thislen = parsed_end - parsed_begin;
  total_string_length += thislen;
  good_string++;
  char *end = NULL;
  if (!parse_string((const char *)buf, bigbuffer, &end)) {
    printf("WARNING: reference parser seems to think that the string is NOT "
           "valid %32s \n",
           buf);
  }
  if (end == bigbuffer) {
    // we have a zero-length string
    if (parsed_begin != parsed_end) {
      printf("WARNING: We have a zero-length but gap is %zu \n",
             (size_t)(parsed_end - parsed_begin));
      probable_bug = true;
    }
    empty_string++;
    return;
  }
  size_t len = end - bigbuffer;
  if (len != thislen) {
    printf("WARNING: lengths on parsed strings disagree %zu %zu \n", thislen,
           len);
    printf("\nour parsed string  : '%*s'\n\n", (int)thislen,
           (const char *)parsed_begin);
    print_hex((const char *)parsed_begin, thislen);
    printf("\n");

    printf("reference parsing   :'%*s'\n\n", (int)len, bigbuffer);
    print_hex((const char *)bigbuffer, len);
    printf("\n");

    probable_bug = true;
  }
  if (memcmp(bigbuffer, parsed_begin, thislen) != 0) {
    printf("WARNING: parsed strings disagree  \n");
    printf("Lengths %zu %zu  \n", thislen, len);

    printf("\nour parsed string  : '%*s'\n", (int)thislen,
           (const char *)parsed_begin);
    print_hex((const char *)parsed_begin, thislen);
    printf("\n");

    printf("reference parsing   :'%*s'\n", (int)len, bigbuffer);
    print_hex((const char *)bigbuffer, len);
    printf("\n");

    print_cmp_hex((const char *)parsed_begin, bigbuffer, thislen);

    probable_bug = true;
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

bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre), lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool validate(const char *dirname) {
  size_t total_strings = 0;
  probable_bug = false;
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
      ParsedJson pj;
      bool allocok = pj.allocateCapacity(p.size(), 1024);
      if (!allocok) {
        std::cerr << "can't allocate memory" << std::endl;
        return false;
      }
      bigbuffer = (char *) malloc(p.size());
      if(bigbuffer == NULL) {
        std::cerr << "can't allocate memory" << std::endl;
        return false;
      }
      bad_string = 0;
      good_string = 0;
      total_string_length = 0;
      empty_string = 0;
      bool isok = json_parse(p, pj);
      free(bigbuffer);
      if (good_string > 0) {
        printf("File %40s %s --- bad strings: %10zu \tgood strings: %10zu\t "
               "empty strings: %10zu "
               "\taverage string length: %.1f \n",
               name, isok ? " is valid     " : " is not valid ", bad_string,
               good_string, empty_string,
               (double)total_string_length / good_string);
      } else if (bad_string > 0) {
        printf("File %40s %s --- bad strings: %10zu  \n", name,
               isok ? " is valid     " : " is not valid ", bad_string);
      }
      total_strings += bad_string + good_string;
      free(fullpath);
    }
  }
  printf("%zu strings checked.\n", total_strings);
  if (probable_bug) {
    fprintf(stderr, "STRING PARSING FAILS?\n");
  } else {
    printf("All ok.\n");
  }
  for (int i = 0; i < c; ++i)
    free(entry_list[i]);
  free(entry_list);
  return probable_bug == false;
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
