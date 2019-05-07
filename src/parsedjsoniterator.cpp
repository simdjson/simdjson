#include "simdjson/parsedjson.h"
#include "simdjson/common_defs.h"
#include <iterator>

ParsedJson::iterator::iterator(ParsedJson &pj_) : pj(pj_), depth(0), location(0), tape_length(0), depthindex(nullptr) {
        if(!pj.isValid()) {
            throw InvalidJSON();
        }
        depthindex = new scopeindex_t[pj.depthcapacity];
        // memory allocation would throw
        //if(depthindex == nullptr) { 
        //    return;
        //}
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
        } else {
            // should never happen
            throw InvalidJSON();
        }
}

ParsedJson::iterator::~iterator() {
      delete[] depthindex;
}

ParsedJson::iterator::iterator(const iterator &o):
    pj(o.pj), depth(o.depth), location(o.location),
    tape_length(0), current_type(o.current_type),
    current_val(o.current_val), depthindex(nullptr) {
    depthindex = new scopeindex_t[pj.depthcapacity];
    // allocation might throw
    memcpy(depthindex, o.depthindex, pj.depthcapacity * sizeof(depthindex[0]));
    tape_length = o.tape_length;
}

ParsedJson::iterator::iterator(iterator &&o):
      pj(o.pj), depth(o.depth), location(o.location),
      tape_length(o.tape_length), current_type(o.current_type),
      current_val(o.current_val), depthindex(o.depthindex) {
        o.depthindex = nullptr;// we take ownership
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
        if(depth == 0) {
            // Should not be necessary
            return false;
        }
    } else if ((current_type == 'd') || (current_type == 'l')) {
        // d and l types use 2 locations on the tape, not just one.
        location += 1;
    }

    location += 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    return true;
}

uint8_t ParsedJson::iterator::get_type()  const {
    return current_type;
}


int64_t ParsedJson::iterator::get_integer()  const {
    if(location + 1 >= tape_length) { 
      return 0;// default value in case of error
    }
    return static_cast<int64_t>(pj.tape[location + 1]);
}

double ParsedJson::iterator::get_double()  const {
    if(location + 1 >= tape_length) { 
      return NAN;// default value in case of error
    }
    double answer;
    memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
    return answer;
}

const char * ParsedJson::iterator::get_string() const {
   return  reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK) + sizeof(uint32_t)) ;
}


uint32_t ParsedJson::iterator::get_string_length() const {
    uint32_t answer;
    memcpy(&answer, reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK)), sizeof(uint32_t));
    return answer;
}

bool ParsedJson::iterator::is_object_or_array() const {
    return is_object_or_array(get_type());
}

bool ParsedJson::iterator::is_object() const {
    return get_type() == '{';
}

bool ParsedJson::iterator::is_array() const {
    return get_type() == '[';
}

bool ParsedJson::iterator::is_string() const {
    return get_type() == '"';
}

bool ParsedJson::iterator::is_integer() const {
    return get_type() == 'l';
}

bool ParsedJson::iterator::is_double() const {
    return get_type() == 'd';
}

bool ParsedJson::iterator::is_true() const {
    return get_type() == 't';
}

bool ParsedJson::iterator::is_false() const {
    return get_type() == 'f';
}

bool ParsedJson::iterator::is_null() const {
    return get_type() == 'n';
}

bool ParsedJson::iterator::is_object_or_array(uint8_t type) {
    return (type == '[' || (type == '{'));
}

bool ParsedJson::iterator::move_to_key(const char * key) {
    if(down()) {
      do {
        assert(is_string());
        bool rightkey = (strcmp(get_string(),key)==0);// null chars would fool this
        next();
        if(rightkey) { 
          return true;
        }
      } while(next());
      assert(up());// not found
    }
    return false;
}


 bool ParsedJson::iterator::next() {
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
    } 
    size_t increment = (current_type == 'd' || current_type == 'l') ? 2 : 1;
    if(location + increment >= tape_length) { return false;
}
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


 bool ParsedJson::iterator::prev() {
    if(location - 1 < depthindex[depth].start_of_scope) { return false;
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
    if(location + 1 >= tape_length) { return false;
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

bool ParsedJson::iterator::print(std::ostream &os, bool escape_strings) const {
    if(!isOk()) { 
      return false;
    }
    switch (current_type) {
    case '"': // we have a string
    os << '"';
    if(escape_strings) {
        print_with_escapes(get_string(), os, get_string_length());
    } else {
        // was: os << get_string();, but given that we can include null chars, we have to do something crazier:
        std::copy(get_string(), get_string() + get_string_length(), std::ostream_iterator<char>(os));
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
    os << static_cast<char>(current_type);
    break;
    default:
    return false;
    }
    return true;
}
