#include "simdjson/document/parser.h"

namespace simdjson {

WARN_UNUSED
bool document::parser::allocate_capacity(size_t len, size_t max_depth) {
  if (len <= 0) {
    len = 64; // allocating 0 bytes is wasteful.
  }
  if (len > SIMDJSON_MAXSIZE_BYTES) {
    return false;
  }
  if ((len <= byte_capacity) && (max_depth <= depth_capacity)) {
    return true;
  }
  if (max_depth <= 0) {
    max_depth = 1; // don't let the user allocate nothing
  }

  deallocate();

  n_structural_indexes = 0;
  uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]);

  containing_scope_offset.reset(new (std::nothrow) uint32_t[max_depth]);
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  //ret_address = new (std::nothrow) void *[max_depth];
  ret_address.reset(new (std::nothrow) void *[max_depth]);
#else
  ret_address.reset(new (std::nothrow) char[max_depth]);
#endif
  if (!containing_scope_offset || !ret_address || !structural_indexes) {
    // Could not allocate memory
    return false;
  }

  /*
  // We do not need to initialize this content for parsing, though we could
  // need to initialize it for safety.
  memset(structural_indexes, 0, max_structures * sizeof(uint32_t));
  */

  byte_capacity = len;
  depth_capacity = max_depth;
  return true;
}

void document::parser::deallocate() {
  byte_capacity = 0;
  depth_capacity = 0;
  ret_address.reset();
  containing_scope_offset.reset();
}

void document::parser::init(document &doc) {
  current_string_buf_loc = doc.string_buf.get();
  current_loc = 0;
  doc.reset();
}

} // namespace simdjson
