namespace stage2 {

/**
 * State stored in the parser for stage 2
 */
struct stage2_state {
  /** Allocate anything that needs allocating */
  really_inline error_code allocate_stage2(parser &parser, size_t capacity, size_t max_depth);
}; // struct stage2_state

really_inline error_code stage2_state::allocate_stage2(parser &parser, UNUSED size_t capacity, size_t max_depth) {
  if (max_depth == 0) {
    parser.ret_address.reset(); // Don't allocate 0 bytes
    parser.containing_scope.reset(); // Don't allocate 0 bytes
    return SUCCESS;
  }

  parser.ret_address.reset(new (std::nothrow) internal::ret_address[max_depth]);
  parser.containing_scope.reset(new (std::nothrow) internal::scope_descriptor[max_depth]); // TODO realloc

  if (!parser.ret_address || !parser.containing_scope) {
    return MEMALLOC;
  }

  return SUCCESS;
}

} // namespace stage2
