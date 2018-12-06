#pragma once

#include <inttypes.h>
#include <string.h>
#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <iomanip>
#include <iostream>

#include "simdjson/jsonformatutils.h"

#define JSONVALUEMASK 0xFFFFFFFFFFFFFF;

struct ParsedJson {
public:
  size_t bytecapacity;  // indicates how many bits are meant to be supported by
                        // structurals
  size_t depthcapacity; // how deep we can go
  size_t tapecapacity;
  size_t stringcapacity;
  u32 current_loc;
  u8 *structurals;
  u32 n_structural_indexes;
  u32 *structural_indexes;

  u64 *tape;
  u32 *containing_scope_offset;
  void **ret_address;

  u8 *string_buf; // should be at least bytecapacity
  u8 *current_string_buf_loc;

  // create a ParsedJson container with zero capacity, call allocateCapacity to
  // allocate memory
  ParsedJson()
      : bytecapacity(0), depthcapacity(0), tapecapacity(0), stringcapacity(0),
        current_loc(0), structurals(NULL), n_structural_indexes(0),
        structural_indexes(NULL), tape(NULL), containing_scope_offset(NULL),
        ret_address(NULL), string_buf(NULL), current_string_buf_loc(NULL) {}

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len butes and maxdepth "depth"
  WARN_UNUSED
  inline bool allocateCapacity(size_t len, size_t maxdepth) {
    if ((maxdepth == 0) || (len == 0)) {
      std::cerr << "capacities must be non-zero " << std::endl;
      return false;
    }
    if (len > 0) {
      if ((len <= bytecapacity) && (depthcapacity < maxdepth))
        return true;
      deallocate();
    }
    bytecapacity = 0; // will only set it to len after allocations are a success
    if (posix_memalign((void **)&structurals, 8, ROUNDUP_N(len, 64) / 8)) {
      std::cerr << "Could not allocate memory for structurals" << std::endl;
      return false;
    };
    n_structural_indexes = 0;
    u32 max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new u32[max_structures];

    if (structural_indexes == NULL) {
      std::cerr << "Could not allocate memory for structural_indexes"
                << std::endl;
      delete[] structurals;
      return false;
    }
    size_t localtapecapacity = ROUNDUP_N(len, 64);
    size_t localstringcapacity = ROUNDUP_N(len, 64);
    string_buf = new u8[localstringcapacity];
    tape = new u64[localtapecapacity];
    containing_scope_offset = new u32[maxdepth];
    ret_address = new void *[maxdepth];

    if ((string_buf == NULL) || (tape == NULL) ||
        (containing_scope_offset == NULL) || (ret_address == NULL)) {
      std::cerr << "Could not allocate memory" << std::endl;
      delete[] ret_address;
      delete[] containing_scope_offset;
      delete[] tape;
      delete[] string_buf;
      delete[] structural_indexes;
      delete[] structurals;
      return false;
    }

    bytecapacity = len;
    depthcapacity = maxdepth;
    tapecapacity = localtapecapacity;
    stringcapacity = localstringcapacity;
    return true;
  }

  // deallocate memory and set capacity to zero, called automatically by the
  // destructor
  void deallocate() {
    bytecapacity = 0;
    depthcapacity = 0;
    tapecapacity = 0;
    stringcapacity = 0;
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;
    free(structurals);
  }

  ~ParsedJson() { deallocate(); }

  // this should be called when parsing (right before writing the tapes)
  void init() {
    current_string_buf_loc = string_buf;
    current_loc = 0;
  }

  // print the json to stdout (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  bool printjson() {
    size_t tapeidx = 0;
    u64 tape_val = tape[tapeidx];
    u8 type = (tape_val >> 56);
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      printf("Error: no starting root node?");
      return false;
    }
    if (howmany > tapecapacity) {
      printf(
          "We may be exceeding the tape capacity. Is this a valid document?\n");
      return false;
    }
    tapeidx++;
    bool *inobject = new bool[depthcapacity];
    size_t *inobjectidx = new size_t[depthcapacity];
    int depth = 1; // only root at level 0
    inobjectidx[depth] = 0;
    for (; tapeidx < howmany; tapeidx++) {
      tape_val = tape[tapeidx];
      u64 payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      if (!inobject[depth]) {
        if ((inobjectidx[depth] > 0) && (type != ']'))
          printf(",  ");
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}'))
          printf(",  ");
        if (((inobjectidx[depth] & 1) == 1))
          printf(" : ");
        inobjectidx[depth]++;
      }
      switch (type) {
      case '"': // we have a string
        putchar('"');
        print_with_escapes((const unsigned char *)(string_buf + payload));
        putchar('"');
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany)
          return false;
        printf("%" PRId64, (int64_t)tape[++tapeidx]);
        break;
      case 'd': // we have a double
        if (tapeidx + 1 >= howmany)
          return false;
        printf("%f", *((double *)&tape[++tapeidx]));
        break;
      case 'n': // we have a null
        printf("null");
        break;
      case 't': // we have a true
        printf("true");
        break;
      case 'f': // we have a false
        printf("false");
        break;
      case '{': // we have an object
        printf("\n");
        printf("%*s\n%*s", depth, "{", depth + 1, "");
        depth++;
        inobject[depth] = true;
        inobjectidx[depth] = 0;
        break;
      case '}': // we end an object
        depth--;
        printf("\n%*s}\n%*s", depth - 1, "", depth, "");
        break;
      case '[': // we start an array
        printf("\n");
        printf("%*s\n%*s", depth, "[", depth + 1, "");
        depth++;
        inobject[depth] = false;
        inobjectidx[depth] = 0;
        break;
      case ']': // we end an array
        depth--;
        printf("\n%*s]\n%*s", depth - 1, "", depth, "");
        break;
      case 'r': // we start and end with the root node
        printf("should we be hitting the root node?\n");
        free(inobject);
        free(inobjectidx);
        return false;
      default:
        printf("bug %c\n", type);
        free(inobject);
        free(inobjectidx);
        return false;
      }
    }
    free(inobject);
    free(inobjectidx);
    return true;
  }

  // all elements are stored on the tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  // this should be considered a private function
  really_inline void write_tape(u64 val, u8 c) {
    tape[current_loc++] = val | (((u64)c) << 56);
  }

  really_inline void write_tape_s64(s64 i) {
    write_tape(0, 'l');
    tape[current_loc++] = *((u64 *)&i);
  }

  really_inline void write_tape_double(double d) {
    write_tape(0, 'd');
    static_assert(sizeof(d) == sizeof(tape[current_loc]), "mismatch size");
    memcpy(& tape[current_loc++], &d, sizeof(double));
    //tape[current_loc++] = *((u64 *)&d);
  }

  really_inline u32 get_current_loc() { return current_loc; }

  really_inline void annotate_previousloc(u32 saved_loc, u64 val) {
    tape[saved_loc] |= val;
  }

  // public interface
#if 1

  struct ParsedJsonHandle {
    ParsedJson &pj;
    u32 depth;
    u32 scope_header; // the start of our current scope that contains our
                      // current location
    u32 location;     // our current location on a tape

    explicit ParsedJsonHandle(ParsedJson &pj_)
        : pj(pj_), depth(0), scope_header(0), location(0) {}
    // OK with default copy constructor as the way to clone the POD structure

    // some placeholder navigation. Will convert over to a more native C++-ish
    // way of doing things once it's working (i.e. ++ and -- operators and get
    // start/end iterators) return true if we can do the navigation, false
    // otherwise
    bool next(); // valid if we're not at the end of a scope
    bool prev(); // valid if we're not at the start of a scope
    bool up();   // valid if we are at depth != 0
    bool down(); // valid if we're at a [ or { call site; moves us to header of
                 // that scope
    // void to_start_scope();            // move us to the start of our current
    // scope; always succeeds void to_end_scope();              // move us to
    // the start of our current scope; always succeeds

    // these navigation elements move us across scope if need be, so allow us to
    // iterate over everything at a given depth
    // bool next_flat();                 // valid if we're not at the end of a
    // tape bool prev_flat();                 // valid if we're not at the start
    // of a tape

    void print(std::ostream &os); // print the thing we're currently pointing at
    u8 get_type(); // retrieve the character code of what we're looking at:
                   // [{"sltfn are the possibilities
    s64 get_s64(); // get the s64 value at this node; valid only if we're at "s"
    double get_double(); // get the double value at this node; valid only if
                         // we're at "d"
    char *
    get_string(); // get the string value at this node; valid only if we're at "
  };

#endif
};

#ifdef DEBUG
inline void dump256(m256 d, const std::string &msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << std::setw(3) << (int)*(((u8 *)(&d)) + i);
    if (!((i + 1) % 8))
      std::cout << "|";
    else if (!((i + 1) % 4))
      std::cout << ":";
    else
      std::cout << " ";
  }
  std::cout << " " << msg << "\n";
}

// dump bits low to high
inline void dumpbits(u64 v, const std::string &msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

inline void dumpbits32(u32 v, const std::string &msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
#else
#define dump256(a, b) ;
#define dumpbits(a, b) ;
#define dumpbits32(a, b) ;
#endif

// dump bits low to high
inline void dumpbits_always(u64 v, const std::string &msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

inline void dumpbits32_always(u32 v, const std::string &msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
