#include "simdjson/document.h"
#include "simdjson/jsonparser.h"

namespace simdjson {

WARN_UNUSED
error_code document::parser::init_parse(size_t len) {
  if (len > capacity()) {
    return error = CAPACITY;
  }
  // If the last doc was taken, we need to allocate a new one
  if (!doc.tape) {
    if (!doc.set_capacity(len)) {
      return error = MEMALLOC;
    }
  }
  return SUCCESS;
}

WARN_UNUSED
bool document::parser::set_capacity(size_t capacity) {
  if (_capacity == capacity) {
    return true;
  }

  // Set capacity to 0 until we finish, in case there's an error
  _capacity = 0;

  //
  // Reallocate the document
  //
  if (!doc.set_capacity(capacity)) {
    return false;
  }

  //
  // Don't allocate 0 bytes, just return.
  //
  if (capacity == 0) {
    structural_indexes.reset();
    return true;
  }

  //
  // Initialize stage 1 output
  //
  uint32_t max_structures = ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]); // TODO realloc
  if (!structural_indexes) {
    return false;
  }

  _capacity = capacity;
  return true;
}

WARN_UNUSED
bool document::parser::set_max_depth(size_t max_depth) {
  _max_depth = 0;

  if (max_depth == 0) {
    ret_address.reset();
    containing_scope_offset.reset();
    return true;
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
    return false;
  }

  _max_depth = max_depth;
  return true;
}

void document::parser::init_stage2() {
  current_string_buf_loc = doc.string_buf.get();
  current_loc = 0;
  valid = false;
  error = UNINITIALIZED;
}

bool document::parser::is_valid() const { return valid; }

int document::parser::get_error_code() const { return error; }

std::string document::parser::get_error_message() const {
  return error_message(error);
}

WARN_UNUSED
bool document::parser::print_json(std::ostream &os) const {
  return is_valid() ? doc.print_json(os) : false;
}

WARN_UNUSED
bool document::parser::dump_raw_tape(std::ostream &os) const {
  return is_valid() ? doc.dump_raw_tape(os) : false;
}

} // namespace simdjson
