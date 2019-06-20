#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include "simdjson/simdjson.h"
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

  // returns true if the document parsed was valid
  bool isValid() const;

  // return an error code corresponding to the last parsing attempt, see simdjson.h
  // will return simdjson::UNITIALIZED if no parsing was attempted
  int getErrorCode() const;

  // return the string equivalent of "getErrorCode"
  std::string getErrorMsg() const;

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

    inline bool isOk() const;

    // useful for debuging purposes
    inline size_t get_tape_location() const;

    // useful for debuging purposes
    inline size_t get_tape_length() const;

    // returns the current depth (start at 1 with 0 reserved for the fictitious root node)
    inline size_t get_depth() const;

    // A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
    // The root node has type 'r'.
    inline uint8_t get_scope_type() const;

    // move forward in document order
    inline bool move_forward();

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    inline uint8_t get_type()  const {
       return current_type; // short functions should be inlined!
    }

    // get the int64_t value at this node; valid only if we're at "l"
    inline int64_t get_integer()  const {
       if(location + 1 >= tape_length) {
         return 0;// default value in case of error
       }
       return static_cast<int64_t>(pj.tape[location + 1]);
    }

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see print_with_escapes)
    // return value is valid UTF-8
    // It may contain NULL chars within the string: get_string_length determines the true 
    // string length.
    inline const char * get_string() const {
      return  reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK) + sizeof(uint32_t)) ;
    }

    // return the length of the string in bytes
    inline uint32_t get_string_length() const {
      uint32_t answer;
      memcpy(&answer, reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK)), sizeof(uint32_t));
      return answer;
    }

    // get the double value at this node; valid only if
    // we're at "d"
    inline double get_double()  const {
      if(location + 1 >= tape_length) {
        return NAN;// default value in case of error
      }
      double answer;
      memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
      return answer;
    }


    inline bool is_object_or_array() const {
      return is_object() || is_array();
    }

    inline bool is_object() const {
      return get_type() == '{';
    }

    inline bool is_array() const {
      return get_type() == '[';
    }

    inline bool is_string() const {
      return get_type() == '"';
    }

    inline bool is_integer() const {
      return get_type() == 'l';
    }

    inline bool is_double() const {
      return get_type() == 'd';
    }

    inline bool is_true() const {
      return get_type() == 't';
    }

    inline bool is_false() const {
      return get_type() == 'f';
    }

    inline bool is_null() const {
      return get_type() == 'n';
    }

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
    inline bool move_to_key(const char * key);
    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one).
    // The string we search for can contain NULL values.
    inline bool move_to_key(const char * key, uint32_t length);
    
    // when at a key location within an object, this moves to the accompanying value (located next to it).
    // this is equivalent but much faster than calling "next()".
    inline void move_to_value();

    // throughout return true if we can do the navigation, false
    // otherwise

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move forward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, { and [.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    // valid if we're not at the end of a scope (returns true).
    inline bool next();

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move backward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true when starting at the end
    // of the scope.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    inline bool prev();

    // Moves back to either the containing array or object (type { or [) from
    // within a contained scope.
    // Valid unless we are at the first level of the document
    inline bool up();


    // Valid if we're at a [ or { and it starts a non-empty scope; moves us to start of
    // that deeper scope if it not empty.
    // Thus, given [true, null, {"a":1}, [1,2]], if we are at the { node, we would move to the
    // "a" node.
    inline bool down();

    // move us to the start of our current scope,
    // a scope is a series of nodes at the same level
    inline void to_start_scope();

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
  int errorcode{simdjson::UNITIALIZED};

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

WARN_UNUSED
bool ParsedJson::iterator::isOk() const {
      return location < tape_length;
}

// useful for debuging purposes
size_t ParsedJson::iterator::get_tape_location() const {
    return location;
}

// useful for debuging purposes
size_t ParsedJson::iterator::get_tape_length() const {
    return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root node)
size_t ParsedJson::iterator::get_depth() const {
    return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
// The root node has type 'r'.
uint8_t ParsedJson::iterator::get_scope_type() const {
    return depthindex[depth].scope_type;
}

bool ParsedJson::iterator::move_forward() {
    if(location + 1 >= tape_length) {
        return false; // we are at the end!
    }

    if ((current_type == '[') || (current_type == '{')){
        // We are entering a new scope
        depth++;
        depthindex[depth].start_of_scope = location;
        depthindex[depth].scope_type = current_type;
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

void ParsedJson::iterator::move_to_value() {
    // assume that we are on a key, so move by 1.
    location += 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
}


bool ParsedJson::iterator::move_to_key(const char * key) {
    if(down()) {
      do {
        assert(is_string());
        bool rightkey = (strcmp(get_string(),key)==0);// null chars would fool this
        move_to_value();
        if(rightkey) { 
          return true;
        }
      } while(next());
      assert(up());// not found
    }
    return false;
}

bool ParsedJson::iterator::move_to_key(const char * key, uint32_t length) {
    if(down()) {
      do {
        assert(is_string());
        bool rightkey = ((get_string_length() == length) && (memcmp(get_string(),key,length)==0));
        move_to_value();
        if(rightkey) { 
          return true;
        }
      } while(next());
      assert(up());// not found
    }
    return false;
}


 bool ParsedJson::iterator::prev() {
    if(location - 1 < depthindex[depth].start_of_scope) {
      return false;
    }
    location -= 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    if ((current_type == ']') || (current_type == '}')){
      // we need to jump
      size_t new_location = ( current_val & JSONVALUEMASK);
      if(new_location < depthindex[depth].start_of_scope) {
        return false; // shoud never happen
      }
      location = new_location;
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
    }
    return true;
}


 bool ParsedJson::iterator::up() {
    if(depth == 1) {
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


 bool ParsedJson::iterator::down() {
    if(location + 1 >= tape_length) {
      return false;
    }
    if ((current_type == '[') || (current_type == '{')) {
      size_t npos = (current_val & JSONVALUEMASK);
      if(npos == location + 2) {
        return false; // we have an empty scope
      }
      depth++;
      location = location + 1;
      depthindex[depth].start_of_scope = location;
      depthindex[depth].scope_type = current_type;
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
      return true;
    }
    return false;
}

void ParsedJson::iterator::to_start_scope()  {
    location = depthindex[depth].start_of_scope;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
}

bool ParsedJson::iterator::next() {
    size_t npos; 
    if ((current_type == '[') || (current_type == '{')){
      // we need to jump
      npos = ( current_val & JSONVALUEMASK);
    } else {
      npos = location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
    }
    uint64_t nextval = pj.tape[npos];
    uint8_t nexttype = (nextval >> 56);
    if((nexttype == ']') || (nexttype == '}')) {
        return false; // we reached the end of the scope
    }
    location = npos;
    current_val = nextval;
    current_type = nexttype;
    return true;
}
#endif
