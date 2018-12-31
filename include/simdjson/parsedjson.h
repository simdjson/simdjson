#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <iomanip>
#include <iostream>

#include "simdjson/portability.h"
#include "simdjson/jsonformatutils.h"

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
  ParsedJson()
      : bytecapacity(0), depthcapacity(0), tapecapacity(0), stringcapacity(0),
        current_loc(0), n_structural_indexes(0),
        structural_indexes(NULL), tape(NULL), containing_scope_offset(NULL),
        ret_address(NULL), string_buf(NULL), current_string_buf_loc(NULL), isvalid(false) {}

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to len butes and maxdepth "depth"
  WARN_UNUSED
  inline bool allocateCapacity(size_t len, size_t maxdepth = DEFAULTMAXDEPTH) {
    if ((maxdepth == 0) || (len == 0)) {
      std::cerr << "capacities must be non-zero " << std::endl;
      return false;
    }
    if (len > 0) {
      if ((len <= bytecapacity) && (depthcapacity < maxdepth))
        return true;
      deallocate();
    }
    isvalid = false;
    bytecapacity = 0; // will only set it to len after allocations are a success
    n_structural_indexes = 0;
    uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new uint32_t[max_structures];
    size_t localtapecapacity = ROUNDUP_N(len, 64);
    size_t localstringcapacity = ROUNDUP_N(len, 64);
    string_buf = new uint8_t[localstringcapacity];
    tape = new uint64_t[localtapecapacity];
    containing_scope_offset = new uint32_t[maxdepth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
    ret_address = new void *[maxdepth];
#else
    ret_address = new char[maxdepth];
#endif
    if ((string_buf == NULL) || (tape == NULL) ||
        (containing_scope_offset == NULL) || (ret_address == NULL) || (structural_indexes == NULL)) {
      std::cerr << "Could not allocate memory" << std::endl;
      if(ret_address != NULL) delete[] ret_address;
      if(containing_scope_offset != NULL) delete[] containing_scope_offset;
      if(tape != NULL) delete[] tape;
      if(string_buf != NULL) delete[] string_buf;
      if(structural_indexes != NULL) delete[] structural_indexes;
      return false;
    }

    bytecapacity = len;
    depthcapacity = maxdepth;
    tapecapacity = localtapecapacity;
    stringcapacity = localstringcapacity;
    return true;
  }

  bool isValid() const {
    return isvalid;
  }

  // deallocate memory and set capacity to zero, called automatically by the
  // destructor
  void deallocate() {
    bytecapacity = 0;
    depthcapacity = 0;
    tapecapacity = 0;
    stringcapacity = 0;
    if(ret_address != NULL) delete[] ret_address;
    if(containing_scope_offset != NULL) delete[] containing_scope_offset;
    if(tape != NULL) delete[] tape;
    if(string_buf != NULL) delete[] string_buf;
    if(structural_indexes != NULL) delete[] structural_indexes;
    isvalid = false;
  }

  ~ParsedJson() { deallocate(); }

  // this should be called when parsing (right before writing the tapes)
  void init() {
    current_string_buf_loc = string_buf;
    current_loc = 0;
    isvalid = false;
  }

  // print the json to stdout (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool printjson(std::ostream &os) {
    if(!isvalid) return false;
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    if (howmany > tapecapacity) {
      fprintf(stderr, 
          "We may be exceeding the tape capacity. Is this a valid document?\n");
      return false;
    }
    tapeidx++;
    bool *inobject = new bool[depthcapacity];
    size_t *inobjectidx = new size_t[depthcapacity];
    int depth = 1; // only root at level 0
    inobjectidx[depth] = 0;
    inobject[depth] = false;
    for (; tapeidx < howmany; tapeidx++) {
      tape_val = tape[tapeidx];
      uint64_t payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      if (!inobject[depth]) {
        if ((inobjectidx[depth] > 0) && (type != ']'))
          os << ",";
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}'))
          os << ",";
        if (((inobjectidx[depth] & 1) == 1))
          os << ":";
        inobjectidx[depth]++;
      }
      switch (type) {
      case '"': // we have a string
        os << '"';
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany)
          return false;
        os <<  (int64_t)tape[++tapeidx];
        break;
      case 'd': // we have a double
        if (tapeidx + 1 >= howmany)
          return false;
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
        os << answer;
        break;
      case 'n': // we have a null
        os << "null";
        break;
      case 't': // we have a true
        os << "true";
        break;
      case 'f': // we have a false
        os << "false";
        break;
      case '{': // we have an object
        os << '{';
        depth++;
        inobject[depth] = true;
        inobjectidx[depth] = 0;
        break;
      case '}': // we end an object
        depth--;
        os << '}';
        break;
      case '[': // we start an array
        os << '[';
        depth++;
        inobject[depth] = false;
        inobjectidx[depth] = 0;
        break;
      case ']': // we end an array
        depth--;
        os << ']';
        break;
      case 'r': // we start and end with the root node
        fprintf(stderr, "should we be hitting the root node?\n");
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      default:
        fprintf(stderr, "bug %c\n", type);
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      }
    }
    delete[] inobject;
    delete[] inobjectidx;
    return true;
  }

  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) {
    if(!isvalid) return false;
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    os << tapeidx << " : " << type;
    tapeidx++;
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    os << "\t// pointing to " << howmany <<" (right after last node)\n";
    uint64_t payload;
    for (; tapeidx < howmany; tapeidx++) {
      os << tapeidx << " : ";
      tape_val = tape[tapeidx];
      payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      switch (type) {
      case '"': // we have a string
        os << "string \"";
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
        os << '\n';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany)
          return false;
        os << "integer " << (int64_t)tape[++tapeidx] << "\n";
        break;
      case 'd': // we have a double
        os << "float ";
        if (tapeidx + 1 >= howmany)
          return false;
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
        os << answer << '\n';
        break;
      case 'n': // we have a null
        os << "null\n";
        break;
      case 't': // we have a true
        os << "true\n";
        break;
      case 'f': // we have a false
        os << "false\n";
        break;
      case '{': // we have an object
        os << "{\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case '}': // we end an object
        os << "}\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case '[': // we start an array
        os << "[\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case ']': // we end an array
        os << "]\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case 'r': // we start and end with the root node
        printf("end of root\n");
        return false;
      default:
        return false;
      }
    }
    tape_val = tape[tapeidx];
    payload = tape_val & JSONVALUEMASK;
    type = (tape_val >> 56);
    os << tapeidx << " : "<< type <<"\t// pointing to " << payload <<" (start root)\n";
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



  struct iterator {

    explicit iterator(ParsedJson &pj_)
        : pj(pj_), depth(0), location(0), tape_length(0), depthindex(NULL) {
        if(pj.isValid()) {
            depthindex = new scopeindex_t[pj.depthcapacity];
            if(depthindex == NULL) return;
            depthindex[0].start_of_scope = location;
            current_val = pj.tape[location++];
            current_type = (current_val >> 56);
            depthindex[0].scope_type = current_type;
            if (current_type == 'r') {
              tape_length = current_val & JSONVALUEMASK;
              if(location < tape_length) {
                current_val = pj.tape[location];
                current_type = (current_val >> 56);
                depth++;
                depthindex[depth].start_of_scope = location;
                depthindex[depth].scope_type = current_type;
              }
            }
        }
    }
    ~iterator() {
      delete[] depthindex;
    }

    iterator(const iterator &o):
      pj(o.pj), depth(o.depth), location(o.location),
      tape_length(o.tape_length), current_type(o.current_type),
      current_val(o.current_val), depthindex(NULL) {
        depthindex = new scopeindex_t[pj.depthcapacity];
        if(depthindex != NULL) {
          memcpy(o.depthindex, depthindex, pj.depthcapacity * sizeof(depthindex[0]));
        } else {
          tape_length = 0;
        }
    }

    iterator(iterator &&o):
      pj(o.pj), depth(std::move(o.depth)), location(std::move(o.location)),
      tape_length(std::move(o.tape_length)), current_type(std::move(o.current_type)),
      current_val(std::move(o.current_val)), depthindex(std::move(o.depthindex)) {
        o.depthindex = NULL;// we take ownership
    }

    WARN_UNUSED
    bool isOk() const {
      return location < tape_length;
    }

    // useful for debuging purposes
    size_t get_tape_location() const {
      return location;
    }

    // useful for debuging purposes
    size_t get_tape_length() const {
      return tape_length;
    }

    // returns the current depth (start at 1 with 0 reserved for the fictitious root node)
    size_t get_depth() const {
      return depth;
    }

    // A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
    // The root node has type 'r'.
    uint8_t get_scope_type() const {
      return depthindex[depth].scope_type;
    }

    // move forward in document order
    bool move_forward() {
      if(location + 1 >= tape_length) {
        return false; // we are at the end!
      }
      // we are entering a new scope
      if ((current_type == '[') || (current_type == '{')){
        depth++;
        depthindex[depth].start_of_scope = location;
        depthindex[depth].scope_type = current_type;
      }
      location = location + 1;
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
      // if we encounter a scope closure, we need to move up
      while ((current_type == ']') || (current_type == '}')) {
        if(location + 1 >= tape_length) {
          return false; // we are at the end!
        }
        depth--;
        if(depth == 0) {
          return false; // should not be necessary
        }
        location = location + 1;
        current_val = pj.tape[location];
        current_type = (current_val >> 56);
      }
      return true;
    }

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    really_inline uint8_t get_type()  const {
      return current_type;
    }

    // get the int64_t value at this node; valid only if we're at "l"
    really_inline int64_t get_integer()  const {
       if(location + 1 >= tape_length) return 0;// default value in case of error
       return (int64_t) pj.tape[location + 1];
    }

    // get the double value at this node; valid only if
    // we're at "d"
    really_inline double get_double()  const {
       if(location + 1 >= tape_length) return NAN;// default value in case of error
       double answer;
       memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
       return answer;
    }

    bool is_object_or_array() const {
      return is_object_or_array(get_type());
    }

    bool is_object() const {
      return get_type() == '{';
    }

    bool is_array() const {
      return get_type() == '[';
    }

    bool is_string() const {
      return get_type() == '"';
    }

    bool is_integer() const {
      return get_type() == 'l';
    }

    bool is_double() const {
      return get_type() == 'd';
    }

    static bool is_object_or_array(uint8_t type) {
      return (type == '[' || (type == '{'));
    }

    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one)
    bool move_to_key(const char * key) {
      if(down()) {
        do {
          assert(is_string());
          bool rightkey = (strcmp(get_string(),key)==0);
          next();
          if(rightkey) return true;
        } while(next());
        assert(up());// not found
      }
      return false;
    }

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see print_with_escapes)
    // return value is valid UTF-8
    really_inline const char * get_string() const {
      return  (const char *)(pj.string_buf + (current_val & JSONVALUEMASK)) ;
    }

    // throughout return true if we can do the navigation, false
    // otherwise

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move forward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, { and [.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    // valid if we're not at the end of a scope (returns true).
    really_inline bool next() {
      if ((current_type == '[') || (current_type == '{')){
        // we need to jump
        size_t npos = ( current_val & JSONVALUEMASK);
        if(npos >= tape_length) {
          return false; // shoud never happen unless at the root
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
      } else {
        size_t increment = (current_type == 'd' || current_type == 'l') ? 2 : 1;
        if(location + increment >= tape_length) return false;
        uint64_t nextval = pj.tape[location + increment];
        uint8_t nexttype = (nextval >> 56);
        if((nexttype == ']') || (nexttype == '}')) {
          return false; // we reached the end of the scope
        }
        location = location + increment;
        current_val = nextval;
        current_type = nexttype;
        return true;
      }
    }



    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move backward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true when starting at the end
    // of the scope.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    really_inline bool prev() {
      if(location - 1 < depthindex[depth].start_of_scope) return false;
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

    // Moves back to either the containing array or object (type { or [) from
    // within a contained scope.
    // Valid unless we are at the first level of the document
    //
    really_inline bool up() {
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


    // Valid if we're at a [ or { and it starts a non-empty scope; moves us to start of
    // that deeper scope if it not empty.
    // Thus, given [true, null, {"a":1}, [1,2]], if we are at the { node, we would move to the
    // "a" node.
    really_inline bool down() {
      if(location + 1 >= tape_length) return false;
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

    // move us to the start of our current scope,
    // a scope is a series of nodes at the same level
    void to_start_scope()  {
      location = depthindex[depth].start_of_scope;
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
    }

    // void to_end_scope();              // move us to
    // the start of our current scope; always succeeds

    // print the thing we're currently pointing at
    bool print(std::ostream &os, bool escape_strings = true) const {
      if(!isOk()) return false;
      switch (current_type) {
      case '"': // we have a string
        os << '"';
        if(escape_strings) {
          print_with_escapes(get_string(), os);
        } else {
          os << get_string();
        }
        os << '"';
        break;
      case 'l': // we have a long int
        os << get_integer();
        break;
      case 'd':
        os << get_double();
        break;
      case 'n': // we have a null
        os << "null";
        break;
      case 't': // we have a true
        os << "true";
        break;
      case 'f': // we have a false
        os << "false";
        break;
      case '{': // we have an object
      case '}': // we end an object
      case '[': // we start an array
      case ']': // we end an array
        os << (char) current_type;
        break;
      default:
        return false;
      }
      return true;
    }

    typedef struct {size_t start_of_scope; uint8_t scope_type;} scopeindex_t;

private:

    iterator& operator=(const iterator& other) ;

    ParsedJson &pj;
    size_t depth;
    size_t location;     // our current location on a tape
    size_t tape_length;
    uint8_t current_type;
    uint64_t current_val;
    scopeindex_t *depthindex;

  };


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

  ParsedJson(ParsedJson && p)
      : bytecapacity(std::move(p.bytecapacity)), 
        depthcapacity(std::move(p.depthcapacity)), 
        tapecapacity(std::move(p.tapecapacity)), 
        stringcapacity(std::move(p.stringcapacity)),
        current_loc(std::move(p.current_loc)), 
        n_structural_indexes(std::move(p.n_structural_indexes)),
        structural_indexes(std::move(p.structural_indexes)), 
        tape(std::move(p.tape)), 
        containing_scope_offset(std::move(p.containing_scope_offset)),
        ret_address(std::move(p.ret_address)), 
        string_buf(std::move(p.string_buf)), 
        current_string_buf_loc(std::move(p.current_string_buf_loc)), 
        isvalid(std::move(p.isvalid)) {
          p.structural_indexes=NULL;
          p.tape=NULL;
          p.containing_scope_offset=NULL;
          p.ret_address=NULL;
          p.string_buf=NULL;
          p.current_string_buf_loc=NULL;
        }

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
