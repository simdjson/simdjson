namespace stage2 {
namespace atomparsing {

really_inline uint32_t string_to_uint32(const char* str) { return *reinterpret_cast<const uint32_t *>(str); }

WARN_UNUSED
really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | is_not_structural_or_whitespace(src[4])) == 0;
}

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | is_not_structural_or_whitespace(src[5])) == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | is_not_structural_or_whitespace(src[4])) == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // namespace stage2
