#ifndef SIMDJSON_INLINE_DOCUMENT_H
#define SIMDJSON_INLINE_DOCUMENT_H

// Inline implementations go in here.

#include "simdjson/document.h"
#include "simdjson/document_stream.h"
#include "simdjson/implementation.h"
#include "simdjson/padded_string.h"
#include "simdjson/internal/jsonformatutils.h"
#include <iostream>

namespace simdjson {

//
// document::element_result<T> inline implementation
//
template<typename T>
inline document::element_result<T>::element_result(T _value) noexcept : value(_value), error{SUCCESS} {}
template<typename T>
inline document::element_result<T>::element_result(error_code _error) noexcept : value(), error{_error} {}
template<>
inline document::element_result<std::string_view>::operator std::string_view() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
template<>
inline document::element_result<const char *>::operator const char *() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
template<>
inline document::element_result<bool>::operator bool() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
template<>
inline document::element_result<uint64_t>::operator uint64_t() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
template<>
inline document::element_result<int64_t>::operator int64_t() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
template<>
inline document::element_result<double>::operator double() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}

//
// document::element_result<document::array> inline implementation
//
inline document::element_result<document::array>::element_result(document::array _value) noexcept : value(_value), error{SUCCESS} {}
inline document::element_result<document::array>::element_result(error_code _error) noexcept : value(), error{_error} {}
inline document::element_result<document::array>::operator document::array() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
inline document::array::iterator document::element_result<document::array>::begin() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value.begin();
}
inline document::array::iterator document::element_result<document::array>::end() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value.end();
}

//
// document::element_result<document::object> inline implementation
//
inline document::element_result<document::object>::element_result(document::object _value) noexcept : value(_value), error{SUCCESS} {}
inline document::element_result<document::object>::element_result(error_code _error) noexcept : value(), error{_error} {}
inline document::element_result<document::object>::operator document::object() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
inline document::element_result<document::element> document::element_result<document::object>::operator[](const std::string_view &key) const noexcept {
  if (error) { return error; }
  return value[key];
}
inline document::element_result<document::element> document::element_result<document::object>::operator[](const char *key) const noexcept {
  if (error) { return error; }
  return value[key];
}
inline document::object::iterator document::element_result<document::object>::begin() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value.begin();
}
inline document::object::iterator document::element_result<document::object>::end() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value.end();
}

//
// document::element_result<document::element> inline implementation
//
inline document::element_result<document::element>::element_result(document::element _value) noexcept : value(_value), error{SUCCESS} {}
inline document::element_result<document::element>::element_result(error_code _error) noexcept : value(), error{_error} {}
inline document::element_result<bool> document::element_result<document::element>::is_null() const noexcept {
  if (error) { return error; }
  return value.is_null();
}
inline document::element_result<bool> document::element_result<document::element>::as_bool() const noexcept {
  if (error) { return error; }
  return value.as_bool();
}
inline document::element_result<const char*> document::element_result<document::element>::as_c_str() const noexcept {
  if (error) { return error; }
  return value.as_c_str();
}
inline document::element_result<std::string_view> document::element_result<document::element>::as_string() const noexcept {
  if (error) { return error; }
  return value.as_string();
}
inline document::element_result<uint64_t> document::element_result<document::element>::as_uint64_t() const noexcept {
  if (error) { return error; }
  return value.as_uint64_t();
}
inline document::element_result<int64_t> document::element_result<document::element>::as_int64_t() const noexcept {
  if (error) { return error; }
  return value.as_int64_t();
}
inline document::element_result<double> document::element_result<document::element>::as_double() const noexcept {
  if (error) { return error; }
  return value.as_double();
}
inline document::element_result<document::array> document::element_result<document::element>::as_array() const noexcept {
  if (error) { return error; }
  return value.as_array();
}
inline document::element_result<document::object> document::element_result<document::element>::as_object() const noexcept {
  if (error) { return error; }
  return value.as_object();
}

inline document::element_result<document::element>::operator document::element() const noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return value;
}
inline document::element_result<document::element>::operator bool() const noexcept(false) {
  return as_bool();
}
inline document::element_result<document::element>::operator const char *() const noexcept(false) {
  return as_c_str();
}
inline document::element_result<document::element>::operator std::string_view() const noexcept(false) {
  return as_string();
}
inline document::element_result<document::element>::operator uint64_t() const noexcept(false) {
  return as_uint64_t();
}
inline document::element_result<document::element>::operator int64_t() const noexcept(false) {
  return as_int64_t();
}
inline document::element_result<document::element>::operator double() const noexcept(false) {
  return as_double();
}
inline document::element_result<document::element>::operator document::array() const noexcept(false) {
  return as_array();
}
inline document::element_result<document::element>::operator document::object() const noexcept(false) {
  return as_object();
}
inline document::element_result<document::element> document::element_result<document::element>::operator[](const std::string_view &key) const noexcept {
  if (error) { return *this; }
  return value[key];
}
inline document::element_result<document::element> document::element_result<document::element>::operator[](const char *key) const noexcept {
  if (error) { return *this; }
  return value[key];
}

//
// document inline implementation
//
inline document::element document::root() const noexcept {
  return document::element(this, 1);
}
inline document::element_result<document::array> document::as_array() const noexcept {
  return root().as_array();
}
inline document::element_result<document::object> document::as_object() const noexcept {
  return root().as_object();
}
inline document::operator document::element() const noexcept {
  return root();
}
inline document::operator document::array() const noexcept(false) {
  return root();
}
inline document::operator document::object() const noexcept(false) {
  return root();
}
inline document::element_result<document::element> document::operator[](const std::string_view &key) const noexcept {
  return root()[key];
}
inline document::element_result<document::element> document::operator[](const char *key) const noexcept {
  return root()[key];
}

inline document::doc_result document::load(const std::string &path) noexcept {
  document::parser parser;
  auto [doc, error] = parser.load(path);
  return document::doc_result((document &&)doc, error);
}

inline document::doc_result document::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  document::parser parser;
  auto [doc, error] = parser.parse(buf, len, realloc_if_needed);
  return document::doc_result((document &&)doc, error);
}
really_inline document::doc_result document::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_result document::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline document::doc_result document::parse(const padded_string &s) noexcept {
  return parse(s.data(), s.length(), false);
}

WARN_UNUSED
inline error_code document::set_capacity(size_t capacity) noexcept {
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
// document::doc_ref_result inline implementation
//
inline document::doc_ref_result::doc_ref_result(document &_doc, error_code _error) noexcept : doc(_doc), error(_error) { }
inline document::doc_ref_result::operator document&() noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return doc;
}
inline document::element_result<document::element> document::doc_ref_result::operator[](const std::string_view &key) const noexcept {
  if (error) { return error; }
  return doc[key];
}
inline document::element_result<document::element> document::doc_ref_result::operator[](const char *key) const noexcept {
  if (error) { return error; }
  return doc[key];
}

//
// document::doc_result inline implementation
//
inline document::doc_result::doc_result(document &&_doc, error_code _error) noexcept : doc(std::move(_doc)), error(_error) { }
inline document::doc_result::doc_result(document &&_doc) noexcept : doc(std::move(_doc)), error(SUCCESS) { }
inline document::doc_result::doc_result(error_code _error) noexcept : doc(), error(_error) { }
inline document::doc_result::operator document() noexcept(false) {
  if (error) { throw simdjson_error(error); }
  return std::move(doc);
}
inline document::element_result<document::element> document::doc_result::operator[](const std::string_view &key) const noexcept {
  if (error) { return error; }
  return doc[key];
}
inline document::element_result<document::element> document::doc_result::operator[](const char *key) const noexcept {
  if (error) { return error; }
  return doc[key];
}

//
// document::parser inline implementation
//
really_inline document::parser::parser(size_t max_capacity, size_t max_depth) noexcept
  : _max_capacity{max_capacity}, _max_depth{max_depth} {

}
inline bool document::parser::is_valid() const noexcept { return valid; }
inline int document::parser::get_error_code() const noexcept { return error; }
inline std::string document::parser::get_error_message() const noexcept { return error_message(int(error)); }
inline bool document::parser::print_json(std::ostream &os) const noexcept {
  if (!is_valid()) { return false; }
  os << minify(doc);
  return true;
}
inline bool document::parser::dump_raw_tape(std::ostream &os) const noexcept {
  return is_valid() ? doc.dump_raw_tape(os) : false;
}
inline const document &document::parser::get_document() const noexcept(false) {
  if (!is_valid()) {
    throw simdjson_error(error);
  }
  return doc;
}

inline document::doc_ref_result document::parser::load(const std::string &path) noexcept {
  auto [json, _error] = padded_string::load(path);
  if (_error) { return doc_ref_result(doc, _error); }
  return parse(json);
}

inline document::stream document::parser::load_many(const std::string &path, size_t batch_size) noexcept {
  auto [json, _error] = padded_string::load(path);
  return stream(*this, reinterpret_cast<const uint8_t*>(json.data()), json.length(), batch_size, _error);
}

inline document::doc_ref_result document::parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  error_code code = ensure_capacity(len);
  if (code) { return document::doc_ref_result(doc, code); }

  if (realloc_if_needed) {
    const uint8_t *tmp_buf = buf;
    buf = (uint8_t *)internal::allocate_padded_buffer(len);
    if (buf == nullptr)
      return document::doc_ref_result(doc, MEMALLOC);
    memcpy((void *)buf, tmp_buf, len);
  }

  code = simdjson::active_implementation->parse(buf, len, *this);

  // We're indicating validity via the doc_ref_result, so set the parse state back to invalid
  valid = false;
  error = UNINITIALIZED;
  if (realloc_if_needed) {
    aligned_free((void *)buf); // must free before we exit
  }
  return document::doc_ref_result(doc, code);
}
really_inline document::doc_ref_result document::parser::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_ref_result document::parser::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline document::doc_ref_result document::parser::parse(const padded_string &s) noexcept {
  return parse(s.data(), s.length(), false);
}

inline document::stream document::parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  return stream(*this, buf, len, batch_size);
}
inline document::stream document::parser::parse_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return parse_many((const uint8_t *)buf, len, batch_size);
}
inline document::stream document::parser::parse_many(const std::string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline document::stream document::parser::parse_many(const padded_string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}

really_inline size_t document::parser::capacity() const noexcept {
  return _capacity;
}
really_inline size_t document::parser::max_capacity() const noexcept {
  return _max_capacity;
}
really_inline size_t document::parser::max_depth() const noexcept {
  return _max_depth;
}

WARN_UNUSED
inline error_code document::parser::set_capacity(size_t capacity) noexcept {
  if (_capacity == capacity) {
    return SUCCESS;
  }

  // Set capacity to 0 until we finish, in case there's an error
  _capacity = 0;

  //
  // Reallocate the document
  //
  error_code err = doc.set_capacity(capacity);
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
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]); // TODO realloc
  if (!structural_indexes) {
    return MEMALLOC;
  }

  _capacity = capacity;
  return SUCCESS;
}

really_inline void document::parser::set_max_capacity(size_t max_capacity) noexcept {
  _max_capacity = max_capacity;
}

WARN_UNUSED inline error_code document::parser::set_max_depth(size_t max_depth) noexcept {
  if (max_depth == _max_depth && ret_address) { return SUCCESS; }

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
  return SUCCESS;
}

WARN_UNUSED inline bool document::parser::allocate_capacity(size_t capacity, size_t max_depth) noexcept {
  return !set_capacity(capacity) && !set_max_depth(max_depth);
}

inline error_code document::parser::ensure_capacity(size_t desired_capacity) noexcept {
  // If we don't have enough capacity, (try to) automatically bump it.
  if (unlikely(desired_capacity > capacity())) {
    if (desired_capacity > max_capacity()) {
      return error = CAPACITY;
    }

    error = set_capacity(desired_capacity);
    if (error) { return error; }
  }

  // Allocate depth-based buffers if they aren't already.
  error = set_max_depth(max_depth());
  if (error) { return error; }

  // If the last doc was taken, we need to allocate a new one
  if (!doc.tape) {
    error = doc.set_capacity(desired_capacity);
    if (error) { return error; }
  }

  return SUCCESS;
}

//
// document::tape_ref inline implementation
//
really_inline document::tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
really_inline document::tape_ref::tape_ref(const document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}

inline size_t document::tape_ref::after_element() const noexcept {
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
really_inline document::tape_type document::tape_ref::type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
really_inline uint64_t document::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
template<typename T>
really_inline T document::tape_ref::next_tape_value() const noexcept {
  static_assert(sizeof(T) == sizeof(uint64_t));
  return *reinterpret_cast<const T*>(&doc->tape[json_index + 1]);
}
inline std::string_view document::tape_ref::get_string_view() const noexcept {
  size_t string_buf_index = tape_value();
  uint32_t len;
  memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return std::string_view(
    reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]),
    len
  );
}

//
// document::array inline implementation
//
really_inline document::array::array() noexcept : tape_ref() {}
really_inline document::array::array(const document *_doc, size_t _json_index) noexcept : tape_ref(_doc, _json_index) {}
inline document::array::iterator document::array::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline document::array::iterator document::array::end() const noexcept {
  return iterator(doc, after_element() - 1);
}


//
// document::array::iterator inline implementation
//
really_inline document::array::iterator::iterator(const document *_doc, size_t _json_index) noexcept : tape_ref(_doc, _json_index) { }
inline document::element document::array::iterator::operator*() const noexcept {
  return element(doc, json_index);
}
inline bool document::array::iterator::operator!=(const document::array::iterator& other) const noexcept {
  return json_index != other.json_index;
}
inline void document::array::iterator::operator++() noexcept {
  json_index = after_element();
}

//
// document::object inline implementation
//
really_inline document::object::object() noexcept : tape_ref() {}
really_inline document::object::object(const document *_doc, size_t _json_index) noexcept : tape_ref(_doc, _json_index) { };
inline document::object::iterator document::object::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline document::object::iterator document::object::end() const noexcept {
  return iterator(doc, after_element() - 1);
}
inline document::element_result<document::element> document::object::operator[](const std::string_view &key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (key == field.key()) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}
inline document::element_result<document::element> document::object::operator[](const char *key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (!strcmp(key, field.key_c_str())) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}

//
// document::object::iterator inline implementation
//
really_inline document::object::iterator::iterator(const document *_doc, size_t _json_index) noexcept : tape_ref(_doc, _json_index) { }
inline const document::key_value_pair document::object::iterator::operator*() const noexcept {
  return key_value_pair(key(), value());
}
inline bool document::object::iterator::operator!=(const document::object::iterator& other) const noexcept {
  return json_index != other.json_index;
}
inline void document::object::iterator::operator++() noexcept {
  json_index++;
  json_index = after_element();
}
inline std::string_view document::object::iterator::key() const noexcept {
  size_t string_buf_index = tape_value();
  uint32_t len;
  memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return std::string_view(
    reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]),
    len
  );
}
inline const char* document::object::iterator::key_c_str() const noexcept {
  return reinterpret_cast<const char *>(&doc->string_buf[tape_value() + sizeof(uint32_t)]);
}
inline document::element document::object::iterator::value() const noexcept {
  return element(doc, json_index + 1);
}

//
// document::key_value_pair inline implementation
//
inline document::key_value_pair::key_value_pair(std::string_view _key, document::element _value) noexcept :
  key(_key), value(_value) {}

//
// document::element inline implementation
//
really_inline document::element::element() noexcept : tape_ref() {}
really_inline document::element::element(const document *_doc, size_t _json_index) noexcept : tape_ref(_doc, _json_index) { }
really_inline bool document::element::is_null() const noexcept {
  return type() == tape_type::NULL_VALUE;
}
really_inline bool document::element::is_bool() const noexcept {
  return type() == tape_type::TRUE_VALUE || type() == tape_type::FALSE_VALUE;
}
really_inline bool document::element::is_number() const noexcept {
  return type() == tape_type::UINT64 || type() == tape_type::INT64 || type() == tape_type::DOUBLE;
}
really_inline bool document::element::is_integer() const noexcept {
  return type() == tape_type::UINT64 || type() == tape_type::INT64;
}
really_inline bool document::element::is_string() const noexcept {
  return type() == tape_type::STRING;
}
really_inline bool document::element::is_array() const noexcept {
  return type() == tape_type::START_ARRAY;
}
really_inline bool document::element::is_object() const noexcept {
  return type() == tape_type::START_OBJECT;
}
inline document::element::operator bool() const noexcept(false) { return as_bool(); }
inline document::element::operator const char*() const noexcept(false) { return as_c_str(); }
inline document::element::operator std::string_view() const noexcept(false) { return as_string(); }
inline document::element::operator uint64_t() const noexcept(false) { return as_uint64_t(); }
inline document::element::operator int64_t() const noexcept(false) { return as_int64_t(); }
inline document::element::operator double() const noexcept(false) { return as_double(); }
inline document::element::operator document::array() const noexcept(false) { return as_array(); }
inline document::element::operator document::object() const noexcept(false) { return as_object(); }
inline document::element_result<bool> document::element::as_bool() const noexcept {
  switch (type()) {
    case tape_type::TRUE_VALUE:
      return true;
    case tape_type::FALSE_VALUE:
      return false;
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<const char *> document::element::as_c_str() const noexcept {
  switch (type()) {
    case tape_type::STRING: {
      size_t string_buf_index = tape_value();
      return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<std::string_view> document::element::as_string() const noexcept {
  switch (type()) {
    case tape_type::STRING:
      return get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<uint64_t> document::element::as_uint64_t() const noexcept {
  switch (type()) {
    case tape_type::UINT64:
      return next_tape_value<uint64_t>();
    case tape_type::INT64: {
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
inline document::element_result<int64_t> document::element::as_int64_t() const noexcept {
  switch (type()) {
    case tape_type::UINT64: {
      uint64_t result = next_tape_value<uint64_t>();
      // Wrapping max in parens to handle Windows issue: https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
      if (result > (std::numeric_limits<uint64_t>::max)()) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<int64_t>(result);
    }
    case tape_type::INT64:
      return next_tape_value<int64_t>();
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<double> document::element::as_double() const noexcept {
  switch (type()) {
    case tape_type::UINT64:
      return next_tape_value<uint64_t>();
    case tape_type::INT64: {
      return next_tape_value<int64_t>();
      int64_t result = tape_value();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return result;
    }
    case tape_type::DOUBLE:
      return next_tape_value<double>();
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<document::array> document::element::as_array() const noexcept {
  switch (type()) {
    case tape_type::START_ARRAY:
      return array(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<document::object> document::element::as_object() const noexcept {
  switch (type()) {
    case tape_type::START_OBJECT:
      return object(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result<document::element> document::element::operator[](const std::string_view &key) const noexcept {
  auto [obj, error] = as_object();
  if (error) { return error; }
  return obj[key];
}
inline document::element_result<document::element> document::element::operator[](const char *key) const noexcept {
  auto [obj, error] = as_object();
  if (error) { return error; }
  return obj[key];
}

//
// minify inline implementation
//

template<>
inline std::ostream& minify<document>::print(std::ostream& out) {
  return out << minify<document::element>(value.root());
}
template<>
inline std::ostream& minify<document::element>::print(std::ostream& out) {
  using tape_type=document::tape_type;

  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  document::tape_ref iter(value.doc, value.json_index);
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
        out << minify<document::array>(document::array(iter.doc, iter.json_index));
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
        out << minify<document::object>(document::object(iter.doc, iter.json_index));
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
inline std::ostream& minify<document::object>::print(std::ostream& out) {
  out << '{';
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    out << minify<document::key_value_pair>(*pair);
    for (++pair; pair != end; ++pair) {
      out << "," << minify<document::key_value_pair>(*pair);
    }
  }
  return out << '}';
}
template<>
inline std::ostream& minify<document::array>::print(std::ostream& out) {
  out << '[';
  auto element = value.begin();
  auto end = value.end();
  if (element != end) {
    out << minify<document::element>(*element);
    for (++element; element != end; ++element) {
      out << "," << minify<document::element>(*element);
    }
  }
  return out << ']';
}
template<>
inline std::ostream& minify<document::key_value_pair>::print(std::ostream& out) {
  return out << '"' << internal::escape_json_string(value.key) << "\":" << value.value;
}

template<>
inline std::ostream& minify<document::doc_result>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document>(value.doc);
}
template<>
inline std::ostream& minify<document::doc_ref_result>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document>(value.doc);
}
template<>
inline std::ostream& minify<document::element_result<document::element>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::element>(value.value);
}
template<>
inline std::ostream& minify<document::element_result<document::array>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::array>(value.value);
}
template<>
inline std::ostream& minify<document::element_result<document::object>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::object>(value.value);
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_H
