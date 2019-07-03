#include "simdjson/parsedjson.h"
#include "simdjson/common_defs.h"
#include <iterator>

namespace simdjson {
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
}
