#ifndef SIMDJSON_PARSER_INL_H
#define SIMDJSON_PARSER_INL_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/document_stream.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/dom_parser_implementation.h"

#include "simdjson/error-inl.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/dom/document_stream-inl.h"
#include "simdjson/dom/element-inl.h"

#include <climits>
#include <cstring> /* memcmp */

namespace simdjson {
namespace dom {

//
// parser inline implementation
//
simdjson_inline parser::parser(size_t max_capacity) noexcept
  : parser{get_default_allocator(), max_capacity} {
}
simdjson_inline parser::parser(simdjson::allocator& alloc, size_t max_capacity) noexcept
  : doc{alloc},
    _allocator{&alloc},
    _max_capacity{max_capacity},
    loaded_bytes{} {
}
simdjson_inline parser::parser(parser &&other) noexcept = default;
simdjson_inline parser &parser::operator=(parser &&other) noexcept = default;

inline bool parser::is_valid() const noexcept { return valid; }
inline int parser::get_error_code() const noexcept { return error; }
inline std::string parser::get_error_message() const noexcept { return error_message(error); }

inline bool parser::dump_raw_tape(std::ostream &os) const noexcept {
  return valid ? doc.dump_raw_tape(os) : false;
}

inline simdjson_result<size_t> parser::read_file(std::string_view path) noexcept {
  const std::string path_copy(path);
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(path_copy.c_str(), "rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  int ret;
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  ret = _fseeki64(fp, 0, SEEK_END);
#else
  ret = std::fseek(fp, 0, SEEK_END);
#endif // _WIN64
  if(ret < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  __int64 len = _ftelli64(fp);
  if(len == -1L) {
    std::fclose(fp);
    return IO_ERROR;
  }
#else
  long len = std::ftell(fp);
  if((len < 0) || (len == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }
#endif

  if (loaded_bytes.capacity() < size_t(len) + SIMDJSON_PADDING) {
    error_code err = allocate_loaded_bytes(size_t(len));
    if (err) {
      std::fclose(fp);
      return err;
    }
  }

  std::rewind(fp);
  size_t bytes_read = std::fread(loaded_bytes.get(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != size_t(len)) {
    return IO_ERROR;
  }

  return bytes_read;
}

inline error_code parser::allocate_loaded_bytes(size_t len) {
  const size_t padded_len = len + SIMDJSON_PADDING;
  if (padded_len < len) { return MEMALLOC; }
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  if (padded_len > (1UL << 20)) { return MEMALLOC; }
#endif
  auto new_bytes = internal::make_allocated_buffer<char>(padded_len, *_allocator);
  if (!new_bytes) { return MEMALLOC; }
  std::memset(new_bytes.get() + len, 0, SIMDJSON_PADDING);
  loaded_bytes = std::move(new_bytes);
  return SUCCESS;
}

inline simdjson_result<element> parser::load(std::string_view path) & {
  return load_into_document(doc, path);
}

inline simdjson_result<element> parser::load_into_document(document& provided_doc, std::string_view path) & {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  return parse_into_document(provided_doc, loaded_bytes.get(), len, false);
}

inline simdjson_result<document_stream> parser::load_many(std::string_view path, size_t batch_size) {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  return document_stream(*this, reinterpret_cast<const uint8_t*>(loaded_bytes.get()), len, batch_size);
}

inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const uint8_t *buf, size_t len, bool realloc_if_needed) & {
  // Important: we need to ensure that document has enough capacity.
  // Important: It is possible that provided_doc is actually the internal 'doc' within the parser!!!
  error_code _error = ensure_capacity(provided_doc, len);
  if (_error) { return _error; }
  if (realloc_if_needed) {
    if (loaded_bytes.capacity() < len + SIMDJSON_PADDING) {
      SIMDJSON_TRY(allocate_loaded_bytes(len));
    }
    std::memcpy(static_cast<void *>(loaded_bytes.get()), buf, len);
    buf = reinterpret_cast<const uint8_t*>(loaded_bytes.get());
  }

  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  implementation->_number_as_string = _number_as_string;
  _error = implementation->parse(buf, len, provided_doc);

  if (_error) { return _error; }

  return provided_doc.root();
}

simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const char *buf, size_t len, bool realloc_if_needed) & {
  return parse_into_document(provided_doc, reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const std::string &s) & {
  return parse_into_document(provided_doc, s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const padded_string &s) & {
  return parse_into_document(provided_doc, s.data(), s.length(), false);
}


inline simdjson_result<element> parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) & {
  return parse_into_document(doc, buf, len, realloc_if_needed);
}

simdjson_inline simdjson_result<element> parser::parse(const char *buf, size_t len, bool realloc_if_needed) & {
  return parse(reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_inline simdjson_result<element> parser::parse(const std::string &s) & {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_inline simdjson_result<element> parser::parse(const padded_string &s) & {
  return parse(s.data(), s.length(), false);
}
simdjson_inline simdjson_result<element> parser::parse(const padded_string_view &v) & {
  return parse(v.data(), v.length(), false);
}

inline simdjson_result<document_stream> parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  return parse_many(buf, len, batch_size, stream_format::whitespace_delimited);
}
inline simdjson_result<document_stream> parser::parse_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return parse_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const std::string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const padded_string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const padded_string_view &v, size_t batch_size) noexcept {
  return parse_many(v.data(), v.length(), batch_size);
}

inline simdjson_result<document_stream> parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size, stream_format format) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  if (format == stream_format::comma_delimited_array) {
    // Strip leading JSON whitespace.
    while (len > 0 && (buf[0] == ' ' || buf[0] == '\t' || buf[0] == '\n' || buf[0] == '\r')) {
      buf++; len--;
    }
    // Expect the opening '['.
    if (len == 0 || buf[0] != '[') { return TAPE_ERROR; }
    buf++; len--;
    // Strip trailing JSON whitespace.
    while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t' || buf[len-1] == '\n' || buf[len-1] == '\r')) {
      len--;
    }
    // Expect the closing ']'.
    if (len == 0 || buf[len-1] != ']') { return TAPE_ERROR; }
    len--;
    // Fall through to comma_delimited over the array contents.
    format = stream_format::comma_delimited;
  }
  return document_stream(*this, buf, len, batch_size, format);
}
inline simdjson_result<document_stream> parser::parse_many(const char *buf, size_t len, size_t batch_size, stream_format format) noexcept {
  return parse_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size, format);
}
inline simdjson_result<document_stream> parser::parse_many(const std::string &s, size_t batch_size, stream_format format) noexcept {
  return parse_many(s.data(), s.length(), batch_size, format);
}
inline simdjson_result<document_stream> parser::parse_many(const padded_string &s, size_t batch_size, stream_format format) noexcept {
  return parse_many(s.data(), s.length(), batch_size, format);
}
inline simdjson_result<document_stream> parser::parse_many(const padded_string_view &v, size_t batch_size, stream_format format) noexcept {
  return parse_many(v.data(), v.length(), batch_size, format);
}

simdjson_inline size_t parser::capacity() const noexcept {
  return implementation ? implementation->capacity() : 0;
}
simdjson_inline size_t parser::max_capacity() const noexcept {
  return _max_capacity;
}
simdjson_pure simdjson_inline size_t parser::max_depth() const noexcept {
  return implementation ? implementation->max_depth() : DEFAULT_MAX_DEPTH;
}

simdjson_warn_unused
inline error_code parser::allocate(size_t capacity, size_t max_depth) {
  error_code err;
  if (implementation) {
    err = implementation->allocate(capacity, max_depth);
  } else {
    err = simdjson::get_active_implementation()->create_dom_parser_implementation(capacity, max_depth, implementation, *_allocator);
  }
  if (err) { return err; }
  return SUCCESS;
}

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
simdjson_warn_unused
inline bool parser::allocate_capacity(size_t capacity, size_t max_depth) {
  return !allocate(capacity, max_depth);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline error_code parser::ensure_capacity(size_t desired_capacity) {
  return ensure_capacity(doc, desired_capacity);
}


inline error_code parser::ensure_capacity(document& target_document, size_t desired_capacity) {
  // 1. It is wasteful to allocate a document and a parser for documents spanning less than MINIMAL_DOCUMENT_CAPACITY bytes.
  // 2. If we allow desired_capacity = 0 then it is possible to exit this function with implementation == nullptr.
  if(desired_capacity < MINIMAL_DOCUMENT_CAPACITY) { desired_capacity = MINIMAL_DOCUMENT_CAPACITY; }
  // If we don't have enough capacity, (try to) automatically bump it.
  // If the document needs allocation, do it too.
  // Both in one if statement to minimize unlikely branching.
  //
  // Note: we must make sure that this function is called if capacity() == 0. We do so because we
  // ensure that desired_capacity > 0.
  if (simdjson_unlikely(capacity() < desired_capacity || target_document.capacity() < desired_capacity)) {
    if (desired_capacity > max_capacity()) {
      return error = CAPACITY;
    }
    error_code err1 = target_document.capacity() < desired_capacity ? target_document.allocate(desired_capacity) : SUCCESS;
    error_code err2 = capacity() < desired_capacity ? allocate(desired_capacity, max_depth()) : SUCCESS;
    if(err1 != SUCCESS) { return error = err1; }
    if(err2 != SUCCESS) { return error = err2; }
  }
  return SUCCESS;
}

simdjson_inline void parser::set_max_capacity(size_t max_capacity) noexcept {
  if(max_capacity > MINIMAL_DOCUMENT_CAPACITY) {
    _max_capacity = max_capacity;
  } else {
    _max_capacity = MINIMAL_DOCUMENT_CAPACITY;
  }
}

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_PARSER_INL_H
