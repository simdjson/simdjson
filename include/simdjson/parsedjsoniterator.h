#ifndef SIMDJSON_PARSEDJSONITERATOR_H
#define SIMDJSON_PARSEDJSONITERATOR_H

#include "simdjson/parsedjson.h"
#include "simdjson/jsonformatutils.h"
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>

namespace simdjson {
template <size_t max_depth> class ParsedJson::BasicIterator {
  // might throw InvalidJSON if ParsedJson is invalid
public:
  explicit BasicIterator(ParsedJson &pj_);

  BasicIterator(const BasicIterator &o) noexcept;
  BasicIterator &operator=(const BasicIterator &o) noexcept;

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
  // [{"slutfn are the possibilities
  inline uint8_t get_type() const {
    return current_type; // short functions should be inlined!
  }

  // get the int64_t value at this node; valid only if get_type is "l"
  inline int64_t get_integer() const {
    if (location + 1 >= tape_length) {
      return 0; // default value in case of error
    }
    return static_cast<int64_t>(pj->tape[location + 1]);
  }

  // get the value as uint64; valid only if  if get_type is "u"
  inline uint64_t get_unsigned_integer() const {
    if (location + 1 >= tape_length) {
      return 0; // default value in case of error
    }
    return pj->tape[location + 1];
  }

  // get the string value at this node (NULL ended); valid only if get_type is "
  // note that tabs, and line endings are escaped in the returned value (see
  // print_with_escapes) return value is valid UTF-8, it may contain NULL chars
  // within the string: get_string_length determines the true string length.
  inline const char *get_string() const {
    return reinterpret_cast<const char *>(
        pj->string_buf + (current_val & JSON_VALUE_MASK) + sizeof(uint32_t));
  }

  // return the length of the string in bytes
  inline uint32_t get_string_length() const {
    uint32_t answer;
    memcpy(&answer,
           reinterpret_cast<const char *>(pj->string_buf +
                                          (current_val & JSON_VALUE_MASK)),
           sizeof(uint32_t));
    return answer;
  }

  // get the double value at this node; valid only if
  // get_type() is "d"
  inline double get_double() const {
    if (location + 1 >= tape_length) {
      return std::numeric_limits<double>::quiet_NaN(); // default value in
                                                       // case of error
    }
    double answer;
    memcpy(&answer, &pj->tape[location + 1], sizeof(answer));
    return answer;
  }

  inline bool is_object_or_array() const { return is_object() || is_array(); }

  inline bool is_object() const { return get_type() == '{'; }

  inline bool is_array() const { return get_type() == '['; }

  inline bool is_string() const { return get_type() == '"'; }

  // Returns true if the current type of node is an signed integer.
  // You can get its value with `get_integer()`.
  inline bool is_integer() const { return get_type() == 'l'; }

  // Returns true if the current type of node is an unsigned integer.
  // You can get its value with `get_unsigned_integer()`.
  //
  // NOTE:
  // Only a large value, which is out of range of a 64-bit signed integer, is
  // represented internally as an unsigned node. On the other hand, a typical
  // positive integer, such as 1, 42, or 1000000, is as a signed node.
  // Be aware this function returns false for a signed node.
  inline bool is_unsigned_integer() const { return get_type() == 'u'; }

  inline bool is_double() const { return get_type() == 'd'; }

  inline bool is_number() const {
    return is_integer() || is_unsigned_integer() || is_double();
  }

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
  // Furthermore, we do the comparison character-by-character
  // without taking into account Unicode equivalence.
  inline bool move_to_key(const char *key);

  // as above, but case insensitive lookup (strcmpi instead of strcmp)
  inline bool move_to_key_insensitive(const char *key);

  // when at {, go one level deep, looking for a given key
  // if successful, we are left pointing at the value,
  // if not, we are still pointing at the object ({)
  // (in case of repeated keys, this only finds the first one).
  // The string we search for can contain NULL values.
  // Furthermore, we do the comparison character-by-character
  // without taking into account Unicode equivalence.
  inline bool move_to_key(const char *key, uint32_t length);

  // when at a key location within an object, this moves to the accompanying
  // value (located next to it). This is equivalent but much faster than
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

  // Within a given scope (series of nodes at the same depth within either an
  // array or an object), we move backward.
  // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true
  // when starting at the end of the scope. At the object ({) or at the array
  // ([), you can issue a "down" to visit their content.
  // Performance warning: This function is implemented by starting again
  // from the beginning of the scope and scanning forward. You should expect
  // it to be relatively slow.
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

  // print the node we are currently pointing at
  bool print(std::ostream &os, bool escape_strings = true) const;
  typedef struct {
    size_t start_of_scope;
    uint8_t scope_type;
  } scopeindex_t;

private:
  ParsedJson *pj;
  size_t depth;
  size_t location; // our current location on a tape
  size_t tape_length;
  uint8_t current_type;
  uint64_t current_val;
  scopeindex_t depth_index[max_depth];
};

template <size_t max_depth>
WARN_UNUSED bool ParsedJson::BasicIterator<max_depth>::is_ok() const {
  return location < tape_length;
}

// useful for debuging purposes
template <size_t max_depth>
size_t ParsedJson::BasicIterator<max_depth>::get_tape_location() const {
  return location;
}

// useful for debuging purposes
template <size_t max_depth>
size_t ParsedJson::BasicIterator<max_depth>::get_tape_length() const {
  return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root
// node)
template <size_t max_depth>
size_t ParsedJson::BasicIterator<max_depth>::get_depth() const {
  return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an
// object ({) or an array ([). The root node has type 'r'.
template <size_t max_depth>
uint8_t ParsedJson::BasicIterator<max_depth>::get_scope_type() const {
  return depth_index[depth].scope_type;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_forward() {
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
  current_val = pj->tape[location];
  current_type = (current_val >> 56);
  return true;
}

template <size_t max_depth>
void ParsedJson::BasicIterator<max_depth>::move_to_value() {
  // assume that we are on a key, so move by 1.
  location += 1;
  current_val = pj->tape[location];
  current_type = (current_val >> 56);
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_to_key(const char *key) {
    if (down()) {
      do {
        const bool right_key = (strcmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
    }
    return false;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_to_key_insensitive(
    const char *key) {
    if (down()) {
      do {
        const bool right_key = (simdjson_strcasecmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
    }
    return false;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_to_key(const char *key,
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
  }
  return false;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_to_index(uint32_t index) {
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
  }
  return false;
}

template <size_t max_depth> bool ParsedJson::BasicIterator<max_depth>::prev() {
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
      npos = (current_val & JSON_VALUE_MASK);
    } else {
      npos = npos + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
    }
  } while (npos < target_location);
  location = oldnpos;
  current_val = pj->tape[location];
  current_type = current_val >> 56;
  return true;
}

template <size_t max_depth> bool ParsedJson::BasicIterator<max_depth>::up() {
  if (depth == 1) {
    return false; // don't allow moving back to root
  }
  to_start_scope();
  // next we just move to the previous value
  depth--;
  location -= 1;
  current_val = pj->tape[location];
  current_type = (current_val >> 56);
  return true;
}

template <size_t max_depth> bool ParsedJson::BasicIterator<max_depth>::down() {
  if (location + 1 >= tape_length) {
    return false;
  }
  if ((current_type == '[') || (current_type == '{')) {
    size_t npos = (current_val & JSON_VALUE_MASK);
    if (npos == location + 2) {
      return false; // we have an empty scope
    }
    depth++;
    assert(depth < max_depth);
    location = location + 1;
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
    current_val = pj->tape[location];
    current_type = (current_val >> 56);
    return true;
  }
  return false;
}

template <size_t max_depth>
void ParsedJson::BasicIterator<max_depth>::to_start_scope() {
  location = depth_index[depth].start_of_scope;
  current_val = pj->tape[location];
  current_type = (current_val >> 56);
}

template <size_t max_depth> bool ParsedJson::BasicIterator<max_depth>::next() {
  size_t npos;
  if ((current_type == '[') || (current_type == '{')) {
    // we need to jump
    npos = (current_val & JSON_VALUE_MASK);
  } else {
    npos = location + (is_number() ? 2 : 1);
  }
  uint64_t next_val = pj->tape[npos];
  uint8_t next_type = (next_val >> 56);
  if ((next_type == ']') || (next_type == '}')) {
    return false; // we reached the end of the scope
  }
  location = npos;
  current_val = next_val;
  current_type = next_type;
  return true;
}

template <size_t max_depth>
ParsedJson::BasicIterator<max_depth>::BasicIterator(ParsedJson &pj_)
    : pj(&pj_), depth(0), location(0), tape_length(0) {
  if (!pj->is_valid()) {
    throw InvalidJSON();
  }
  depth_index[0].start_of_scope = location;
  current_val = pj->tape[location++];
  current_type = (current_val >> 56);
  depth_index[0].scope_type = current_type;
  if (current_type == 'r') {
    tape_length = current_val & JSON_VALUE_MASK;
    if (location < tape_length) {
      // If we make it here, then depth_capacity must >=2, but the compiler
      // may not know this.
      current_val = pj->tape[location];
      current_type = (current_val >> 56);
      depth++;
      assert(depth < max_depth);
      depth_index[depth].start_of_scope = location;
      depth_index[depth].scope_type = current_type;
    }
  } else {
    // should never happen
    throw InvalidJSON();
  }
}

template <size_t max_depth>
ParsedJson::BasicIterator<max_depth>::BasicIterator(
    const BasicIterator &o) noexcept
    : pj(o.pj), depth(o.depth), location(o.location),
      tape_length(o.tape_length), current_type(o.current_type),
      current_val(o.current_val) {
  memcpy(depth_index, o.depth_index, (depth + 1) * sizeof(depth_index[0]));
}

template <size_t max_depth>
ParsedJson::BasicIterator<max_depth> &ParsedJson::BasicIterator<max_depth>::
operator=(const BasicIterator &o) noexcept {
  pj = o.pj;
  depth = o.depth;
  location = o.location;
  tape_length = o.tape_length;
  current_type = o.current_type;
  current_val = o.current_val;
  memcpy(depth_index, o.depth_index, (depth + 1) * sizeof(depth_index[0]));
  return *this;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::print(std::ostream &os,
                                                 bool escape_strings) const {
  if (!is_ok()) {
    return false;
  }
  switch (current_type) {
  case '"': // we have a string
    os << '"';
    if (escape_strings) {
      print_with_escapes(get_string(), os, get_string_length());
    } else {
      // was: os << get_string();, but given that we can include null chars, we
      // have to do something crazier:
      std::copy(get_string(), get_string() + get_string_length(),
                std::ostream_iterator<char>(os));
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
    os << static_cast<char>(current_type);
    break;
  default:
    return false;
  }
  return true;
}

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::move_to(const char *pointer,
                                                   uint32_t length) {
  char *new_pointer = nullptr;
  if (pointer[0] == '#') {
    // Converting fragment representation to string representation
    new_pointer = new char[length];
    uint32_t new_length = 0;
    for (uint32_t i = 1; i < length; i++) {
      if (pointer[i] == '%' && pointer[i + 1] == 'x') {
        try {
          int fragment =
              std::stoi(std::string(&pointer[i + 2], 2), nullptr, 16);
          if (fragment == '\\' || fragment == '"' || (fragment <= 0x1F)) {
            // escaping the character
            new_pointer[new_length] = '\\';
            new_length++;
          }
          new_pointer[new_length] = fragment;
          i += 3;
        } catch (std::invalid_argument &) {
          delete[] new_pointer;
          return false; // the fragment is invalid
        }
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

template <size_t max_depth>
bool ParsedJson::BasicIterator<max_depth>::relative_move_to(const char *pointer,
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
    if (move_to_key(key_or_index.c_str(), key_or_index.length())) {
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
          npos = (current_val & JSON_VALUE_MASK);
        } else {
          npos =
              location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
        }
        location = npos;
        current_val = pj->tape[npos];
        current_type = (current_val >> 56);
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
} // namespace simdjson
#endif
