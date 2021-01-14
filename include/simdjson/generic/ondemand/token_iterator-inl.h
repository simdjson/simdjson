namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline token_iterator::token_iterator(const uint8_t *_buf, token_position _index) noexcept
  : buf{_buf}, index{_index}
{
}

simdjson_really_inline const uint8_t *token_iterator::advance() noexcept {
  return &buf[*(index++)];
}

simdjson_really_inline const uint8_t *token_iterator::peek(token_position position) const noexcept {
  return &buf[*position];
}
simdjson_really_inline uint32_t token_iterator::peek_index(token_position position) const noexcept {
  return *position;
}
simdjson_really_inline uint32_t token_iterator::peek_length(token_position position) const noexcept {
  return *(position+1) - *position;
}

simdjson_really_inline const uint8_t *token_iterator::peek(int32_t delta) const noexcept {
  return &buf[*(index+delta)];
}
simdjson_really_inline uint32_t token_iterator::peek_index(int32_t delta) const noexcept {
  return *(index+delta);
}
simdjson_really_inline uint32_t token_iterator::peek_length(int32_t delta) const noexcept {
  return *(index+delta+1) - *(index+delta);
}

simdjson_really_inline token_position token_iterator::position() const noexcept {
  return index;
}
simdjson_really_inline void token_iterator::set_position(token_position target_checkpoint) noexcept {
  index = target_checkpoint;
}

simdjson_really_inline bool token_iterator::operator==(const token_iterator &other) const noexcept {
  return index == other.index;
}
simdjson_really_inline bool token_iterator::operator!=(const token_iterator &other) const noexcept {
  return index != other.index;
}
simdjson_really_inline bool token_iterator::operator>(const token_iterator &other) const noexcept {
  return index > other.index;
}
simdjson_really_inline bool token_iterator::operator>=(const token_iterator &other) const noexcept {
  return index >= other.index;
}
simdjson_really_inline bool token_iterator::operator<(const token_iterator &other) const noexcept {
  return index < other.index;
}
simdjson_really_inline bool token_iterator::operator<=(const token_iterator &other) const noexcept {
  return index <= other.index;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::token_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(error) {}

} // namespace simdjson
