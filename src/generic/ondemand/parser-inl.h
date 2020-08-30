namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code parser::allocate(size_t new_capacity, size_t new_max_depth) noexcept {
  if (new_capacity == _capacity && new_max_depth == _max_depth) { return SUCCESS; }

  // string_capacity copied from document::allocate
  _capacity = 0;
  _max_depth = 0;
  // The most string buffer we could possibly need is capacity-2 (a string the whole document long).
  // Allocate up to capacity so we don't have to check for capacity == 0 or 1.
  string_buf.reset(new (std::nothrow) uint8_t[new_capacity]);
  SIMDJSON_TRY( dom_parser.set_capacity(new_capacity) );
  SIMDJSON_TRY( dom_parser.set_max_depth(DEFAULT_MAX_DEPTH) );
  _capacity = new_capacity;
  _max_depth = new_max_depth;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<document> parser::parse(const padded_string &buf) noexcept {
  // Allocate if needed
  error_code error;
  if (_capacity < buf.size()) {
    error = allocate(buf.size(), _max_depth);
    if (error) {
      return { this, error };
    }
  }

  // Run stage 1.
  error = dom_parser.stage1((const uint8_t *)buf.data(), buf.size(), false);  
  return { this, error };
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
