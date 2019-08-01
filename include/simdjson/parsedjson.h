#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include "simdjson/common_defs.h"
#include "simdjson/jsonformatutils.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>

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
  ParsedJson();
  ~ParsedJson();
  ParsedJson(ParsedJson &&p);

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
  bool print_json(std::ostream &os);
  WARN_UNUSED
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
    tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline void write_tape_s64(int64_t i) {
    write_tape(0, 'l');
    tape[current_loc++] = *(reinterpret_cast<uint64_t *>(&i));
  }

  really_inline void write_tape_double(double d) {
    write_tape(0, 'd');
    static_assert(sizeof(d) == sizeof(tape[current_loc]), "mismatch size");
    memcpy(&tape[current_loc++], &d, sizeof(double));
    // tape[current_loc++] = *((uint64_t *)&d);
  }

  really_inline uint32_t get_current_loc() { return current_loc; }

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    tape[saved_loc] |= val;
  }

  class InvalidJSON : public std::exception {
    const char *what() const throw() { return "JSON document is invalid"; }
  };

  class Iterator {
    // might throw InvalidJSON if ParsedJson is invalid
  public:
    explicit Iterator(ParsedJson &pj_);
    ~Iterator();

    Iterator(const Iterator &o) noexcept;

    Iterator(Iterator &&o) noexcept;

    inline bool is_ok() const;

    // useful for debuging purposes
    inline size_t get_tape_location() const;

    // useful for debuging purposes
    inline size_t get_tape_length() const;

    // returns the current depth (start at 1 with 0 reserved for the fictitious
    // root node)
    inline size_t get_depth() const;

    // A scope is a series of nodes at the same depth, typically it is either an
    // object ({) or an array ([). The root node has type 'r'.
    inline uint8_t get_scope_type() const;

    // move forward in document order
    inline bool move_forward();

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    inline uint8_t get_type() const {
      return current_type; // short functions should be inlined!
    }

    // get the int64_t value at this node; valid only if we're at "l"
    inline int64_t get_integer() const {
      if (location + 1 >= tape_length) {
        return 0; // default value in case of error
      }
      return static_cast<int64_t>(pj.tape[location + 1]);
    }

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see
    // print_with_escapes) return value is valid UTF-8 It may contain NULL chars
    // within the string: get_string_length determines the true string length.
    inline const char *get_string() const {
      return reinterpret_cast<const char *>(
          pj.string_buf + (current_val & JSON_VALUE_MASK) + sizeof(uint32_t));
    }

    // return the length of the string in bytes
    inline uint32_t get_string_length() const {
      uint32_t answer;
      memcpy(&answer,
             reinterpret_cast<const char *>(pj.string_buf +
                                            (current_val & JSON_VALUE_MASK)),
             sizeof(uint32_t));
      return answer;
    }

    // get the double value at this node; valid only if
    // we're at "d"
    inline double get_double() const {
      if (location + 1 >= tape_length) {
        return std::numeric_limits<double>::quiet_NaN(); // default value in
                                                         // case of error
      }
      double answer;
      memcpy(&answer, &pj.tape[location + 1], sizeof(answer));
      return answer;
    }

    inline bool is_object_or_array() const { return is_object() || is_array(); }

    inline bool is_object() const { return get_type() == '{'; }

    inline bool is_array() const { return get_type() == '['; }

    inline bool is_string() const { return get_type() == '"'; }

    inline bool is_integer() const { return get_type() == 'l'; }

    inline bool is_double() const { return get_type() == 'd'; }

    inline bool is_true() const { return get_type() == 't'; }

    inline bool is_false() const { return get_type() == 'f'; }

    inline bool is_null() const { return get_type() == 'n'; }

    static bool is_object_or_array(uint8_t type) {
      return ((type == '[') || (type == '{'));
    }

    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one).
    // We seek the key using C's strcmp so if your JSON strings contain
    // NULL chars, this would trigger a false positive: if you expect that
    // to be the case, take extra precautions.
    inline bool move_to_key(const char *key);
    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one).
    // The string we search for can contain NULL values.
    inline bool move_to_key(const char *key, uint32_t length);

    // when at a key location within an object, this moves to the accompanying
    // value (located next to it). this is equivalent but much faster than
    // calling "next()".
    inline void move_to_value();

    // when at [, go one level deep, and advance to the given index.
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the array ([)
    inline bool move_to_index(uint32_t index);

    // Moves the iterator to the value correspoding to the json pointer.
    // Always search from the root of the document.
    // if successful, we are left pointing at the value,
    // if not, we are still pointing the same value we were pointing before the
    // call. The json pointer follows the rfc6901 standard's syntax:
    // https://tools.ietf.org/html/rfc6901 However, the standard says "If a
    // referenced member name is not unique in an object, the member that is
    // referenced is undefined, and evaluation fails". Here we just return the
    // first corresponding value. The length parameter is the length of the
    // jsonpointer string ('pointer').
    bool move_to(const char *pointer, uint32_t length);

    // Moves the iterator to the value correspoding to the json pointer.
    // Always search from the root of the document.
    // if successful, we are left pointing at the value,
    // if not, we are still pointing the same value we were pointing before the
    // call. The json pointer implementation follows the rfc6901 standard's
    // syntax: https://tools.ietf.org/html/rfc6901 However, the standard says
    // "If a referenced member name is not unique in an object, the member that
    // is referenced is undefined, and evaluation fails". Here we just return
    // the first corresponding value.
    inline bool move_to(const std::string &pointer) {
      return move_to(pointer.c_str(), pointer.length());
    }

  private:
    // Almost the same as move_to(), except it searchs from the current
    // position. The pointer's syntax is identical, though that case is not
    // handled by the rfc6901 standard. The '/' is still required at the
    // beginning. However, contrary to move_to(), the URI Fragment Identifier
    // Representation is not supported here. Also, in case of failure, we are
    // left pointing at the closest value it could reach. For these reasons it
    // is private. It exists because it is used by move_to().
    bool relative_move_to(const char *pointer, uint32_t length);

  public:
    // throughout return true if we can do the navigation, false
    // otherwise

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move forward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, {
    // and [. At the object ({) or at the array ([), you can issue a "down" to
    // visit their content. valid if we're not at the end of a scope (returns
    // true).
    inline bool next();

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move backward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true
    // when starting at the end of the scope. At the object ({) or at the array
    // ([), you can issue a "down" to visit their content.
    inline bool prev();

    // Moves back to either the containing array or object (type { or [) from
    // within a contained scope.
    // Valid unless we are at the first level of the document
    inline bool up();

    // Valid if we're at a [ or { and it starts a non-empty scope; moves us to
    // start of that deeper scope if it not empty. Thus, given [true, null,
    // {"a":1}, [1,2]], if we are at the { node, we would move to the "a" node.
    inline bool down();

    // move us to the start of our current scope,
    // a scope is a series of nodes at the same level
    inline void to_start_scope();

    inline void rewind() {
      while (up())
        ;
    }

    // void to_end_scope();              // move us to
    // the start of our current scope; always succeeds

    // print the thing we're currently pointing at
    bool print(std::ostream &os, bool escape_strings = true) const;
    typedef struct {
      size_t start_of_scope;
      uint8_t scope_type;
    } scopeindex_t;

  private:
    Iterator &operator=(const Iterator &other) = delete;

    ParsedJson &pj;
    size_t depth;
    size_t location; // our current location on a tape
    size_t tape_length;
    uint8_t current_type;
    uint64_t current_val;
    scopeindex_t *depth_index;
  };

  size_t byte_capacity{0}; // indicates how many bits are meant to be supported

  size_t depth_capacity{0}; // how deep we can go
  size_t tape_capacity{0};
  size_t string_capacity{0};
  uint32_t current_loc{0};
  uint32_t n_structural_indexes{0};

  uint32_t *structural_indexes;

  uint64_t *tape;
  uint32_t *containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  void **ret_address;
#else
  char *ret_address;
#endif

  uint8_t *string_buf; // should be at least byte_capacity
  uint8_t *current_string_buf_loc;
  bool valid{false};
  int error_code{simdjson::UNITIALIZED};

private:
  // we don't want the default constructor to be called
  ParsedJson(const ParsedJson &p) =
      delete; // we don't want the default constructor to be called
  // we don't want the assignment to be called
  ParsedJson &operator=(const ParsedJson &o) = delete;
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

WARN_UNUSED
bool ParsedJson::Iterator::is_ok() const { return location < tape_length; }

// useful for debuging purposes
size_t ParsedJson::Iterator::get_tape_location() const { return location; }

// useful for debuging purposes
size_t ParsedJson::Iterator::get_tape_length() const { return tape_length; }

// returns the current depth (start at 1 with 0 reserved for the fictitious root
// node)
size_t ParsedJson::Iterator::get_depth() const { return depth; }

// A scope is a series of nodes at the same depth, typically it is either an
// object ({) or an array ([). The root node has type 'r'.
uint8_t ParsedJson::Iterator::get_scope_type() const {
  return depth_index[depth].scope_type;
}

bool ParsedJson::Iterator::move_forward() {
  if (location + 1 >= tape_length) {
    return false; // we are at the end!
  }

  if ((current_type == '[') || (current_type == '{')) {
    // We are entering a new scope
    depth++;
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
  } else if ((current_type == ']') || (current_type == '}')) {
    // Leaving a scope.
    depth--;
  } else if ((current_type == 'd') || (current_type == 'l')) {
    // d and l types use 2 locations on the tape, not just one.
    location += 1;
  }

  location += 1;
  current_val = pj.tape[location];
  current_type = (current_val >> 56);
  return true;
}

void ParsedJson::Iterator::move_to_value() {
  // assume that we are on a key, so move by 1.
  location += 1;
  current_val = pj.tape[location];
  current_type = (current_val >> 56);
}

bool ParsedJson::Iterator::move_to_key(const char *key) {
  if (down()) {
    do {
      assert(is_string());
      bool right_key =
          (strcmp(get_string(), key) == 0); // null chars would fool this
      move_to_value();
      if (right_key) {
        return true;
      }
    } while (next());
    assert(up()); // not found
  }
  return false;
}

bool ParsedJson::Iterator::move_to_key(const char *key, uint32_t length) {
  if (down()) {
    do {
      assert(is_string());
      bool right_key = ((get_string_length() == length) &&
                        (memcmp(get_string(), key, length) == 0));
      move_to_value();
      if (right_key) {
        return true;
      }
    } while (next());
    assert(up()); // not found
  }
  return false;
}

bool ParsedJson::Iterator::move_to_index(uint32_t index) {
  assert(is_array());
  if (down()) {
    uint32_t i = 0;
    for (; i < index; i++) {
      if (!next()) {
        break;
      }
    }
    if (i == index) {
      return true;
    }
    assert(up());
  }
  return false;
}

bool ParsedJson::Iterator::prev() {
  if (location - 1 < depth_index[depth].start_of_scope) {
    return false;
  }
  location -= 1;
  current_val = pj.tape[location];
  current_type = (current_val >> 56);
  if ((current_type == ']') || (current_type == '}')) {
    // we need to jump
    size_t new_location = (current_val & JSON_VALUE_MASK);
    if (new_location < depth_index[depth].start_of_scope) {
      return false; // shoud never happen
    }
    location = new_location;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
  }
  return true;
}

bool ParsedJson::Iterator::up() {
  if (depth == 1) {
    return false; // don't allow moving back to root
  }
  to_start_scope();
  // next we just move to the previous value
  depth--;
  location -= 1;
  current_val = pj.tape[location];
  current_type = (current_val >> 56);
  return true;
}

bool ParsedJson::Iterator::down() {
  if (location + 1 >= tape_length) {
    return false;
  }
  if ((current_type == '[') || (current_type == '{')) {
    size_t npos = (current_val & JSON_VALUE_MASK);
    if (npos == location + 2) {
      return false; // we have an empty scope
    }
    depth++;
    location = location + 1;
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    return true;
  }
  return false;
}

void ParsedJson::Iterator::to_start_scope() {
  location = depth_index[depth].start_of_scope;
  current_val = pj.tape[location];
  current_type = (current_val >> 56);
}

bool ParsedJson::Iterator::next() {
  size_t npos;
  if ((current_type == '[') || (current_type == '{')) {
    // we need to jump
    npos = (current_val & JSON_VALUE_MASK);
  } else {
    npos = location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
  }
  uint64_t next_val = pj.tape[npos];
  uint8_t next_type = (next_val >> 56);
  if ((next_type == ']') || (next_type == '}')) {
    return false; // we reached the end of the scope
  }
  location = npos;
  current_val = next_val;
  current_type = next_type;
  return true;
}
} // namespace simdjson
#endif
