#ifndef SIMDJSON_JSONFORMATUTILS_H
#define SIMDJSON_JSONFORMATUTILS_H

#include <cstdio>
#include <iomanip>
#include <iostream>

static inline void print_with_escapes(const unsigned char *src) {
  while (*src != 0u) {
    switch (*src) {
    case '\b':
      putchar('\\');
      putchar('b');
      break;
    case '\f':
      putchar('\\');
      putchar('f');
      break;
    case '\n':
      putchar('\\');
      putchar('n');
      break;
    case '\r':
      putchar('\\');
      putchar('r');
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
        printf("\\u%04x", *src);
      } else {
        putchar(*src);
}
    }
    src++;
  }
}

static inline void print_with_escapes(const unsigned char *src, std::ostream &os) {
  while (*src != 0u) {
    switch (*src) {
    case '\b':
      os << '\\';
      os << 'b';
      break;
    case '\f':
      os << '\\';
      os << 'f';
      break;
    case '\n':
      os << '\\';
      os << 'n';
      break;
    case '\r':
      os << '\\';
      os << 'r';
      break;
    case '\"':
      os << '\\';
      os << '"';
      break;
    case '\t':
      os << '\\';
      os << 't';
      break;
    case '\\':
      os << '\\';
      os << '\\';
      break;
    default:
      if (*src <= 0x1F) {
        std::ios::fmtflags f(os.flags());
        os << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*src);
        os.flags(f);
      } else {
        os << *src;
}
    }
    src++;
  }
}

static inline void print_with_escapes(const char *src, std::ostream &os) {
  print_with_escapes(reinterpret_cast<const unsigned char *>(src), os);
}

#endif
