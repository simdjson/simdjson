#ifndef SIMDJSON_GENERIC_ONDEMAND_TOKEN_ITERATOR_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_TOKEN_ITERATOR_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/token_iterator.h"
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_inline token_iterator::token_iterator(
  const uint8_t *_buf,
  token_position position
) noexcept : buf{_buf}, _position{position}
{
}

simdjson_inline uint32_t token_iterator::current_offset() const noexcept {
  return *(_position);
}


simdjson_inline const uint8_t *token_iterator::return_current_and_advance() noexcept {
  return &buf[*(_position++)];
}

simdjson_inline const uint8_t *token_iterator::peek(token_position position) const noexcept {
  return &buf[*position];
}
simdjson_inline uint32_t token_iterator::peek_index(token_position position) const noexcept {
  return *position;
}
simdjson_inline uint32_t token_iterator::peek_length(token_position position) const noexcept {
  return *(position+1) - *position;
}

simdjson_inline uint32_t token_iterator::peek_root_length(token_position position) const noexcept {
  return *(position+2) - *(position) > *(position+1) - *(position) ?
      *(position+1) - *(position)
      : *(position+2) - *(position);
}
simdjson_inline const uint8_t *token_iterator::peek(int32_t delta) const noexcept {
  return &buf[*(_position+delta)];
}
simdjson_inline uint32_t token_iterator::peek_index(int32_t delta) const noexcept {
  return *(_position+delta);
}
simdjson_inline uint32_t token_iterator::peek_length(int32_t delta) const noexcept {
  return *(_position+delta+1) - *(_position+delta);
}

simdjson_inline token_position token_iterator::position() const noexcept {
  return _position;
}
simdjson_inline void token_iterator::set_position(token_position target_position) noexcept {
  _position = target_position;
}

simdjson_inline bool token_iterator::operator==(const token_iterator &other) const noexcept {
  return _position == other._position;
}
simdjson_inline bool token_iterator::operator!=(const token_iterator &other) const noexcept {
  return _position != other._position;
}
simdjson_inline bool token_iterator::operator>(const token_iterator &other) const noexcept {
  return _position > other._position;
}
simdjson_inline bool token_iterator::operator>=(const token_iterator &other) const noexcept {
  return _position >= other._position;
}
simdjson_inline bool token_iterator::operator<(const token_iterator &other) const noexcept {
  return _position < other._position;
}
simdjson_inline bool token_iterator::operator<=(const token_iterator &other) const noexcept {
  return _position <= other._position;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::token_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::token_iterator>(error) {}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_TOKEN_ITERATOR_INL_H