namespace simdjson {
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

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<document> parser::iterate(const padded_string &buf) & noexcept {
  // Allocate if needed
  if (_capacity < buf.size()) {
    SIMDJSON_TRY( allocate(buf.size(), _max_depth) );
  }

  // Run stage 1.
  SIMDJSON_TRY( dom_parser.stage1((const uint8_t *)buf.data(), buf.size(), false) );  
  return document::start(this);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<json_iterator> parser::iterate_raw(const padded_string &buf) & noexcept {
  // Allocate if needed
  if (_capacity < buf.size()) {
    SIMDJSON_TRY( allocate(buf.size(), _max_depth) );
  }

  // Run stage 1.
  SIMDJSON_TRY( dom_parser.stage1((const uint8_t *)buf.data(), buf.size(), false) );  
  return json_iterator(this);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::parser &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::parser>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser>(error) {}

} // namespace simdjson
