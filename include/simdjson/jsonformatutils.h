#pragma once

#include <stdio.h>

static inline void print_with_escapes(const unsigned char *src) {
  while (*src) {
    switch (*src) {
    case '\n':
      putchar('\\');
      putchar('n');
      break;
    case '\"':
      putchar('\\');
      putchar('"');
      break;
    case '\t':
      putchar('\\');
      putchar('t');
      break;
    case '\\':
      putchar('\\');
      putchar('\\');
      break;
    default:
      if (*src <= 0x1F) {
        printf("\\u%x", *src);
      } else
        putchar(*src);
    }
    src++;
  }
}

