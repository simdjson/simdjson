#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "simdjson/common_defs.h"
#include "simdjson/jsonformatutils.h"
#include "simdjson/portability.h"

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
  // documents having up to len bytes and maxdepth "depth"
  WARN_UNUSED
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
  WARN_UNUSED
  bool printjson(std::ostream &os);
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
    memcpy(& tape[current_loc++], &d, sizeof(double));
    //tape[current_loc++] = *((uint64_t *)&d);
  }

  really_inline uint32_t get_current_loc() { return current_loc; }

  really_inline void annotate_previousloc(uint32_t saved_loc, uint64_t val) {
      tape[saved_loc] |= val;
  }

  struct InvalidJSON : public std::exception {
	const char * what () const throw () {
 	     return "JSON document is invalid";
    }
  };

  struct iterator {
    // might throw InvalidJSON if ParsedJson is invalid
    explicit iterator(ParsedJson &pj_);
    ~iterator();

    iterator(const iterator &o);

    iterator(iterator &&o);

    bool isOk() const;

    // useful for debuging purposes
    size_t get_tape_location() const;

    // useful for debuging purposes
    size_t get_tape_length() const;

    // returns the current depth (start at 1 with 0 reserved for the fictitious root node)
    size_t get_depth() const;

    // A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
    // The root node has type 'r'.
    uint8_t get_scope_type() const;

    // move forward in document order
    bool move_forward();

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    uint8_t get_type()  const;

    // get the int64_t value at this node; valid only if we're at "l"
    int64_t get_integer()  const;

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see print_with_escapes)
    // return value is valid UTF-8
    // It may contain NULL chars within the string: get_string_length determines the true 
    // string length.
    const char * get_string() const;

    uint32_t get_string_length() const;

    // get the double value at this node; valid only if
    // we're at "d"
    double get_double()  const;

    bool is_object_or_array() const;

    bool is_object() const;

    bool is_array() const;

    bool is_string() const;

    bool is_integer() const;

    bool is_double() const;

    bool is_true() const;

    bool is_false() const;

    bool is_null() const;

    static bool is_object_or_array(uint8_t type);

    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one)
    // We seek the key using C's strcmp so if your JSON strings contain
    // NULL chars, this would trigger a false positive: if you expect that
    // to be the case, take extra precautions.
    bool move_to_key(const char * key);

    // throughout return true if we can do the navigation, false
    // otherwise

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move forward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, { and [.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    // valid if we're not at the end of a scope (returns true).
    bool next();

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move backward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true when starting at the end
    // of the scope.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    bool prev();

    // Moves back to either the containing array or object (type { or [) from
    // within a contained scope.
    // Valid unless we are at the first level of the document
    bool up();


    // Valid if we're at a [ or { and it starts a non-empty scope; moves us to start of
    // that deeper scope if it not empty.
    // Thus, given [true, null, {"a":1}, [1,2]], if we are at the { node, we would move to the
    // "a" node.
    bool down();

    // move us to the start of our current scope,
    // a scope is a series of nodes at the same level
    void to_start_scope();

    // void to_end_scope();              // move us to
    // the start of our current scope; always succeeds

    // print the thing we're currently pointing at
    bool print(std::ostream &os, bool escape_strings = true) const;
    typedef struct {size_t start_of_scope; uint8_t scope_type;} scopeindex_t;

private:

    iterator& operator=(const iterator& other) = delete ;

    ParsedJson &pj;
    size_t depth;
    size_t location;     // our current location on a tape
    size_t tape_length;
    uint8_t current_type;
    uint64_t current_val;
    scopeindex_t *depthindex;
  };

  size_t bytecapacity{0};  // indicates how many bits are meant to be supported 

  size_t depthcapacity{0}; // how deep we can go
  size_t tapecapacity{0};
  size_t stringcapacity{0};
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

  uint8_t *string_buf; // should be at least bytecapacity
  uint8_t *current_string_buf_loc;
  bool isvalid{false};

private :

 // we don't want the default constructor to be called
 ParsedJson(const ParsedJson & p) = delete; // we don't want the default constructor to be called
 // we don't want the assignment to be called
 ParsedJson & operator=(const ParsedJson&o) = delete;
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


#endif
