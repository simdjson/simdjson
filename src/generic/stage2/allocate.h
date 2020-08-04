namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
simdjson_really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.open_containers.reset(new (std::nothrow) open_container[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.open_containers) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
