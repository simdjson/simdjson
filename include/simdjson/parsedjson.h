#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include <cstring>
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

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  /***
   * Following functions are a stage 2 consumer.
   */

  // return true if you want to continue, false to stop
  really_inline bool found_double(double d) {
    write_tape_double(d);
    return true;
  }
  really_inline bool found_int64(int64_t d) {
    write_tape_s64(d);
    return true;
  }
  really_inline bool found_uint64(uint64_t d) {
    write_tape_u64(d);
    return true;
  }

  really_inline bool found_start_object() {
    return true;
  }

  really_inline bool found_end_object() {
    return true;
  }

  really_inline bool found_start_array() {
    return true;
  }

  really_inline bool found_end_array() {
    return true;
  }

  really_inline bool found_true() {
    write_tape(0, 't');
    return true;
  }

  really_inline bool found_false() {
    write_tape(0, 'f');
    return true;
  }

  really_inline bool found_null(){
    write_tape(0, 'n');
    return true;
  }

  really_inline void found_end_of_document() {

  }

  really_inline void found_error(int err_code) {
      valid = false;
      err_code = simdjson::UNINITIALIZED;
  }

  // gives me a pointer to an available buffer where I could write up to "budget" bytes, return null if unavailable
  really_inline char * string_buffer(size_t /*budget*/) {
    return (char *)current_string_buf_loc + sizeof(uint32_t);
  }
  // I wrote a string of length "len" on the string buffer
  really_inline bool found_string(size_t len) {
    write_tape(current_string_buf_loc - string_buf.get(), '"');
    memcpy(current_string_buf_loc, &len, sizeof(uint32_t));
    current_string_buf_loc +=  sizeof(uint32_t) + len + 1;
    return true;
  }

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



  struct InvalidJSON : public std::exception {
    const char *what() const noexcept { return "JSON document is invalid"; }
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


  bool valid{false};
  int error_code{simdjson::UNINITIALIZED};
private:
  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity
  uint8_t *current_string_buf_loc;
  // this should be considered a private function
  really_inline void write_tape(uint64_t val, uint8_t c) {
    tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline void write_tape_s64(int64_t i) {
    write_tape(0, 'l');
    std::memcpy(&tape[current_loc], &i, sizeof(i));
    ++current_loc;
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

};


} // namespace simdjson
#endif
