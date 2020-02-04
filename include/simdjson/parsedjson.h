#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include <cstring>
#include <memory>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"

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

  really_inline ErrorValues on_error(ErrorValues new_error_code) {
    error_code = new_error_code;
    return new_error_code;
  }
  really_inline ErrorValues on_success(ErrorValues success_code) {
    error_code = success_code;
    valid = true;
    return success_code;
  }
  really_inline bool on_start_document(uint32_t depth) {
    containing_scope_offset[depth] = get_current_loc();
    write_tape(0, 'r');
    return true;
  }
  really_inline bool on_start_object(uint32_t depth) {
    containing_scope_offset[depth] = get_current_loc();
    write_tape(0, '{');
    return true;
  }
  really_inline bool on_start_array(uint32_t depth) {
    containing_scope_offset[depth] = get_current_loc();
    write_tape(0, '[');
    return true;
  }
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    annotate_previous_loc(containing_scope_offset[depth], get_current_loc());
    write_tape(containing_scope_offset[depth], 'r');
    return true;
  }
  really_inline bool on_end_object(uint32_t depth) {
    // write our tape location to the header scope
    write_tape(containing_scope_offset[depth], '}');
    annotate_previous_loc(containing_scope_offset[depth], get_current_loc());
    return true;
  }
  really_inline bool on_end_array(uint32_t depth) {
    // write our tape location to the header scope
    write_tape(containing_scope_offset[depth], ']');
    annotate_previous_loc(containing_scope_offset[depth], get_current_loc());
    return true;
  }

  really_inline bool on_true_atom() {
    write_tape(0, 't');
    return true;
  }
  really_inline bool on_false_atom() {
    write_tape(0, 'f');
    return true;
  }
  really_inline bool on_null_atom() {
    write_tape(0, 'n');
    return true;
  }

  really_inline uint8_t *on_start_string() {
    /* we advance the point, accounting for the fact that we have a NULL
      * termination         */
    write_tape(current_string_buf_loc - string_buf.get(), '"');
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline bool on_end_string(uint8_t *dst) {
    uint32_t str_length = dst - (current_string_buf_loc + sizeof(uint32_t));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
    return true;
  }

  really_inline bool on_number_s64(int64_t value) {
    write_tape(0, 'l');
    std::memcpy(&tape[current_loc], &value, sizeof(value));
    ++current_loc;
    return true;
  }
  really_inline bool on_number_u64(uint64_t value) {
    write_tape(0, 'u');
    tape[current_loc++] = value;
    return true;
  }
  really_inline bool on_number_double(double value) {
    write_tape(0, 'd');
    static_assert(sizeof(value) == sizeof(tape[current_loc]), "mismatch size");
    memcpy(&tape[current_loc++], &value, sizeof(double));
    // tape[current_loc++] = *((uint64_t *)&d);
    return true;
  }

  really_inline uint32_t get_current_loc() const { return current_loc; }

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

  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity
  uint8_t *current_string_buf_loc;
  bool valid{false};
  int error_code{simdjson::UNINITIALIZED};

private:
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

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    tape[saved_loc] |= val;
  }
};


} // namespace simdjson
#endif
