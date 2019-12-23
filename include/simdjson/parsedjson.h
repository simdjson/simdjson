#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include <cstring>
#include <iostream>
#include <memory>

#define JSON_VALUE_MASK 0xFFFFFFFFFFFFFF

#define DEFAULT_MAX_DEPTH                                                      \
  1024 // a JSON document with a depth exceeding 1024 is probably de facto
       // invalid

namespace simdjson {
/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
class ParsedJson {
public:
  // create a ParsedJson container with zero capacity, call allocate_capacity to
  // allocate memory
  ParsedJson()=default;
  ~ParsedJson()=default;

  // this is a move only class
  ParsedJson(ParsedJson &&p) = default;
  ParsedJson(const ParsedJson &p) = delete;
  ParsedJson &operator=(ParsedJson &&o) = default;
  ParsedJson &operator=(const ParsedJson &o) = delete;

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len bytes and max_depth "depth"
  WARN_UNUSED
  bool allocate_capacity(size_t len, size_t max_depth = DEFAULT_MAX_DEPTH);

  // returns true if the document parsed was valid
  bool is_valid() const;

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return simdjson::UNITIALIZED if no parsing was attempted
  int get_error_code() const;

  // return the string equivalent of "get_error_code"
  std::string get_error_message() const;

  // deallocate memory and set capacity to zero, called automatically by the
  // destructor
  void deallocate();

  // this should be called when parsing (right before writing the tapes)
  void init();

  // print the json to stdout (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

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
    tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline void write_tape_s64(int64_t i) {
    write_tape(0, 'l');
    // this seems weird - storing the address of a stack variable?
    tape[current_loc++] = *(reinterpret_cast<uint64_t *>(&i));
  }

  really_inline void write_tape_u64(uint64_t i) {
    write_tape(0, 'u');
    tape[current_loc++] = i;
  }

  really_inline void write_tape_double(double d) {
    write_tape(0, 'd');
    static_assert(sizeof(d) == sizeof(tape[current_loc]), "mismatch size");
    memcpy(&tape[current_loc++], &d, sizeof(double));
    // tape[current_loc++] = *((uint64_t *)&d);
  }

  really_inline uint32_t get_current_loc() const { return current_loc; }

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    tape[saved_loc] |= val;
  }

  class InvalidJSON : public std::exception {
    const char *what() const throw() { return "JSON document is invalid"; }
  };

  template <size_t max_depth> class BasicIterator;
  using Iterator = BasicIterator<DEFAULT_MAX_DEPTH>;

  size_t byte_capacity{0}; // indicates how many bits are meant to be supported

  size_t depth_capacity{0}; // how deep we can go
  size_t tape_capacity{0};
  size_t string_capacity{0};
  uint32_t current_loc{0};
  uint32_t n_structural_indexes{0};

  std::unique_ptr<uint32_t[]> structural_indexes;

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint32_t[]> containing_scope_offset;

#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity
  uint8_t *current_string_buf_loc;
  bool valid{false};
  int error_code{simdjson::UNITIALIZED};

};

// dump bits low to high
inline void dumpbits_always(uint64_t v, const std::string &msg) {
  for (uint32_t i = 0; i < 64; i++) {
    std::cout << (((v >> static_cast<uint64_t>(i)) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg.c_str() << "\n";
}

inline void dumpbits32_always(uint32_t v, const std::string &msg) {
  for (uint32_t i = 0; i < 32; i++) {
    std::cout << (((v >> i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg.c_str() << "\n";
}
} // namespace simdjson
#endif
