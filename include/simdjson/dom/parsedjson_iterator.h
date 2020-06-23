// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_PARSEDJSON_ITERATOR_H
#define SIMDJSON_DOM_PARSEDJSON_ITERATOR_H

#include <cstring>
#include <string>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

#include "simdjson/dom/document.h"
#include "simdjson/dom/parsedjson.h"
#include "simdjson/internal/jsonformatutils.h"

namespace simdjson {

/** @private **/
class [[deprecated("Use the new DOM navigation API instead (see doc/basics.md)")]] dom::parser::Iterator {
public:
  inline Iterator(const dom::parser &parser) noexcept(false);
  inline Iterator(const Iterator &o) noexcept;
  inline ~Iterator() noexcept;

  inline Iterator& operator=(const Iterator&) = delete;

  inline bool is_ok() const;

  // useful for debugging purposes
  inline size_t get_tape_location() const;

  // useful for debugging purposes
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
      return static_cast<int64_t>(doc.tape[location + 1]);
  }

  // get the value as uint64; valid only if  if get_type is "u"
  inline uint64_t get_unsigned_integer() const {
      if (location + 1 >= tape_length) {
      return 0; // default value in case of error
      }
      return doc.tape[location + 1];
  }

  // get the string value at this node (NULL ended); valid only if get_type is "
  // note that tabs, and line endings are escaped in the returned value (see
  // print_with_escapes) return value is valid UTF-8, it may contain NULL chars
  // within the string: get_string_length determines the true string length.
  inline const char *get_string() const {
      return reinterpret_cast<const char *>(
          doc.string_buf.get() + (current_val & internal::JSON_VALUE_MASK) + sizeof(uint32_t));
  }

  // return the length of the string in bytes
  inline uint32_t get_string_length() const {
      uint32_t answer;
      memcpy(&answer,
          reinterpret_cast<const char *>(doc.string_buf.get() +
                                          (current_val & internal::JSON_VALUE_MASK)),
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
      memcpy(&answer, &doc.tape[location + 1], sizeof(answer));
      return answer;
  }

  inline bool is_object_or_array() const { return is_object() || is_array(); }

  inline bool is_object() const { return get_type() == '{'; }

  inline bool is_array() const { return get_type() == '['; }

  inline bool is_string() const { return get_type() == '"'; }

  // Returns true if the current type of the node is an signed integer.
  // You can get its value with `get_integer()`.
  inline bool is_integer() const { return get_type() == 'l'; }

  // Returns true if the current type of the node is an unsigned integer.
  // You can get its value with `get_unsigned_integer()`.
  //
  // NOTE:
  // Only a large value, which is out of range of a 64-bit signed integer, is
  // represented internally as an unsigned node. On the other hand, a typical
  // positive integer, such as 1, 42, or 1000000, is as a signed node.
  // Be aware this function returns false for a signed node.
  inline bool is_unsigned_integer() const { return get_type() == 'u'; }
  // Returns true if the current type of the node is a double floating-point number.
  inline bool is_double() const { return get_type() == 'd'; }
  // Returns true if the current type of the node is a number (integer or floating-point).
  inline bool is_number() const {
      return is_integer() || is_unsigned_integer() || is_double();
  }
  // Returns true if the current type of the node is a bool with true value.
  inline bool is_true() const { return get_type() == 't'; }
  // Returns true if the current type of the node is a bool with false value.
  inline bool is_false() const { return get_type() == 'f'; }
  // Returns true if the current type of the node is null.
  inline bool is_null() const { return get_type() == 'n'; }
  // Returns true if the type byte represents an object of an array
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

  // Moves the iterator to the value corresponding to the json pointer.
  // Always search from the root of the document.
  // if successful, we are left pointing at the value,
  // if not, we are still pointing the same value we were pointing before the
  // call. The json pointer follows the rfc6901 standard's syntax:
  // https://tools.ietf.org/html/rfc6901 However, the standard says "If a
  // referenced member name is not unique in an object, the member that is
  // referenced is undefined, and evaluation fails". Here we just return the
  // first corresponding value. The length parameter is the length of the
  // jsonpointer string ('pointer').
  inline bool move_to(const char *pointer, uint32_t length);

  // Moves the iterator to the value corresponding to the json pointer.
  // Always search from the root of the document.
  // if successful, we are left pointing at the value,
  // if not, we are still pointing the same value we were pointing before the
  // call. The json pointer implementation follows the rfc6901 standard's
  // syntax: https://tools.ietf.org/html/rfc6901 However, the standard says
  // "If a referenced member name is not unique in an object, the member that
  // is referenced is undefined, and evaluation fails". Here we just return
  // the first corresponding value.
  inline bool move_to(const std::string &pointer) {
      return move_to(pointer.c_str(), uint32_t(pointer.length()));
  }

  private:
  // Almost the same as move_to(), except it searches from the current
  // position. The pointer's syntax is identical, though that case is not
  // handled by the rfc6901 standard. The '/' is still required at the
  // beginning. However, contrary to move_to(), the URI Fragment Identifier
  // Representation is not supported here. Also, in case of failure, we are
  // left pointing at the closest value it could reach. For these reasons it
  // is private. It exists because it is used by move_to().
  inline bool relative_move_to(const char *pointer, uint32_t length);

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



  // print the node we are currently pointing at
  inline bool print(std::ostream &os, bool escape_strings = true) const;

  private:
  const document &doc;
  size_t max_depth{};
  size_t depth{};
  size_t location{}; // our current location on a tape
  size_t tape_length{};
  uint8_t current_type{};
  uint64_t current_val{};
  typedef struct {
      size_t start_of_scope;
      uint8_t scope_type;
  } scopeindex_t;

  scopeindex_t *depth_index{};
};

} // namespace simdjson

#endif // SIMDJSON_DOM_PARSEDJSON_ITERATOR_H
