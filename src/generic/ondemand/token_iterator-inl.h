namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline token_iterator::token_iterator() noexcept = default;
simdjson_really_inline token_iterator::token_iterator(token_iterator &&other) noexcept = default;
simdjson_really_inline token_iterator &token_iterator::operator=(token_iterator &&other) noexcept = default;
simdjson_really_inline token_iterator::token_iterator(const uint8_t *_buf, uint32_t *_index, uint32_t _depth) noexcept
  : buf{_buf}, index{_index}, depth{_depth}
{
}

simdjson_really_inline const uint8_t *token_iterator::peek(int32_t delta) const noexcept {
  return &buf[*(index+delta)];
}
simdjson_really_inline const uint8_t *token_iterator::advance() noexcept {
  return &buf[*(index++)];
}
simdjson_really_inline uint32_t token_iterator::peek_index(int32_t delta) const noexcept {
  return *(index+delta);
}

simdjson_really_inline void token_iterator::skip_unfinished_children(uint32_t container_depth) noexcept {
  SIMDJSON_ASSUME(depth >= container_depth);
  while (depth != container_depth) {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': depth--; logger::log_end_value(*this, "skip"); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip"); depth++; break;
      default: logger::log_value(*this, "skip", ""); break;
    }
  }
}

simdjson_really_inline void token_iterator::skip_value() noexcept {
  uint32_t child_depth = 0;
  do {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': child_depth--; logger::log_end_value(*this, "skip", -1, child_depth); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip", -1, child_depth); child_depth++; break;
      default: logger::log_value(*this, "skip", "", -1, child_depth); break;
    }
  } while (child_depth != 0);
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
} // namespace {
