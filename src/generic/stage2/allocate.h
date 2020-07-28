namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.containing_scope.reset(new (std::nothrow) scope_descriptor[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.containing_scope) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
