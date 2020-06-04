namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.containing_scope.reset(new (std::nothrow) scope_descriptor[max_depth]);
  parser.ret_address.reset(new (std::nothrow) ret_address_t[max_depth]);

  if (!parser.ret_address || !parser.containing_scope) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
