#pragma once
 #include <math.h> 
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

#define JSONVALUEMASK 0xFFFFFFFFFFFFFF

#define DEFAULTMAXDEPTH 1024// a JSON document with a depth exceeding 1024 is probably de facto invalid

struct ParsedJson {
public:

  // create a ParsedJson container with zero capacity, call allocateCapacity to
  // allocate memory
  ParsedJson()
      : bytecapacity(0), depthcapacity(0), tapecapacity(0), stringcapacity(0),
        current_loc(0), structurals(NULL), n_structural_indexes(0),
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
    if (posix_memalign((void **)&structurals, 8, ROUNDUP_N(len, 64) / 8)) {
      std::cerr << "Could not allocate memory for structurals" << std::endl;
      return false;
    };
    n_structural_indexes = 0;
    u32 max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new u32[max_structures];
    size_t localtapecapacity = ROUNDUP_N(len, 64);
    size_t localstringcapacity = ROUNDUP_N(len, 64);
    string_buf = new u8[localstringcapacity];
    tape = new u64[localtapecapacity];
    containing_scope_offset = new u32[maxdepth];
    ret_address = new void *[maxdepth];

    if ((string_buf == NULL) || (tape == NULL) ||
        (containing_scope_offset == NULL) || (ret_address == NULL) || (structural_indexes == NULL)) {
      std::cerr << "Could not allocate memory" << std::endl;
      delete[] ret_address;
      delete[] containing_scope_offset;
      delete[] tape;
      delete[] string_buf;
      delete[] structural_indexes;
      free(structurals);
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
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;
    free(structurals);
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
          os << ",  ";
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}'))
          os << ",  ";
        if (((inobjectidx[depth] & 1) == 1))
          os << " : ";
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
        os << '\n';
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
        os << '\n';
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
        printf("should we be hitting the root node?\n");
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      default:
        printf("bug %c\n", type);
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
    u64 tape_val = tape[tapeidx++];
    u8 type = (tape_val >> 56);
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      printf("Error: no starting root node?");
      return false;
    }
    for (; tapeidx < howmany; tapeidx++) {
      os << tapeidx << " : "; 
      tape_val = tape[tapeidx];
      u64 payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      switch (type) {
      case '"': // we have a string
        os << "string \""; 
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
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
        os << "{\t// pointing to next tape location " << payload << "\n";
        break;
      case '}': // we end an object
        os << "}\t// pointing to previous tape location " << payload << "\n";
        break;
      case '[': // we start an array
        os << "[\t// pointing to next tape location " << payload << "\n";
        break;
      case ']': // we end an array
        os << "]\t// pointing to previous tape location " << payload << "\n";
        break;
      case 'r': // we start and end with the root node
        printf("end of root\n");
        return false;
      default:
        return false;
      }
    }
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



  struct iterator {

    explicit iterator(ParsedJson &pj_)
        : pj(pj_), depth(0), location(0), tape_length(0), depthindex(NULL) {
        if(pj.isValid()) {
            depthindex = new size_t[pj.depthcapacity];
            if(depthindex == NULL) return;
            depthindex[0] = 0;
            current_val = pj.tape[location++];
            current_type = (current_val >> 56);
            if (current_type == 'r') {
              tape_length = current_val & JSONVALUEMASK;
              if(location < tape_length) {
                current_val = pj.tape[location];
                current_type = (current_val >> 56);
                depth++;
                depthindex[depth] = location;
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
        depthindex = new size_t[pj.depthcapacity];
        if(depthindex != NULL) {
          memcpy(o.depthindex, depthindex, pj.depthcapacity * sizeof(depthindex[0]));
        } else {
          tape_length = 0;
        }
    }

    iterator(iterator &&o): 
      pj(o.pj), depth(o.depth), location(o.location), 
      tape_length(o.tape_length), current_type(o.current_type), 
      current_val(o.current_val), depthindex(o.depthindex) {
        o.depthindex = NULL;// we take ownship
    }
    WARN_UNUSED
    bool isOk() const {
      return location < tape_length;
    }

    size_t get_tape_location() const {
      return location;
    }

    size_t get_tape_lenght() const {
      return tape_length;
    }

    // return true if we can do the navigation, false
    // otherwise

    // withing a give scope, we move forward
    // valid if we're not at the end of a scope (returns true)
    WARN_UNUSED
    really_inline bool next() { 
      if ((current_type == '[') || (current_type == '{')){
        // we need to jump
        size_t npos = ( current_val & JSONVALUEMASK); 
        if(npos >= tape_length) {
          return false; // shoud never happen unless at the root
        }
        u64 nextval = pj.tape[npos];
        u8 nexttype = (nextval >> 56);
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
        u64 nextval = pj.tape[location + increment];
        u8 nexttype = (nextval >> 56);
        if((nexttype == ']') || (nexttype == '}')) {
          return false; // we reached the end of the scope
        }
        location = location + increment;
        current_val = nextval;
        current_type = nexttype;
        return true;
      }
    }

    // valid if we're not at the start of a scope
    WARN_UNUSED
    really_inline bool prev() {
      if(location - 1 < depthindex[depth]) return false;
      location -= 1;
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
      if ((current_type == ']') || (current_type == '}')){
        // we need to jump
        size_t new_location = ( current_val & JSONVALUEMASK); 
        if(new_location < depthindex[depth]) {
          return false; // shoud never happen 
        }
        location = new_location;
        current_val = pj.tape[location];
        current_type = (current_val >> 56);
      }
      return true;
    }


    // valid unless we are at the first level of the document
    WARN_UNUSED
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
 
    
    // valid if we're at a [ or { and it starts a non-empty scope; moves us to start of
    // that deeper scope if it not empty
    WARN_UNUSED
    really_inline bool down() {
      if(location + 1 >= tape_length) return false;
      if ((current_type == '[') || (current_type == '{')) {
        size_t npos = (current_val & JSONVALUEMASK); 
        if(npos == location + 2) {
          return false; // we have an empty scope
        }
        depth++;
        location = location + 1;
        depthindex[depth] = location;
        current_val = pj.tape[location];
        current_type = (current_val >> 56);
        return true;
      } 
      return false;
    }

    // move us to the start of our current scope
    void to_start_scope()  {             
      location = depthindex[depth];
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

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    really_inline u8 get_type()  const {
      return current_type;
    }

    // get the s64 value at this node; valid only if we're at "l"
    really_inline s64 get_integer()  const {
       if(location + 1 >= tape_length) return 0;// default value in case of error
       return (s64) pj.tape[location + 1];
    }

    // get the double value at this node; valid only if
    // we're at "d"
    really_inline double get_double()  const {
       if(location + 1 >= tape_length) return NAN;// default value in case of error
       double answer;
       memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
       return answer;
    } 

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see print_with_escapes)
    // return value is valid UTF-8
    really_inline const char * get_string() const {
      return  (const char *)(pj.string_buf + (current_val & JSONVALUEMASK)) ;
    }

private:

    iterator& operator=(const iterator& other) ;

    ParsedJson &pj;
    size_t depth;
    size_t location;     // our current location on a tape
    size_t tape_length; 
    u8 current_type;
    u64 current_val;
    size_t *depthindex;

  };


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
  bool isvalid;
   ParsedJson(const ParsedJson && p); // we don't want the default constructor to be called

private :
 ParsedJson(const ParsedJson & p); // we don't want the default constructor to be called


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
