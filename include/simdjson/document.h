#ifndef SIMDJSON_DOCUMENT_H
#define SIMDJSON_DOCUMENT_H

#include <cstring>
#include <memory>
#include <string>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include "simdjson/padded_string.h"

#define JSON_VALUE_MASK 0xFFFFFFFFFFFFFF
#define DEFAULT_MAX_DEPTH 1024 // a JSON document with a depth exceeding 1024 is probably de facto invalid

namespace simdjson {

template<size_t max_depth> class document_iterator;
class document_parser;

class document {
public:
  // create a document container with zero capacity, parser will allocate capacity as needed
  document()=default;
  ~document()=default;

  // this is a move only class
  document(document &&p) = default;
  document(const document &p) = delete;
  document &operator=(document &&o) = default;
  document &operator=(const document &o) = delete;

  // Nested classes. See definitions later in file.
  using iterator = document_iterator<DEFAULT_MAX_DEPTH>;
  class parser;
  class doc_result;
  class doc_ref_result;

  //
  // Tell whether this document has been parsed, or is just empty.
  //
  bool is_initialized() {
    return tape && string_buf;
  }

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  bool print_json(std::ostream &os, size_t max_depth=DEFAULT_MAX_DEPTH) const;
  WARN_UNUSED
  bool dump_raw_tape(std::ostream &os) const;

  //
  // Parse a JSON document.
  //
  // If you will be parsing more than one JSON document, it's recommended to create a
  // document::parser object instead, keeping internal buffers around for efficiency reasons.
  //
  // Throws invalid_json if the JSON is invalid.
  //
  static doc_result parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept;
  static doc_result parse(const char *buf, size_t len, bool realloc_if_needed = true) noexcept;
  static doc_result parse(const std::string &s, bool realloc_if_needed = true) noexcept;
  static doc_result parse(const padded_string &s) noexcept;
  // We do not want to allow implicit conversion from C string to std::string.
  doc_result parse(const char *buf, bool realloc_if_needed = true) noexcept = delete;

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity

private:
  bool set_capacity(size_t len);
}; // class document

class document::doc_result {
private:
  doc_result(document &&_doc, error_code _error) : doc(std::move(_doc)), error(_error) { }
  doc_result(document &&_doc) : doc(std::move(_doc)), error(SUCCESS) { }
  doc_result(error_code _error) : doc(), error(_error) { }
  friend class document;
public:
  ~doc_result()=default;

  operator bool() noexcept { return error == SUCCESS; }
  operator document() {
    if (!*this) {
      throw invalid_json(error);
    }
    return std::move(doc);
  }
  document doc;
  error_code error;
  const std::string &get_error_message() {
    return error_message(error);
  }
}; // class doc_result

/**
  * The result of document::parser::parse(). Stores an error code and a document reference.
  *
  * Designed so that you can either check the error code before using the document, or use
  * exceptions and use thedirectly and parse it, or 
  *
  */
class document::doc_ref_result {
public:
  doc_ref_result(document &_doc, error_code _error) : doc(_doc), error(_error) { }
  ~doc_ref_result()=default;

  operator bool() noexcept { return error == SUCCESS; }
  operator document&() {
    if (!*this) {
      throw invalid_json(error);
    }
    return doc;
  }
  document& doc;
  error_code error;
  const std::string &get_error_message() noexcept {
    return error_message(error);
  }
}; // class document::doc_result

/**
  * A persistent document parser.
  *
  * Use this if you intend to parse more than one document. It holds the internal memory necessary
  * to do parsing, as well as memory for a single document that is overwritten on each parse.
  *
  * @note This is not thread safe: one parser cannot produce two documents at the same time!
  */
class document::parser {
public:
  /**
  * Create a JSON parser with zero capacity. Call allocate_capacity() to initialize it.
  */
  parser()=default;
  ~parser()=default;

  // this is a move only class
  parser(document::parser &&p) = default;
  parser(const document::parser &p) = delete;
  parser &operator=(document::parser &&o) = default;
  parser &operator=(const document::parser &o) = delete;

  //
  // Parse a JSON document and return a reference to it.
  //
  // The JSON document still lives in the parser: this is the most efficient way to parse JSON
  // documents because it reuses the same buffers, but you *must* use the document before you
  // destroy the parser or call parse() again.
  //
  // Throws invalid_json if the JSON is invalid.
  //
  inline doc_ref_result parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept;
  inline doc_ref_result parse(const char *buf, size_t len, bool realloc_if_needed = true) noexcept;
  inline doc_ref_result parse(const std::string &s, bool realloc_if_needed = true) noexcept;
  inline doc_ref_result parse(const padded_string &s) noexcept;
  // We do not want to allow implicit conversion from C string to std::string.
  doc_ref_result parse(const char *buf, bool realloc_if_needed = true) noexcept = delete;

  //
  // Current capacity: the largest document this parser can support without reallocating.
  //
  size_t capacity() { return _capacity; }

  //
  // The maximum level of nested object and arrays supported by this parser.
  //
  size_t max_depth() { return _max_depth; }

  // if needed, allocate memory so that the object is able to process JSON
  // documents having up to capacity bytes and max_depth "depth"
  WARN_UNUSED bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) {
    return set_capacity(capacity) && set_max_depth(max_depth);
  }

  // type aliases for backcompat
  using Iterator = document::iterator;
  using InvalidJSON = invalid_json;

  // Next location to write to in the tape
  uint32_t current_loc{0};

  // structural indices passed from stage 1 to stage 2
  uint32_t n_structural_indexes{0};
  std::unique_ptr<uint32_t[]> structural_indexes;

  // location and return address of each open { or [
  std::unique_ptr<uint32_t[]> containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  // Next place to write a string
  uint8_t *current_string_buf_loc;

  bool valid{false};
  error_code error{UNINITIALIZED};

  // Document we're writing to
  document doc;

  //
  // TODO these are deprecated; use the results of parse instead.
  //

  // returns true if the document parsed was valid
  bool is_valid() const { return valid; }

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return UNITIALIZED if no parsing was attempted
  int get_error_code() const { return error; }

  // return the string equivalent of "get_error_code"
  std::string get_error_message() const { return error_message(error); }

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  WARN_UNUSED
  inline bool print_json(std::ostream &os) const { return is_valid() ? doc.print_json(os) : false; }
  WARN_UNUSED
  inline bool dump_raw_tape(std::ostream &os) const { return is_valid() ? doc.dump_raw_tape(os) : false; }

  //
  // Parser callbacks: these are internal!
  //
  // TODO find a way to do this without exposing the interface or crippling performance
  //

  // this should be called when parsing (right before writing the tapes)
  really_inline void init_stage2();
  really_inline error_code on_error(error_code new_error_code);
  really_inline error_code on_success(error_code success_code);
  really_inline bool on_start_document(uint32_t depth);
  really_inline bool on_start_object(uint32_t depth);
  really_inline bool on_start_array(uint32_t depth);
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth);
  really_inline bool on_end_object(uint32_t depth);
  really_inline bool on_end_array(uint32_t depth);
  really_inline bool on_true_atom();
  really_inline bool on_false_atom();
  really_inline bool on_null_atom();
  really_inline uint8_t *on_start_string();
  really_inline bool on_end_string(uint8_t *dst);
  really_inline bool on_number_s64(int64_t value);
  really_inline bool on_number_u64(uint64_t value);
  really_inline bool on_number_double(double value);
  //
  // Called before a parse is initiated.
  //
  // - Returns CAPACITY if the document is too large
  // - Returns MEMALLOC if we needed to allocate memory and could not
  //
  WARN_UNUSED really_inline error_code init_parse(size_t len);

  const document &get_document() const noexcept(false) {
    if (!is_valid()) {
      throw invalid_json(error);
    }
    return doc;
  }

private:
  //
  // The maximum document length this parser supports.
  //
  // Buffers are large enough to handle any document up to this length.
  //
  size_t _capacity{0};

  //
  // The maximum depth (number of nested objects and arrays) supported by this parser.
  //
  // Defaults to DEFAULT_MAX_DEPTH.
  //
  size_t _max_depth{0};

  // all nodes are stored on the doc.tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the doc.tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  really_inline void write_tape(uint64_t val, uint8_t c) {
    doc.tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    doc.tape[saved_loc] |= val;
  }

  //
  // Set the current capacity: the largest document this parser can support without reallocating.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  WARN_UNUSED bool set_capacity(size_t capacity);

  //
  // Set the maximum level of nested object and arrays supported by this parser.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  WARN_UNUSED bool set_max_depth(size_t max_depth);
}; // class parser

} // namespace simdjson

#include "simdjson/inline/document.h"
#include "simdjson/document_iterator.h"

#endif // SIMDJSON_DOCUMENT_H