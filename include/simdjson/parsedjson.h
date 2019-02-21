#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <iomanip>
#include <iostream>

#include "simdjson/portability.h"
#include "simdjson/jsonformatutils.h"
#include "simdjson/common_defs.h"

#define JSONVALUEMASK 0xFFFFFFFFFFFFFF

#define DEFAULTMAXDEPTH 1024// a JSON document with a depth exceeding 1024 is probably de facto invalid


/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
struct ParsedJson {
public:

  // create a ParsedJson container with zero capacity, call allocateCapacity to
  // allocate memory
  ParsedJson();
  ~ParsedJson();
  ParsedJson(ParsedJson && p);

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len butes and maxdepth "depth"
  bool allocateCapacity(size_t len, size_t maxdepth = DEFAULTMAXDEPTH);

  bool isValid() const;

  // deallocate memory and set capacity to zero, called automatically by the
  // destructor
  void deallocate();

  // this should be called when parsing (right before writing the tapes)
  void init();

  // print the json to stdout (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  // WARN_UNUSED
  bool printjson(std::ostream &os);
  // WARN_UNUSED
  bool dump_raw_tape(std::ostream &os);


  // all nodes are stored on the tape using a 64-bit word.
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
  really_inline void write_tape(uint64_t val, uint8_t c) {
      tape[current_loc++] = val | (((uint64_t)c) << 56);
  }

  really_inline void write_tape_s64(int64_t i) {
      write_tape(0, 'l');
      tape[current_loc++] = *((uint64_t *)&i);
  }

  really_inline void write_tape_double(double d) {
    write_tape(0, 'd');
    static_assert(sizeof(d) == sizeof(tape[current_loc]), "mismatch size");
    memcpy(& tape[current_loc++], &d, sizeof(double));
    //tape[current_loc++] = *((uint64_t *)&d);
  }

  really_inline uint32_t get_current_loc() { return current_loc; }

  really_inline void annotate_previousloc(uint32_t saved_loc, uint64_t val) {
      tape[saved_loc] |= val;
  }

  size_t bytecapacity;  // indicates how many bits are meant to be supported 

  size_t depthcapacity; // how deep we can go
  size_t tapecapacity;
  size_t stringcapacity;
  uint32_t current_loc;
  uint32_t n_structural_indexes;

  uint32_t *structural_indexes;

  uint64_t *tape;
  uint32_t *containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  void **ret_address;
#else 
  char *ret_address;
#endif

  uint8_t *string_buf; // should be at least bytecapacity
  uint8_t *current_string_buf_loc;
  bool isvalid;

private :

 // we don't want the default constructor to be called
 ParsedJson(const ParsedJson & p); // we don't want the default constructor to be called
 // we don't want the assignment to be called
 ParsedJson & operator=(const ParsedJson&o);
};


// dump bits low to high
inline void dumpbits_always(uint64_t v, const std::string &msg) {
  for (uint32_t i = 0; i < 64; i++) {
    std::cout << (((v >> (uint64_t)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg.c_str() << "\n";
}

inline void dumpbits32_always(uint32_t v, const std::string &msg) {
  for (uint32_t i = 0; i < 32; i++) {
    std::cout << (((v >> (uint32_t)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg.c_str() << "\n";
}


#endif
