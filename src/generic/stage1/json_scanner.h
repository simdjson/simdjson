namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * A block of scanned json, with information on operators and scalars.
 *
 * We seek to identify pseudo-structural characters. Anything that is inside
 * a string must be omitted (hence  & ~_string.string_tail()).
 * Otherwise, pseudo-structural characters come in two forms.
 * 1. We have the structural characters ([,],{,},:, comma). The 
 *    term 'structural character' is from the JSON RFC.
 * 2. We have the 'scalar pseudo-structural characters'.
 *    Scalars are quotes, and any character except structural characters and white space.
 *
 * To identify the scalar pseudo-structural characters, we must look at what comes
 * before them: it must be a space, a quote or a structural characters.
 * Starting with simdjson v0.3, we identify them by
 * negation: we identify everything that is followed by a non-quote scalar,
 * and we negate that. Whatever remains must be a 'scalar pseudo-structural character'.
 */
struct json_block {
public:
  /**
   * The start of structurals.
   * In simdjson prior to v0.3, these were called the pseudo-structural characters.
   **/
  simdjson_really_inline uint64_t structural_start() { return potential_structural_start() & ~_string.string_tail(); }
  /** All JSON whitespace (i.e. not in a string) */
  simdjson_really_inline uint64_t whitespace() { return non_quote_outside_string(_characters.whitespace()); }

  // Helpers

  /** Whether the given characters are inside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) { return _string.non_quote_inside_string(mask); }
  /** Whether the given characters are outside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) { return _string.non_quote_outside_string(mask); }

  // string and escape characters
  json_string_block _string;
  // whitespace, structural characters ('operators'), scalars
  json_character_block _characters;
  // whether the previous character was a scalar
  uint64_t _follows_potential_nonquote_scalar;
private:
  // Potential structurals (i.e. disregarding strings)

  /**
   * structural elements ([,],{,},:, comma) plus scalar starts like 123, true and "abc".
   * They may reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_structural_start() { return _characters.op() | potential_scalar_start(); }
  /**
   * The start of non-operator runs, like 123, true and "abc".
   * It main reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_scalar_start() {
    // The term "scalar" refers to anything except structural characters and white space
    // (so letters, numbers, quotes).
    // Whenever it is preceded by something that is not a structural element ({,},[,],:, ") nor a white-space
    // then we know that it is irrelevant structurally.
    return _characters.scalar() & ~follows_potential_scalar();
  }
  /**
   * Whether the given character is immediately after a non-operator like 123, true.
   * The characters following a quote are not included.
   */
  simdjson_really_inline uint64_t follows_potential_scalar() {
    // _follows_potential_nonquote_scalar: is defined as marking any character that follows a character
    // that is not a structural element ({,},[,],:, comma) nor a quote (") and that is not a
    // white space.
    // It is understood that within quoted region, anything at all could be marked (irrelevant).
    return _follows_potential_nonquote_scalar;
  }
};

/**
 * Scans JSON for important bits: structural characters or 'operators', strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - structural characters or 'operators' ([]{},:, comma)
 *   and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  json_scanner() {}
  simdjson_really_inline json_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  uint64_t prev_scalar = 0ULL;
  json_string_scanner string_scanner{};
};


//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
simdjson_really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

simdjson_really_inline json_block json_scanner::next(const simd::simd8x64<uint8_t>& in) {
  json_string_block strings = string_scanner.next(in);
  // identifies the white-space and the structurat characters
  json_character_block characters = json_character_block::classify(in);
  // The term "scalar" refers to anything except structural characters and white space
  // (so letters, numbers, quotes).
  // We want  follows_scalar to mark anything that follows a non-quote scalar (so letters and numbers).
  //
  // A terminal quote should either be followed by a structural character (comma, brace, bracket, colon)
  // or nothing. However, we still want ' "a string"true ' to mark the 't' of 'true' as a potential
  // pseudo-structural character just like we would if we had  ' "a string" true '; otherwise we
  // may need to add an extra check when parsing strings.
  //
  // Performance: there are many ways to skin this cat.
  const uint64_t nonquote_scalar = characters.scalar() & ~strings.quote();
  uint64_t follows_nonquote_scalar = follows(nonquote_scalar, prev_scalar);
  return {
    strings,
    characters,
    follows_nonquote_scalar
  };
}

simdjson_really_inline error_code json_scanner::finish(bool streaming) {
  return string_scanner.finish(streaming);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
