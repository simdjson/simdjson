#ifndef SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H
#define SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H

#include "simdjson/dom/parsedjson_iterator.h"
#include "simdjson/portability.h"
#include <cstring>

namespace simdjson {

// VS2017 reports deprecated warnings when you define a deprecated class's methods.
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING

// Because of template weirdness, the actual class definition is inline in the document class

SIMDJSON_WARN_UNUSED bool dom::parser::Iterator::is_ok() const {
  return location < tape_length;
}

// useful for debugging purposes
size_t dom::parser::Iterator::get_tape_location() const {
  return location;
}

// useful for debugging purposes
size_t dom::parser::Iterator::get_tape_length() const {
  return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root
// node)
size_t dom::parser::Iterator::get_depth() const {
  return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an
// object ({) or an array ([). The root node has type 'r'.
uint8_t dom::parser::Iterator::get_scope_type() const {
  return depth_index[depth].scope_type;
}

bool dom::parser::Iterator::move_forward() {
  if (location + 1 >= tape_length) {
    return false; // we are at the end!
  }

  if ((current_type == '[') || (current_type == '{')) {
    // We are entering a new scope
    depth++;
    assert(depth < max_depth);
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
  } else if ((current_type == ']') || (current_type == '}')) {
    // Leaving a scope.
    depth--;
  } else if (is_number()) {
    // these types use 2 locations on the tape, not just one.
    location += 1;
  }

  location += 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

void dom::parser::Iterator::move_to_value() {
  // assume that we are on a key, so move by 1.
  location += 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
}

bool dom::parser::Iterator::move_to_key(const char *key) {
    if (down()) {
      do {
        const bool right_key = (strcmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
      up();
    }
    return false;
}

bool dom::parser::Iterator::move_to_key_insensitive(
    const char *key) {
    if (down()) {
      do {
        const bool right_key = (simdjson_strcasecmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
      up();
    }
    return false;
}

bool dom::parser::Iterator::move_to_key(const char *key,
                                                       uint32_t length) {
  if (down()) {
    do {
      bool right_key = ((get_string_length() == length) &&
                        (memcmp(get_string(), key, length) == 0));
      move_to_value();
      if (right_key) {
        return true;
      }
    } while (next());
    up();
  }
  return false;
}

bool dom::parser::Iterator::move_to_index(uint32_t index) {
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
    up();
  }
  return false;
}

bool dom::parser::Iterator::prev() {
  size_t target_location = location;
  to_start_scope();
  size_t npos = location;
  if (target_location == npos) {
    return false; // we were already at the start
  }
  size_t oldnpos;
  // we have that npos < target_location here
  do {
    oldnpos = npos;
    if ((current_type == '[') || (current_type == '{')) {
      // we need to jump
      npos = uint32_t(current_val);
    } else {
      npos = npos + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
    }
  } while (npos < target_location);
  location = oldnpos;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

bool dom::parser::Iterator::up() {
  if (depth == 1) {
    return false; // don't allow moving back to root
  }
  to_start_scope();
  // next we just move to the previous value
  depth--;
  location -= 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

bool dom::parser::Iterator::down() {
  if (location + 1 >= tape_length) {
    return false;
  }
  if ((current_type == '[') || (current_type == '{')) {
    size_t npos = uint32_t(current_val);
    if (npos == location + 2) {
      return false; // we have an empty scope
    }
    depth++;
    assert(depth < max_depth);
    location = location + 1;
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
    current_val = doc.tape[location];
    current_type = uint8_t(current_val >> 56);
    return true;
  }
  return false;
}

void dom::parser::Iterator::to_start_scope() {
  location = depth_index[depth].start_of_scope;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
}

bool dom::parser::Iterator::next() {
  size_t npos;
  if ((current_type == '[') || (current_type == '{')) {
    // we need to jump
    npos = uint32_t(current_val);
  } else {
    npos = location + (is_number() ? 2 : 1);
  }
  uint64_t next_val = doc.tape[npos];
  uint8_t next_type = uint8_t(next_val >> 56);
  if ((next_type == ']') || (next_type == '}')) {
    return false; // we reached the end of the scope
  }
  location = npos;
  current_val = next_val;
  current_type = next_type;
  return true;
}
dom::parser::Iterator::Iterator(const dom::parser &pj) noexcept(false)
    : doc(pj.doc)
{
#if SIMDJSON_EXCEPTIONS
  if (!pj.valid) { throw simdjson_error(pj.error); }
#else
  if (!pj.valid) { return; } //  abort() usage is forbidden in the library
#endif

  max_depth = pj.max_depth();
  depth_index = new scopeindex_t[max_depth + 1];
  depth_index[0].start_of_scope = location;
  current_val = doc.tape[location++];
  current_type = uint8_t(current_val >> 56);
  depth_index[0].scope_type = current_type;
  tape_length = size_t(current_val & internal::JSON_VALUE_MASK);
  if (location < tape_length) {
    // If we make it here, then depth_capacity must >=2, but the compiler
    // may not know this.
    current_val = doc.tape[location];
    current_type = uint8_t(current_val >> 56);
    depth++;
    assert(depth < max_depth);
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
  }
}
dom::parser::Iterator::Iterator(
    const dom::parser::Iterator &o) noexcept
    : doc(o.doc),
    max_depth(o.depth),
    depth(o.depth),
    location(o.location),
    tape_length(o.tape_length),
    current_type(o.current_type),
    current_val(o.current_val)
{
  depth_index = new scopeindex_t[max_depth+1];
  memcpy(depth_index, o.depth_index, (depth + 1) * sizeof(depth_index[0]));
}

dom::parser::Iterator::~Iterator() noexcept {
  if (depth_index) { delete[] depth_index; }
}

bool dom::parser::Iterator::print(std::ostream &os, bool escape_strings) const {
  if (!is_ok()) {
    return false;
  }
  switch (current_type) {
  case '"': // we have a string
    os << '"';
    if (escape_strings) {
      os << internal::escape_json_string(std::string_view(get_string(), get_string_length()));
    } else {
      // was: os << get_string();, but given that we can include null chars, we
      // have to do something crazier:
      std::copy(get_string(), get_string() + get_string_length(), std::ostream_iterator<char>(os));
    }
    os << '"';
    break;
  case 'l': // we have a long int
    os << get_integer();
    break;
  case 'u':
    os << get_unsigned_integer();
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
    os << char(current_type);
    break;
  default:
    return false;
  }
  return true;
}

bool dom::parser::Iterator::move_to(const char *pointer,
                                                   uint32_t length) {
  char *new_pointer = nullptr;
  if (pointer[0] == '#') {
    // Converting fragment representation to string representation
    new_pointer = new char[length];
    uint32_t new_length = 0;
    for (uint32_t i = 1; i < length; i++) {
      if (pointer[i] == '%' && pointer[i + 1] == 'x') {
#if __cpp_exceptions
        try {
#endif
          int fragment =
              std::stoi(std::string(&pointer[i + 2], 2), nullptr, 16);
          if (fragment == '\\' || fragment == '"' || (fragment <= 0x1F)) {
            // escaping the character
            new_pointer[new_length] = '\\';
            new_length++;
          }
          new_pointer[new_length] = char(fragment);
          i += 3;
#if __cpp_exceptions
        } catch (std::invalid_argument &) {
          delete[] new_pointer;
          return false; // the fragment is invalid
        }
#endif
      } else {
        new_pointer[new_length] = pointer[i];
      }
      new_length++;
    }
    length = new_length;
    pointer = new_pointer;
  }

  // saving the current state
  size_t depth_s = depth;
  size_t location_s = location;
  uint8_t current_type_s = current_type;
  uint64_t current_val_s = current_val;

  rewind(); // The json pointer is used from the root of the document.

  bool found = relative_move_to(pointer, length);
  delete[] new_pointer;

  if (!found) {
    // since the pointer has found nothing, we get back to the original
    // position.
    depth = depth_s;
    location = location_s;
    current_type = current_type_s;
    current_val = current_val_s;
  }

  return found;
}

bool dom::parser::Iterator::relative_move_to(const char *pointer,
                                                            uint32_t length) {
  if (length == 0) {
    // returns the whole document
    return true;
  }

  if (pointer[0] != '/') {
    // '/' must be the first character
    return false;
  }

  // finding the key in an object or the index in an array
  std::string key_or_index;
  uint32_t offset = 1;

  // checking for the "-" case
  if (is_array() && pointer[1] == '-') {
    if (length != 2) {
      // the pointer must be exactly "/-"
      // there can't be anything more after '-' as an index
      return false;
    }
    key_or_index = '-';
    offset = length; // will skip the loop coming right after
  }

  // We either transform the first reference token to a valid json key
  // or we make sure it is a valid index in an array.
  for (; offset < length; offset++) {
    if (pointer[offset] == '/') {
      // beginning of the next key or index
      break;
    }
    if (is_array() && (pointer[offset] < '0' || pointer[offset] > '9')) {
      // the index of an array must be an integer
      // we also make sure std::stoi won't discard whitespaces later
      return false;
    }
    if (pointer[offset] == '~') {
      // "~1" represents "/"
      if (pointer[offset + 1] == '1') {
        key_or_index += '/';
        offset++;
        continue;
      }
      // "~0" represents "~"
      if (pointer[offset + 1] == '0') {
        key_or_index += '~';
        offset++;
        continue;
      }
    }
    if (pointer[offset] == '\\') {
      if (pointer[offset + 1] == '\\' || pointer[offset + 1] == '"' ||
          (pointer[offset + 1] <= 0x1F)) {
        key_or_index += pointer[offset + 1];
        offset++;
        continue;
      }
      return false; // invalid escaped character
    }
    if (pointer[offset] == '\"') {
      // unescaped quote character. this is an invalid case.
      // lets do nothing and assume most pointers will be valid.
      // it won't find any corresponding json key anyway.
      // return false;
    }
    key_or_index += pointer[offset];
  }

  bool found = false;
  if (is_object()) {
    if (move_to_key(key_or_index.c_str(), uint32_t(key_or_index.length()))) {
      found = relative_move_to(pointer + offset, length - offset);
    }
  } else if (is_array()) {
    if (key_or_index == "-") { // handling "-" case first
      if (down()) {
        while (next())
          ; // moving to the end of the array
        // moving to the nonexistent value right after...
        size_t npos;
        if ((current_type == '[') || (current_type == '{')) {
          // we need to jump
          npos = uint32_t(current_val);
        } else {
          npos =
              location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
        }
        location = npos;
        current_val = doc.tape[npos];
        current_type = uint8_t(current_val >> 56);
        return true; // how could it fail ?
      }
    } else { // regular numeric index
      // The index can't have a leading '0'
      if (key_or_index[0] == '0' && key_or_index.length() > 1) {
        return false;
      }
      // it cannot be empty
      if (key_or_index.length() == 0) {
        return false;
      }
      // we already checked the index contains only valid digits
      uint32_t index = std::stoi(key_or_index);
      if (move_to_index(index)) {
        found = relative_move_to(pointer + offset, length - offset);
      }
    }
  }

  return found;
}

SIMDJSON_POP_DISABLE_WARNINGS

} // namespace simdjson

#endif // SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H
