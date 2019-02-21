#include "simdjson/parsedjsoniterator.h"
#include "simdjson/parsedjson.h"
#include "simdjson/common_defs.h"

ParsedJsonIterator::ParsedJsonIterator(ParsedJson &pj_) : pj(pj_), depth(0), location(0), tape_length(0), depthindex(NULL) {
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

ParsedJsonIterator::~ParsedJsonIterator() {
      delete[] depthindex;
}

ParsedJsonIterator::ParsedJsonIterator(const ParsedJsonIterator &o):
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

ParsedJsonIterator::ParsedJsonIterator(ParsedJsonIterator &&o):
      pj(o.pj), depth(std::move(o.depth)), location(std::move(o.location)),
      tape_length(std::move(o.tape_length)), current_type(std::move(o.current_type)),
      current_val(std::move(o.current_val)), depthindex(std::move(o.depthindex)) {
        o.depthindex = NULL;// we take ownership
}

WARN_UNUSED
bool ParsedJsonIterator::isOk() const {
      return location < tape_length;
}

// useful for debuging purposes
size_t ParsedJsonIterator::get_tape_location() const {
    return location;
}

// useful for debuging purposes
size_t ParsedJsonIterator::get_tape_length() const {
    return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root node)
size_t ParsedJsonIterator::get_depth() const {
    return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
// The root node has type 'r'.
uint8_t ParsedJsonIterator::get_scope_type() const {
    return depthindex[depth].scope_type;
}

bool ParsedJsonIterator::move_forward() {
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

//really_inline
 uint8_t ParsedJsonIterator::get_type()  const {
    return current_type;
}

//really_inline

 int64_t ParsedJsonIterator::get_integer()  const {
    if(location + 1 >= tape_length) return 0;// default value in case of error
    return (int64_t) pj.tape[location + 1];
}

//really_inline
 double ParsedJsonIterator::get_double()  const {
    if(location + 1 >= tape_length) return NAN;// default value in case of error
    double answer;
    memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
    return answer;
}

//really_inline
 const char * ParsedJsonIterator::get_string() const {
    return  (const char *)(pj.string_buf + (current_val & JSONVALUEMASK)) ;
}


bool ParsedJsonIterator::is_object_or_array() const {
    return is_object_or_array(get_type());
}

bool ParsedJsonIterator::is_object() const {
    return get_type() == '{';
}

bool ParsedJsonIterator::is_array() const {
    return get_type() == '[';
}

bool ParsedJsonIterator::is_string() const {
    return get_type() == '"';
}

bool ParsedJsonIterator::is_integer() const {
    return get_type() == 'l';
}

bool ParsedJsonIterator::is_double() const {
    return get_type() == 'd';
}

bool ParsedJsonIterator::is_object_or_array(uint8_t type) {
    return (type == '[' || (type == '{'));
}

bool ParsedJsonIterator::move_to_key(const char * key) {
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

//really_inline
 bool ParsedJsonIterator::next() {
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

//really_inline
 bool ParsedJsonIterator::prev() {
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

//really_inline
 bool ParsedJsonIterator::up() {
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

//really_inline
 bool ParsedJsonIterator::down() {
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

void ParsedJsonIterator::to_start_scope()  {
    location = depthindex[depth].start_of_scope;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
}

bool ParsedJsonIterator::print(std::ostream &os, bool escape_strings) const {
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
