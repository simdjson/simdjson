namespace stage2 {

#ifdef SIMDJSON_USE_COMPUTED_GOTO
typedef void* ret_address_t;
#else
typedef char ret_address_t;
#endif

// expectation: sizeof(scope_descriptor) = 64/8.
struct scope_descriptor {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct scope_descriptor

/**
 * State stored in the parser for stage 2
 */
struct stage2_state {
  /** @private Tape location of each open { or [ */
  std::unique_ptr<scope_descriptor[]> containing_scope{};
  /** @private Return address of each open { or [ */
  std::unique_ptr<ret_address_t[]> ret_address{};
  /** Allocate anything that needs allocating */
  really_inline error_code allocate_stage2(parser &parser, size_t capacity, size_t max_depth);
}; // struct stage2_state

really_inline error_code stage2_state::allocate_stage2(UNUSED parser &parser, UNUSED size_t capacity, size_t max_depth) {
  if (max_depth == 0) {
    ret_address.reset(); // Don't allocate 0 bytes
    containing_scope.reset(); // Don't allocate 0 bytes
    return SUCCESS;
  }

  ret_address.reset(new (std::nothrow) ret_address_t[max_depth]);
  containing_scope.reset(new (std::nothrow) scope_descriptor[max_depth]); // TODO realloc

  if (!ret_address || !containing_scope) {
    return MEMALLOC;
  }

  return SUCCESS;
}

} // namespace stage2
