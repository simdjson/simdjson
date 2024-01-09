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
  : _max_capacity{max_capacity},
    loaded_bytes(nullptr) {
}
simdjson_inline parser::parser(parser &&other) noexcept = default;
simdjson_inline parser &parser::operator=(parser &&other) noexcept = default;

inline bool parser::is_valid() const noexcept { return valid; }
inline int parser::get_error_code() const noexcept { return error; }
inline std::string parser::get_error_message() const noexcept { return error_message(error); }

inline bool parser::dump_raw_tape(std::ostream &os) const noexcept {
  return valid ? doc.dump_raw_tape(os) : false;
}

inline simdjson_result<size_t> parser::read_file(const std::string &path) noexcept {
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(path.c_str(), "rb");
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

inline simdjson_result<element> parser::load(const std::string &path) & noexcept {
  return load_into_document(doc, path);
}

inline simdjson_result<element> parser::load_into_document(document& provided_doc, const std::string &path) & noexcept {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  return parse_into_document(provided_doc, loaded_bytes.get(), len, false);
}

inline simdjson_result<document_stream> parser::load_many(const std::string &path, size_t batch_size) noexcept {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  return document_stream(*this, reinterpret_cast<const uint8_t*>(loaded_bytes.get()), len, batch_size);
}

inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const uint8_t *buf, size_t len, bool realloc_if_needed) & noexcept {
  // Important: we need to ensure that document has enough capacity.
  // Important: It is possible that provided_doc is actually the internal 'doc' within the parser!!!
  error_code _error = ensure_capacity(provided_doc, len);
  if (_error) { return _error; }
  if (realloc_if_needed) {
    // Make sure we have enough capacity to copy len bytes
    if (!loaded_bytes || _loaded_bytes_capacity < len) {
      loaded_bytes.reset( internal::allocate_padded_buffer(len) );
      if (!loaded_bytes) {
        return MEMALLOC;
      }
      _loaded_bytes_capacity = len;
    }
    std::memcpy(static_cast<void *>(loaded_bytes.get()), buf, len);
    buf = reinterpret_cast<const uint8_t*>(loaded_bytes.get());
  }

  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  _error = implementation->parse(buf, len, provided_doc);

  if (_error) { return _error; }

  return provided_doc.root();
}

simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const char *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse_into_document(provided_doc, reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const std::string &s) & noexcept {
  return parse_into_document(provided_doc, s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const padded_string &s) & noexcept {
  return parse_into_document(provided_doc, s.data(), s.length(), false);
}


inline simdjson_result<element> parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse_into_document(doc, buf, len, realloc_if_needed);
}

simdjson_inline simdjson_result<element> parser::parse(const char *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse(reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_inline simdjson_result<element> parser::parse(const std::string &s) & noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_inline simdjson_result<element> parser::parse(const padded_string &s) & noexcept {
  return parse(s.data(), s.length(), false);
}
simdjson_inline simdjson_result<element> parser::parse(const padded_string_view &v) & noexcept {
  return parse(v.data(), v.length(), false);
}

inline simdjson_result<document_stream> parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  if((len >= 3) && (std::memcmp(buf, "\xEF\xBB\xBF", 3) == 0)) {
    buf += 3;
    len -= 3;
  }
  return document_stream(*this, buf, len, batch_size);
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

simdjson_inline size_t parser::capacity() const noexcept {
  return implementation ? implementation->capacity() : 0;
}
simdjson_inline size_t parser::max_capacity() const noexcept {
  return _max_capacity;
}
simdjson_inline size_t parser::max_depth() const noexcept {
  return implementation ? implementation->max_depth() : DEFAULT_MAX_DEPTH;
}

simdjson_warn_unused
inline error_code parser::allocate(size_t capacity, size_t max_depth) noexcept {
  //
  // Reallocate implementation if needed
  //
  error_code err;
  if (implementation) {
    err = implementation->allocate(capacity, max_depth);
  } else {
    err = simdjson::get_active_implementation()->create_dom_parser_implementation(capacity, max_depth, implementation);
  }
  if (err) { return err; }
  return SUCCESS;
}

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
simdjson_warn_unused
inline bool parser::allocate_capacity(size_t capacity, size_t max_depth) noexcept {
  return !allocate(capacity, max_depth);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline error_code parser::ensure_capacity(size_t desired_capacity) noexcept {
  return ensure_capacity(doc, desired_capacity);
}


inline error_code parser::ensure_capacity(document& target_document, size_t desired_capacity) noexcept {
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
