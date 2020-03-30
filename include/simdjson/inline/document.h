#ifndef SIMDJSON_INLINE_DOCUMENT_H
#define SIMDJSON_INLINE_DOCUMENT_H

// Inline implementations go in here.

#include "simdjson/document.h"
#include "simdjson/document_stream.h"
#include "simdjson/implementation.h"
#include "simdjson/padded_string.h"
#include "simdjson/internal/jsonformatutils.h"
#include <iostream>
#include <climits>
#include <cctype>

namespace simdjson {

//
// simdjson_result<dom::element> inline implementation
//
really_inline simdjson_result<dom::element>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::element>() {}
really_inline simdjson_result<dom::element>::simdjson_result(dom::element &&value) noexcept
    : internal::simdjson_result_base<dom::element>(std::forward<dom::element>(value)) {}
really_inline simdjson_result<dom::element>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::element>(error) {}
inline simdjson_result<bool> simdjson_result<dom::element>::is_null() const noexcept {
  if (error()) { return error(); }
  return first.is_null();
}
template<typename T>
inline simdjson_result<bool> simdjson_result<dom::element>::is() const noexcept {
  if (error()) { return error(); }
  return first.is<T>();
}
template<typename T>
inline simdjson_result<T> simdjson_result<dom::element>::get() const noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}

inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at(const std::string_view &json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key_case_insensitive(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element>::operator bool() const noexcept(false) {
  return get<bool>();
}
inline simdjson_result<dom::element>::operator const char *() const noexcept(false) {
  return get<const char *>();
}
inline simdjson_result<dom::element>::operator std::string_view() const noexcept(false) {
  return get<std::string_view>();
}
inline simdjson_result<dom::element>::operator uint64_t() const noexcept(false) {
  return get<uint64_t>();
}
inline simdjson_result<dom::element>::operator int64_t() const noexcept(false) {
  return get<int64_t>();
}
inline simdjson_result<dom::element>::operator double() const noexcept(false) {
  return get<double>();
}
inline simdjson_result<dom::element>::operator dom::array() const noexcept(false) {
  return get<dom::array>();
}
inline simdjson_result<dom::element>::operator dom::object() const noexcept(false) {
  return get<dom::object>();
}

inline dom::array::iterator simdjson_result<dom::element>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::array::iterator simdjson_result<dom::element>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif

//
// simdjson_result<dom::array> inline implementation
//
really_inline simdjson_result<dom::array>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::array>() {}
really_inline simdjson_result<dom::array>::simdjson_result(dom::array value) noexcept
    : internal::simdjson_result_base<dom::array>(std::forward<dom::array>(value)) {}
really_inline simdjson_result<dom::array>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::array>(error) {}

#if SIMDJSON_EXCEPTIONS

inline dom::array::iterator simdjson_result<dom::array>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::array::iterator simdjson_result<dom::array>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element> simdjson_result<dom::array>::at(const std::string_view &json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::array>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}

//
// simdjson_result<dom::object> inline implementation
//
really_inline simdjson_result<dom::object>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::object>() {}
really_inline simdjson_result<dom::object>::simdjson_result(dom::object value) noexcept
    : internal::simdjson_result_base<dom::object>(std::forward<dom::object>(value)) {}
really_inline simdjson_result<dom::object>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::object>(error) {}

inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at(const std::string_view &json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key_case_insensitive(const std::string_view &key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

inline dom::object::iterator simdjson_result<dom::object>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::object::iterator simdjson_result<dom::object>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

namespace simdjson::dom {

//
// document inline implementation
//
inline element document::root() const noexcept {
  return element(this, 1);
}

//#define REPORT_ERROR(CODE, MESSAGE) ((std::cerr << MESSAGE << std::endl), CODE)
#define REPORT_ERROR(CODE, MESSAGE) (CODE)
#define RETURN_ERROR(CODE, MESSAGE) return REPORT_ERROR((CODE), (MESSAGE));

WARN_UNUSED
inline error_code document::allocate(size_t capacity) noexcept {
  if (capacity == 0) {
    string_buf.reset();
    tape.reset();
    return SUCCESS;
  }

  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of at least len + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6" 
  //where len + 1 tape elements are
  // generated, see issue https://github.com/lemire/simdjson/issues/345
  size_t tape_capacity = ROUNDUP_N(capacity + 2, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t string_capacity = ROUNDUP_N(5 * capacity / 3 + 32, 64);
  string_buf.reset( new (std::nothrow) uint8_t[string_capacity]);
  tape.reset(new (std::nothrow) uint64_t[tape_capacity]);
  return string_buf && tape ? SUCCESS : MEMALLOC;
}

inline bool document::dump_raw_tape(std::ostream &os) const noexcept {
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = (tape_val >> 56);
  os << tape_idx << " : " << type;
  tape_idx++;
  size_t how_many = 0;
  if (type == 'r') {
    how_many = tape_val & internal::JSON_VALUE_MASK;
  } else {
    // Error: no starting root node?
    return false;
  }
  os << "\t// pointing to " << how_many << " (right after last node)\n";
  uint64_t payload;
  for (; tape_idx < how_many; tape_idx++) {
    os << tape_idx << " : ";
    tape_val = tape[tape_idx];
    payload = tape_val & internal::JSON_VALUE_MASK;
    type = (tape_val >> 56);
    switch (type) {
    case '"': // we have a string
      os << "string \"";
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      os << internal::escape_json_string(std::string_view(
        (const char *)(string_buf.get() + payload + sizeof(uint32_t)),
        string_length
      ));
      os << '"';
      os << '\n';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
      break;
    case 'u': // we have a long uint
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "unsigned integer " << tape[++tape_idx] << "\n";
      break;
    case 'd': // we have a double
      os << "float ";
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
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
      os << "{\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case 'r': // we start and end with the root node
      // should we be hitting the root node?
      return false;
    default:
      return false;
    }
  }
  tape_val = tape[tape_idx];
  payload = tape_val & internal::JSON_VALUE_MASK;
  type = (tape_val >> 56);
  os << tape_idx << " : " << type << "\t// pointing to " << payload
     << " (start root)\n";
  return true;
}

//
// parser inline implementation
//
really_inline parser::parser(size_t max_capacity) noexcept
  : _max_capacity{max_capacity}, loaded_bytes(nullptr, &aligned_free_char) {}
inline bool parser::is_valid() const noexcept { return valid; }
inline int parser::get_error_code() const noexcept { return error; }
inline std::string parser::get_error_message() const noexcept { return error_message(error); }
inline bool parser::print_json(std::ostream &os) const noexcept {
  if (!valid) { return false; }
  os << doc.root();
  return true;
}
inline bool parser::dump_raw_tape(std::ostream &os) const noexcept {
  return valid ? doc.dump_raw_tape(os) : false;
}

inline simdjson_result<size_t> parser::read_file(const std::string &path) noexcept {
  // Open the file
  std::FILE *fp = std::fopen(path.c_str(), "rb");
  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
  long len = std::ftell(fp);
  if((len < 0) || (len == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }

  // Make sure we have enough capacity to load the file
  if (_loaded_bytes_capacity < size_t(len)) {
    loaded_bytes.reset( internal::allocate_padded_buffer(len) );
    if (!loaded_bytes) {
      std::fclose(fp);
      return MEMALLOC;
    }
    _loaded_bytes_capacity = len;
  }

  // Read the string
  std::rewind(fp);
  size_t bytes_read = std::fread(loaded_bytes.get(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != size_t(len)) {
    return IO_ERROR;
  }

  return bytes_read;
}

inline simdjson_result<element> parser::load(const std::string &path) noexcept {
  auto [len, code] = read_file(path);
  if (code) { return code; }

  return parse(loaded_bytes.get(), len, false);
}

inline document_stream parser::load_many(const std::string &path, size_t batch_size) noexcept {
  auto [len, code] = read_file(path);
  return document_stream(*this, (const uint8_t*)loaded_bytes.get(), len, batch_size, code);
}

inline simdjson_result<element> parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  error_code code = ensure_capacity(len);
  if (code) { return code; }

  if (realloc_if_needed) {
    const uint8_t *tmp_buf = buf;
    buf = (uint8_t *)internal::allocate_padded_buffer(len);
    if (buf == nullptr)
      return MEMALLOC;
    memcpy((void *)buf, tmp_buf, len);
  }

  code = simdjson::active_implementation->parse(buf, len, *this);
  if (realloc_if_needed) {
    aligned_free((void *)buf); // must free before we exit
  }
  if (code) { return code; }

  // We're indicating validity via the simdjson_result<element>, so set the parse state back to invalid
  valid = false;
  error = UNINITIALIZED;
  return doc.root();
}
really_inline simdjson_result<element> parser::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline simdjson_result<element> parser::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline simdjson_result<element> parser::parse(const padded_string &s) noexcept {
  return parse(s.data(), s.length(), false);
}

inline document_stream parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  return document_stream(*this, buf, len, batch_size);
}
inline document_stream parser::parse_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return parse_many((const uint8_t *)buf, len, batch_size);
}
inline document_stream parser::parse_many(const std::string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline document_stream parser::parse_many(const padded_string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}

really_inline size_t parser::capacity() const noexcept {
  return _capacity;
}
really_inline size_t parser::max_capacity() const noexcept {
  return _max_capacity;
}
really_inline size_t parser::max_depth() const noexcept {
  return _max_depth;
}

WARN_UNUSED
inline error_code parser::allocate(size_t capacity, size_t max_depth) noexcept {
  //
  // If capacity has changed, reallocate capacity-based buffers
  //
  if (_capacity != capacity) {
    // Set capacity to 0 until we finish, in case there's an error
    _capacity = 0;

    //
    // Reallocate the document
    //
    error_code err = doc.allocate(capacity);
    if (err) { return err; }

    //
    // Don't allocate 0 bytes, just return.
    //
    if (capacity == 0) {
      structural_indexes.reset();
      return SUCCESS;
    }

    //
    // Initialize stage 1 output
    //
    uint32_t max_structures = ROUNDUP_N(capacity, 64) + 2 + 7;
    structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] ); // TODO realloc
    if (!structural_indexes) {
      return MEMALLOC;
    }

    _capacity = capacity;

  //
  // If capacity hasn't changed, but the document was taken, allocate a new document.
  //
  } else if (!doc.tape) {
    error_code err = doc.allocate(capacity);
    if (err) { return err; }
  }

  //
  // If max_depth has changed, reallocate those buffers
  //
  if (max_depth != _max_depth) {
    _max_depth = 0;

    if (max_depth == 0) {
      ret_address.reset();
      containing_scope_offset.reset();
      return SUCCESS;
    }

    //
    // Initialize stage 2 state
    //
    containing_scope_offset.reset(new (std::nothrow) uint32_t[max_depth]); // TODO realloc
  #ifdef SIMDJSON_USE_COMPUTED_GOTO
    ret_address.reset(new (std::nothrow) void *[max_depth]);
  #else
    ret_address.reset(new (std::nothrow) char[max_depth]);
  #endif

    if (!ret_address || !containing_scope_offset) {
      // Could not allocate memory
      return MEMALLOC;
    }

    _max_depth = max_depth;
  }
  return SUCCESS;
}

WARN_UNUSED
inline bool parser::allocate_capacity(size_t capacity, size_t max_depth) noexcept {
  return !allocate(capacity, max_depth);
}

really_inline void parser::set_max_capacity(size_t max_capacity) noexcept {
  _max_capacity = max_capacity;
}

inline error_code parser::ensure_capacity(size_t desired_capacity) noexcept {
  // If we don't have enough capacity, (try to) automatically bump it.
  // If the document was taken, reallocate that too.
  // Both in one if statement to minimize unlikely branching.
  if (unlikely(desired_capacity > capacity() || !doc.tape)) {
    if (desired_capacity > max_capacity()) {
      return error = CAPACITY;
    }
    return allocate(desired_capacity, _max_depth > 0 ? _max_depth : DEFAULT_MAX_DEPTH);
  }

  return SUCCESS;
}

//
// array inline implementation
//
really_inline array::array() noexcept : internal::tape_ref() {}
really_inline array::array(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) {}
inline array::iterator array::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline array::iterator array::end() const noexcept {
  return iterator(doc, after_element() - 1);
}

inline simdjson_result<element> array::at(const std::string_view &json_pointer) const noexcept {
  // - means "the append position" or "the element after the end of the array"
  // We don't support this, because we're returning a real element, not a position.
  if (json_pointer == "-") { return INDEX_OUT_OF_BOUNDS; }

  // Read the array index
  size_t array_index = 0;
  size_t i;
  for (i = 0; i < json_pointer.length() && json_pointer[i] != '/'; i++) {
    uint8_t digit = uint8_t(json_pointer[i]) - '0';
    // Check for non-digit in array index. If it's there, we're trying to get a field in an object
    if (digit > 9) { return INCORRECT_TYPE; }
    array_index = array_index*10 + digit;
  }

  // 0 followed by other digits is invalid
  if (i > 1 && json_pointer[0] == '0') { RETURN_ERROR(INVALID_JSON_POINTER, "JSON pointer array index has other characters after 0"); }

  // Empty string is invalid; so is a "/" with no digits before it
  if (i == 0) { RETURN_ERROR(INVALID_JSON_POINTER, "Empty string in JSON pointer array index"); }

  // Get the child
  auto child = array(doc, json_index).at(array_index);
  // If there is a /, we're not done yet, call recursively.
  if (i < json_pointer.length()) {
    child = child.at(json_pointer.substr(i+1));
  }
  return child;
}
inline simdjson_result<element> array::at(size_t index) const noexcept {
  size_t i=0;
  for (auto element : *this) {
    if (i == index) { return element; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

//
// array::iterator inline implementation
//
really_inline array::iterator::iterator(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }
inline element array::iterator::operator*() const noexcept {
  return element(doc, json_index);
}
inline bool array::iterator::operator!=(const array::iterator& other) const noexcept {
  return json_index != other.json_index;
}
inline void array::iterator::operator++() noexcept {
  json_index = after_element();
}

//
// object inline implementation
//
really_inline object::object() noexcept : internal::tape_ref() {}
really_inline object::object(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { };
inline object::iterator object::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline object::iterator object::end() const noexcept {
  return iterator(doc, after_element() - 1);
}

inline simdjson_result<element> object::operator[](const std::string_view &key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::operator[](const char *key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::at(const std::string_view &json_pointer) const noexcept {
  size_t slash = json_pointer.find('/');
  std::string_view key = json_pointer.substr(0, slash);

  // Grab the child with the given key
  simdjson_result<element> child;

  // If there is an escape character in the key, unescape it and then get the child.
  size_t escape = key.find('~');
  if (escape != std::string_view::npos) {
    // Unescape the key
    std::string unescaped(key);
    do {
      switch (unescaped[escape+1]) {
        case '0':
          unescaped.replace(escape, 2, "~");
          break;
        case '1':
          unescaped.replace(escape, 2, "/");
          break;
        default:
          RETURN_ERROR(INVALID_JSON_POINTER, "Unexpected ~ escape character in JSON pointer");
      }
      escape = unescaped.find('~', escape+1);
    } while (escape != std::string::npos);
    child = at_key(unescaped);
  } else {
    child = at_key(key);
  }

  // If there is a /, we have to recurse and look up more of the path
  if (slash != std::string_view::npos) {
    child = child.at(json_pointer.substr(slash+1));
  }

  return child;
}
inline simdjson_result<element> object::at_key(const std::string_view &key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (key == field.key()) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}
// In case you wonder why we need this, please see
// https://github.com/simdjson/simdjson/issues/323
// People do seek keys in a case-insensitive manner.
inline simdjson_result<element> object::at_key_case_insensitive(const std::string_view &key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    auto field_key = field.key();
    if (key.length() == field_key.length()) {
      bool equal = true;
      for (size_t i=0; i<field_key.length(); i++) {
        equal = equal && std::tolower(key[i]) != std::tolower(field_key[i]);
      }
      if (equal) { return field.value(); }
    }
  }
  return NO_SUCH_FIELD;
}

//
// object::iterator inline implementation
//
really_inline object::iterator::iterator(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }
inline const key_value_pair object::iterator::operator*() const noexcept {
  return key_value_pair(key(), value());
}
inline bool object::iterator::operator!=(const object::iterator& other) const noexcept {
  return json_index != other.json_index;
}
inline void object::iterator::operator++() noexcept {
  json_index++;
  json_index = after_element();
}
inline std::string_view object::iterator::key() const noexcept {
  size_t string_buf_index = tape_value();
  uint32_t len;
  memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return std::string_view(
    reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]),
    len
  );
}
inline const char* object::iterator::key_c_str() const noexcept {
  return reinterpret_cast<const char *>(&doc->string_buf[tape_value() + sizeof(uint32_t)]);
}
inline element object::iterator::value() const noexcept {
  return element(doc, json_index + 1);
}

//
// key_value_pair inline implementation
//
inline key_value_pair::key_value_pair(const std::string_view &_key, element _value) noexcept :
  key(_key), value(_value) {}

//
// element inline implementation
//
really_inline element::element() noexcept : internal::tape_ref() {}
really_inline element::element(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }

really_inline bool element::is_null() const noexcept {
  return type() == internal::tape_type::NULL_VALUE;
}

template<>
inline simdjson_result<bool> element::get<bool>() const noexcept {
  switch (type()) {
    case internal::tape_type::TRUE_VALUE:
      return true;
    case internal::tape_type::FALSE_VALUE:
      return false;
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<const char *> element::get<const char *>() const noexcept {
  switch (type()) {
    case internal::tape_type::STRING: {
      size_t string_buf_index = tape_value();
      return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
    }
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<std::string_view> element::get<std::string_view>() const noexcept {
  switch (type()) {
    case internal::tape_type::STRING:
      return get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<uint64_t> element::get<uint64_t>() const noexcept {
  switch (type()) {
    case internal::tape_type::UINT64:
      return next_tape_value<uint64_t>();
    case internal::tape_type::INT64: {
      int64_t result = next_tape_value<int64_t>();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<uint64_t>(result);
    }
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<int64_t> element::get<int64_t>() const noexcept {
  switch (type()) {
    case internal::tape_type::UINT64: {
      uint64_t result = next_tape_value<uint64_t>();
      // Wrapping max in parens to handle Windows issue: https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
      if (result > (std::numeric_limits<uint64_t>::max)()) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<int64_t>(result);
    }
    case internal::tape_type::INT64:
      return next_tape_value<int64_t>();
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<double> element::get<double>() const noexcept {
  switch (type()) {
    case internal::tape_type::UINT64:
      return next_tape_value<uint64_t>();
    case internal::tape_type::INT64: {
      return next_tape_value<int64_t>();
      int64_t result = tape_value();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return double(result);
    }
    case internal::tape_type::DOUBLE:
      return next_tape_value<double>();
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<array> element::get<array>() const noexcept {
  switch (type()) {
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
template<>
inline simdjson_result<object> element::get<object>() const noexcept {
  switch (type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}

template<typename T>
really_inline bool element::is() const noexcept {
  auto result = get<T>();
  return !result.error();
}

#if SIMDJSON_EXCEPTIONS

inline element::operator bool() const noexcept(false) { return get<bool>(); }
inline element::operator const char*() const noexcept(false) { return get<const char *>(); }
inline element::operator std::string_view() const noexcept(false) { return get<std::string_view>(); }
inline element::operator uint64_t() const noexcept(false) { return get<uint64_t>(); }
inline element::operator int64_t() const noexcept(false) { return get<int64_t>(); }
inline element::operator double() const noexcept(false) { return get<double>(); }
inline element::operator array() const noexcept(false) { return get<array>(); }
inline element::operator object() const noexcept(false) { return get<object>(); }

inline dom::array::iterator dom::element::begin() const noexcept(false) {
  return get<array>().begin();
}
inline dom::array::iterator dom::element::end() const noexcept(false) {
  return get<array>().end();
}

#endif

inline simdjson_result<element> element::operator[](const std::string_view &key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::operator[](const char *key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::at(const std::string_view &json_pointer) const noexcept {
  switch (type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index).at(json_pointer);
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index).at(json_pointer);
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<element> element::at(size_t index) const noexcept {
  return get<array>().at(index);
}
inline simdjson_result<element> element::at_key(const std::string_view &key) const noexcept {
  return get<object>().at_key(key);
}
inline simdjson_result<element> element::at_key_case_insensitive(const std::string_view &key) const noexcept {
  return get<object>().at_key_case_insensitive(key);
}

inline bool element::dump_raw_tape(std::ostream &out) const noexcept {
  return doc->dump_raw_tape(out);
}

} // namespace simdjson::dom

namespace simdjson {

//
// minify inline implementation
//

template<>
inline std::ostream& minify<dom::element>::print(std::ostream& out) {
  using tape_type=internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value);
  do {
    // print commas after each value
    if (after_value) {
      out << ",";
    }
    // If we are in an object, print the next key and :, and skip to the next value.
    if (is_object[depth]) {
      out << '"' << internal::escape_json_string(iter.get_string_view()) << "\":";
      iter.json_index++;
    }
    switch (iter.type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::array>(dom::array(iter.doc, iter.json_index));
        iter.json_index = iter.tape_value() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      out << '[';
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.type() == tape_type::END_ARRAY) {
        out << ']';
        depth--;
        break;
      }

      is_object[depth] = false;
      after_value = false;
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::object>(dom::object(iter.doc, iter.json_index));
        iter.json_index = iter.tape_value() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      out << '{';
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.type() == tape_type::END_OBJECT) {
        out << '}';
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      out << '"' << internal::escape_json_string(iter.get_string_view()) << '"';
      break;
    case tape_type::INT64:
      out << iter.next_tape_value<int64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::UINT64:
      out << iter.next_tape_value<uint64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::DOUBLE:
      out << iter.next_tape_value<double>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::TRUE_VALUE:
      out << "true";
      break;
    case tape_type::FALSE_VALUE:
      out << "false";
      break;
    case tape_type::NULL_VALUE:
      out << "null";
      break;

    // These are impossible
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
    case tape_type::ROOT:
      abort();
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.type() == tape_type::END_ARRAY || iter.type() == tape_type::END_OBJECT)) {
      out << char(iter.type());
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);

  return out;
}
template<>
inline std::ostream& minify<dom::object>::print(std::ostream& out) {
  out << '{';
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    out << minify<dom::key_value_pair>(*pair);
    for (++pair; pair != end; ++pair) {
      out << "," << minify<dom::key_value_pair>(*pair);
    }
  }
  return out << '}';
}
template<>
inline std::ostream& minify<dom::array>::print(std::ostream& out) {
  out << '[';
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    out << minify<dom::element>(*iter);
    for (++iter; iter != end; ++iter) {
      out << "," << minify<dom::element>(*iter);
    }
  }
  return out << ']';
}
template<>
inline std::ostream& minify<dom::key_value_pair>::print(std::ostream& out) {
  return out << '"' << internal::escape_json_string(value.key) << "\":" << value.value;
}

#if SIMDJSON_EXCEPTIONS

template<>
inline std::ostream& minify<simdjson_result<dom::element>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::element>(value.first);
}
template<>
inline std::ostream& minify<simdjson_result<dom::array>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::array>(value.first);
}
template<>
inline std::ostream& minify<simdjson_result<dom::object>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::object>(value.first);
}

#endif

} // namespace simdjson

namespace simdjson::internal {

//
// tape_ref inline implementation
//
really_inline tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
really_inline tape_ref::tape_ref(const document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}

inline size_t tape_ref::after_element() const noexcept {
  switch (type()) {
    case tape_type::START_ARRAY:
    case tape_type::START_OBJECT:
      return tape_value();
    case tape_type::UINT64:
    case tape_type::INT64:
    case tape_type::DOUBLE:
      return json_index + 2;
    default:
      return json_index + 1;
  }
}
really_inline tape_type tape_ref::type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
really_inline uint64_t internal::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
template<typename T>
really_inline T tape_ref::next_tape_value() const noexcept {
  static_assert(sizeof(T) == sizeof(uint64_t));
  return *reinterpret_cast<const T*>(&doc->tape[json_index + 1]);
}
inline std::string_view internal::tape_ref::get_string_view() const noexcept {
  size_t string_buf_index = tape_value();
  uint32_t len;
  memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return std::string_view(
    reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]),
    len
  );
}


} // namespace simdjson::internal

#endif // SIMDJSON_INLINE_DOCUMENT_H
