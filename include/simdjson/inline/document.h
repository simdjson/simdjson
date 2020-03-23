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

namespace simdjson {

//
// element_result inline implementation
//
really_inline document::element_result::element_result(element value) noexcept : simdjson_result<element>(value) {}
really_inline document::element_result::element_result(error_code error) noexcept : simdjson_result<element>(error) {}
inline simdjson_result<bool> document::element_result::is_null() const noexcept {
  if (error()) { return error(); }
  return first.is_null();
}
inline simdjson_result<bool> document::element_result::as_bool() const noexcept {
  if (error()) { return error(); }
  return first.as_bool();
}
inline simdjson_result<const char*> document::element_result::as_c_str() const noexcept {
  if (error()) { return error(); }
  return first.as_c_str();
}
inline simdjson_result<std::string_view> document::element_result::as_string() const noexcept {
  if (error()) { return error(); }
  return first.as_string();
}
inline simdjson_result<uint64_t> document::element_result::as_uint64_t() const noexcept {
  if (error()) { return error(); }
  return first.as_uint64_t();
}
inline simdjson_result<int64_t> document::element_result::as_int64_t() const noexcept {
  if (error()) { return error(); }
  return first.as_int64_t();
}
inline simdjson_result<double> document::element_result::as_double() const noexcept {
  if (error()) { return error(); }
  return first.as_double();
}
inline document::array_result document::element_result::as_array() const noexcept {
  if (error()) { return error(); }
  return first.as_array();
}
inline document::object_result document::element_result::as_object() const noexcept {
  if (error()) { return error(); }
  return first.as_object();
}

inline document::element_result document::element_result::operator[](std::string_view key) const noexcept {
  if (error()) { return *this; }
  return first[key];
}
inline document::element_result document::element_result::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::element_result::at(std::string_view key) const noexcept {
  if (error()) { return *this; }
  return first.at(key);
}
inline document::element_result document::element_result::at(size_t index) const noexcept {
  if (error()) { return *this; }
  return first.at(index);
}
inline document::element_result document::element_result::at_key(std::string_view key) const noexcept {
  if (error()) { return *this; }
  return first.at_key(key);
}
inline document::element_result document::element_result::at_key(const char *key) const noexcept {
  if (error()) { return *this; }
  return first.at_key(key);
}

#if SIMDJSON_EXCEPTIONS

inline document::element_result::operator bool() const noexcept(false) {
  return as_bool();
}
inline document::element_result::operator const char *() const noexcept(false) {
  return as_c_str();
}
inline document::element_result::operator std::string_view() const noexcept(false) {
  return as_string();
}
inline document::element_result::operator uint64_t() const noexcept(false) {
  return as_uint64_t();
}
inline document::element_result::operator int64_t() const noexcept(false) {
  return as_int64_t();
}
inline document::element_result::operator double() const noexcept(false) {
  return as_double();
}
inline document::element_result::operator document::array() const noexcept(false) {
  return as_array();
}
inline document::element_result::operator document::object() const noexcept(false) {
  return as_object();
}

#endif

//
// array_result inline implementation
//
really_inline document::array_result::array_result(array value) noexcept : simdjson_result<array>(value) {}
really_inline document::array_result::array_result(error_code error) noexcept : simdjson_result<array>(error) {}

#if SIMDJSON_EXCEPTIONS

inline document::array::iterator document::array_result::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline document::array::iterator document::array_result::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

inline document::element_result document::array_result::operator[](std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline document::element_result document::array_result::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::array_result::at(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline document::element_result document::array_result::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}

//
// object_result inline implementation
//
really_inline document::object_result::object_result(object value) noexcept : simdjson_result<object>(value) {}
really_inline document::object_result::object_result(error_code error) noexcept : simdjson_result<object>(error) {}

inline document::element_result document::object_result::operator[](std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first[json_pointer];
}
inline document::element_result document::object_result::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::object_result::at(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at(json_pointer);
}
inline document::element_result document::object_result::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline document::element_result document::object_result::at_key(const char *key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}

#if SIMDJSON_EXCEPTIONS

inline document::object::iterator document::object_result::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline document::object::iterator document::object_result::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

//
// document inline implementation
//
inline document::element document::root() const noexcept {
  return element(this, 1);
}
inline document::array_result document::as_array() const noexcept {
  return root().as_array();
}
inline document::object_result document::as_object() const noexcept {
  return root().as_object();
}
inline document::operator element() const noexcept {
  return root();
}

#if SIMDJSON_EXCEPTIONS

inline document::operator document::array() const noexcept(false) {
  return root();
}
inline document::operator document::object() const noexcept(false) {
  return root();
}

#endif

//#define REPORT_ERROR(CODE, MESSAGE) ((std::cerr << MESSAGE << std::endl), CODE)
#define REPORT_ERROR(CODE, MESSAGE) (CODE)
#define RETURN_ERROR(CODE, MESSAGE) return REPORT_ERROR((CODE), (MESSAGE));

inline document::element_result document::at(std::string_view json_pointer) const noexcept {
  if (json_pointer == "") { return root(); }
  // NOTE: JSON pointer requires a / at the beginning of the document; we allow it to be optional.
  return root().at(json_pointer.substr(json_pointer[0] == '/' ? 1 : 0));
}
inline document::element_result document::at(size_t index) const noexcept {
  return as_array().at(index);
}
inline document::element_result document::at_key(std::string_view key) const noexcept {
  return as_object().at_key(key);
}
inline document::element_result document::at_key(const char *key) const noexcept {
  return as_object().at_key(key);
}
inline document::element_result document::operator[](std::string_view json_pointer) const noexcept {
  return at(json_pointer);
}
inline document::element_result document::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}


inline document::doc_move_result document::load(const std::string &path) noexcept {
  document::parser parser;
  auto [doc, error] = parser.load(path);
  return doc_move_result((document &&)doc, error);
}

inline document::doc_move_result document::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  document::parser parser;
  auto [doc, error] = parser.parse(buf, len, realloc_if_needed);
  return doc_move_result((document &&)doc, error);
}
really_inline document::doc_move_result document::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_move_result document::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline document::doc_move_result document::parse(const padded_string &s) noexcept {
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
// doc_result inline implementation
//
inline document::doc_result::doc_result(document &doc, error_code error) noexcept : simdjson_result<document&>(doc, error) { }

inline document::array_result document::doc_result::as_array() const noexcept {
  if (error()) { return error(); }
  return first.root().as_array();
}
inline document::object_result document::doc_result::as_object() const noexcept {
  if (error()) { return error(); }
  return first.root().as_object();
}

inline document::element_result document::doc_result::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline document::element_result document::doc_result::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::doc_result::at(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at(key);
}
inline document::element_result document::doc_result::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
inline document::element_result document::doc_result::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline document::element_result document::doc_result::at_key(const char *key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}

//
// doc_move_result inline implementation
//
inline document::doc_move_result::doc_move_result(document &&doc, error_code error) noexcept : simdjson_move_result<document>(std::move(doc), error) { }
inline document::doc_move_result::doc_move_result(document &&doc) noexcept : simdjson_move_result<document>(std::move(doc)) { }
inline document::doc_move_result::doc_move_result(error_code error) noexcept : simdjson_move_result<document>(error) { }

inline document::array_result document::doc_move_result::as_array() const noexcept {
  if (error()) { return error(); }
  return first.root().as_array();
}
inline document::object_result document::doc_move_result::as_object() const noexcept {
  if (error()) { return error(); }
  return first.root().as_object();
}

inline document::element_result document::doc_move_result::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline document::element_result document::doc_move_result::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::doc_move_result::at(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at(key);
}
inline document::element_result document::doc_move_result::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
inline document::element_result document::doc_move_result::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline document::element_result document::doc_move_result::at_key(const char *key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}

//
// document::parser inline implementation
//
really_inline document::parser::parser(size_t max_capacity, size_t max_depth) noexcept
  : _max_capacity{max_capacity}, _max_depth{max_depth}, loaded_bytes(nullptr, &aligned_free_char) {

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

#if SIMDJSON_EXCEPTIONS

inline const document &document::parser::get_document() const noexcept(false) {
  if (!is_valid()) {
    throw simdjson_error(error);
  }
  return doc;
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<size_t> document::parser::read_file(const std::string &path) noexcept {
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

inline document::doc_result document::parser::load(const std::string &path) noexcept {
  auto [len, code] = read_file(path);
  if (code) { return doc_result(doc, code); }

  return parse(loaded_bytes.get(), len, false);
}

inline document::stream document::parser::load_many(const std::string &path, size_t batch_size) noexcept {
  auto [len, code] = read_file(path);
  return stream(*this, (const uint8_t*)loaded_bytes.get(), len, batch_size, code);
}

inline document::doc_result document::parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  error_code code = ensure_capacity(len);
  if (code) { return doc_result(doc, code); }

  if (realloc_if_needed) {
    const uint8_t *tmp_buf = buf;
    buf = (uint8_t *)internal::allocate_padded_buffer(len);
    if (buf == nullptr)
      return doc_result(doc, MEMALLOC);
    memcpy((void *)buf, tmp_buf, len);
  }

  code = simdjson::active_implementation->parse(buf, len, *this);

  // We're indicating validity via the doc_result, so set the parse state back to invalid
  valid = false;
  error = UNINITIALIZED;
  if (realloc_if_needed) {
    aligned_free((void *)buf); // must free before we exit
  }
  return doc_result(doc, code);
}
really_inline document::doc_result document::parser::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_result document::parser::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline document::doc_result document::parser::parse(const padded_string &s) noexcept {
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
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] ); // TODO realloc
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
// tape_ref inline implementation
//
really_inline internal::tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
really_inline internal::tape_ref::tape_ref(const document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}

inline size_t internal::tape_ref::after_element() const noexcept {
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
really_inline internal::tape_type internal::tape_ref::type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
really_inline uint64_t internal::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
template<typename T>
really_inline T internal::tape_ref::next_tape_value() const noexcept {
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

//
// array inline implementation
//
really_inline document::array::array() noexcept : internal::tape_ref() {}
really_inline document::array::array(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) {}
inline document::array::iterator document::array::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline document::array::iterator document::array::end() const noexcept {
  return iterator(doc, after_element() - 1);
}

inline document::element_result document::array::at(std::string_view json_pointer) const noexcept {
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
inline document::element_result document::array::at(size_t index) const noexcept {
  size_t i=0;
  for (auto element : *this) {
    if (i == index) { return element; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

//
// document::array::iterator inline implementation
//
really_inline document::array::iterator::iterator(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }
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
really_inline document::object::object() noexcept : internal::tape_ref() {}
really_inline document::object::object(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { };
inline document::object::iterator document::object::begin() const noexcept {
  return iterator(doc, json_index + 1);
}
inline document::object::iterator document::object::end() const noexcept {
  return iterator(doc, after_element() - 1);
}

inline document::element_result document::object::operator[](std::string_view json_pointer) const noexcept {
  return at(json_pointer);
}
inline document::element_result document::object::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::object::at(std::string_view json_pointer) const noexcept {
  // Unescape the key
  std::string unescaped;
  unescaped.reserve(json_pointer.length());
  size_t i;
  for (i = 0; i < json_pointer.length() && json_pointer[i] != '/'; i++) {
    switch (json_pointer[i]) {
      // Handle ~ escaping: ~0 = ~, ~1 = /
      case '~': {
        i++;
        // ~ at end of string is invalid
        if (i >= json_pointer.length()) { RETURN_ERROR(INVALID_JSON_POINTER, "~ at end of string in JSON pointer"); }
        switch (json_pointer[i]) {
          case '0':
            unescaped.push_back('~');
            break;
          case '1':
            unescaped.push_back('/');
            break;
          default:
            RETURN_ERROR(INVALID_JSON_POINTER, "Unexpected ~ escape character in JSON pointer");
        }
        break;
      }
      // TODO backslash doesn't appear to be a thing in JSON pointer
      case '\\': {
        i++;
        // backslash at end of string is invalid
        if (i >= json_pointer.length()) { RETURN_ERROR(INVALID_JSON_POINTER, "~ at end of string in JSON pointer"); }
        // Check for invalid escape characters
        if (json_pointer[i] != '\\' && json_pointer[i] != '"' && json_pointer[i] > 0x1F) {
          RETURN_ERROR(INVALID_JSON_POINTER, "Invalid backslash escape in JSON pointer");
        }
        unescaped.push_back(json_pointer[i]);
        break;
      }
      default:
        unescaped.push_back(json_pointer[i]);
        break;
    }
  }

  // Grab the child with the given key
  auto child = at_key(unescaped);

  // If there is a /, we have to recurse and look up more of the path
  if (i < json_pointer.length()) {
    child = child.at(json_pointer.substr(i+1));
  }

  return child;
}
inline document::element_result document::object::at_key(std::string_view key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (key == field.key()) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}
inline document::element_result document::object::at_key(const char *key) const noexcept {
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
really_inline document::object::iterator::iterator(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }
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
inline document::key_value_pair::key_value_pair(std::string_view _key, element _value) noexcept :
  key(_key), value(_value) {}

//
// element inline implementation
//
really_inline document::element::element() noexcept : internal::tape_ref() {}
really_inline document::element::element(const document *_doc, size_t _json_index) noexcept : internal::tape_ref(_doc, _json_index) { }

really_inline bool document::element::is_null() const noexcept {
  return type() == internal::tape_type::NULL_VALUE;
}
really_inline bool document::element::is_bool() const noexcept {
  return type() == internal::tape_type::TRUE_VALUE || type() == internal::tape_type::FALSE_VALUE;
}
really_inline bool document::element::is_number() const noexcept {
  return type() == internal::tape_type::UINT64 || type() == internal::tape_type::INT64 || type() == internal::tape_type::DOUBLE;
}
really_inline bool document::element::is_integer() const noexcept {
  return type() == internal::tape_type::UINT64 || type() == internal::tape_type::INT64;
}
really_inline bool document::element::is_string() const noexcept {
  return type() == internal::tape_type::STRING;
}
really_inline bool document::element::is_array() const noexcept {
  return type() == internal::tape_type::START_ARRAY;
}
really_inline bool document::element::is_object() const noexcept {
  return type() == internal::tape_type::START_OBJECT;
}

#if SIMDJSON_EXCEPTIONS

inline document::element::operator bool() const noexcept(false) { return as_bool(); }
inline document::element::operator const char*() const noexcept(false) { return as_c_str(); }
inline document::element::operator std::string_view() const noexcept(false) { return as_string(); }
inline document::element::operator uint64_t() const noexcept(false) { return as_uint64_t(); }
inline document::element::operator int64_t() const noexcept(false) { return as_int64_t(); }
inline document::element::operator double() const noexcept(false) { return as_double(); }
inline document::element::operator document::array() const noexcept(false) { return as_array(); }
inline document::element::operator document::object() const noexcept(false) { return as_object(); }

#endif

inline simdjson_result<bool> document::element::as_bool() const noexcept {
  switch (type()) {
    case internal::tape_type::TRUE_VALUE:
      return true;
    case internal::tape_type::FALSE_VALUE:
      return false;
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<const char *> document::element::as_c_str() const noexcept {
  switch (type()) {
    case internal::tape_type::STRING: {
      size_t string_buf_index = tape_value();
      return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<std::string_view> document::element::as_string() const noexcept {
  switch (type()) {
    case internal::tape_type::STRING:
      return get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<uint64_t> document::element::as_uint64_t() const noexcept {
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
inline simdjson_result<int64_t> document::element::as_int64_t() const noexcept {
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
inline simdjson_result<double> document::element::as_double() const noexcept {
  switch (type()) {
    case internal::tape_type::UINT64:
      return next_tape_value<uint64_t>();
    case internal::tape_type::INT64: {
      return next_tape_value<int64_t>();
      int64_t result = tape_value();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return result;
    }
    case internal::tape_type::DOUBLE:
      return next_tape_value<double>();
    default:
      return INCORRECT_TYPE;
  }
}
inline document::array_result document::element::as_array() const noexcept {
  switch (type()) {
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
inline document::object_result document::element::as_object() const noexcept {
  switch (type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index);
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result document::element::operator[](std::string_view json_pointer) const noexcept {
  return at(json_pointer);
}
inline document::element_result document::element::operator[](const char *json_pointer) const noexcept {
  return (*this)[std::string_view(json_pointer)];
}
inline document::element_result document::element::at(std::string_view json_pointer) const noexcept {
  switch (type()) {
    case internal::tape_type::START_OBJECT:
      return object(doc, json_index).at(json_pointer);
    case internal::tape_type::START_ARRAY:
      return array(doc, json_index).at(json_pointer);
    default:
      return INCORRECT_TYPE;
  }
}
inline document::element_result document::element::at(size_t index) const noexcept {
  return as_array().at(index);
}
inline document::element_result document::element::at_key(std::string_view key) const noexcept {
  return as_object().at_key(key);
}
inline document::element_result document::element::at_key(const char *key) const noexcept {
  return as_object().at_key(key);
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
  using tape_type=internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value.doc, value.json_index);
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

#if SIMDJSON_EXCEPTIONS

template<>
inline std::ostream& minify<document::doc_move_result>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<document>(value.first);
}
template<>
inline std::ostream& minify<document::doc_result>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<document>(value.first);
}
template<>
inline std::ostream& minify<document::element_result>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<document::element>(value.first);
}
template<>
inline std::ostream& minify<document::array_result>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<document::array>(value.first);
}
template<>
inline std::ostream& minify<document::object_result>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<document::object>(value.first);
}

#endif

} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_H
