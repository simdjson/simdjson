namespace stage1 {

/**
 * A block of scanned json, with information on operators and scalars.
 */
struct json_block {
public:
  /** The start of structurals */
  really_inline uint64_t structural_start() { return potential_structural_start() & ~_string.string_tail(); }
  /** All JSON whitespace (i.e. not in a string) */
  really_inline uint64_t whitespace() { return non_quote_outside_string(_characters.whitespace()); }

  // Helpers

  /** Whether the given characters are inside a string (only works on non-quotes) */
  really_inline uint64_t non_quote_inside_string(uint64_t mask) { return _string.non_quote_inside_string(mask); }
  /** Whether the given characters are outside a string (only works on non-quotes) */
  really_inline uint64_t non_quote_outside_string(uint64_t mask) { return _string.non_quote_outside_string(mask); }

  // string and escape characters
  json_string_block _string;
  // whitespace, operators, scalars
  json_character_block _characters;
  // whether the previous character was a scalar
  uint64_t _follows_potential_scalar;
private:
  // Potential structurals (i.e. disregarding strings)

  /** operators plus scalar starts like 123, true and "abc" */
  really_inline uint64_t potential_structural_start() { return _characters.op() | potential_scalar_start(); }
  /** the start of non-operator runs, like 123, true and "abc" */
  really_inline uint64_t potential_scalar_start() { return _characters.scalar() & ~follows_potential_scalar(); }
  /** whether the given character is immediately after a non-operator like 123, true or " */
  really_inline uint64_t follows_potential_scalar() { return _follows_potential_scalar; }
};

/**
 * Scans JSON for important bits: operators, strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - operators ([]{},:) and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  really_inline json_block next(const simd::simd8x64<uint8_t> in);
  really_inline error_code finish(bool streaming);

private:
  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or an operator).
  uint64_t prev_scalar = 0ULL;
  json_string_scanner string_scanner;
};


//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= uint64_t(add_overflow(follows_match, filler, &result));
  return result;
}

really_inline json_block json_scanner::next(const simd::simd8x64<uint8_t> in) {
  json_string_block strings = string_scanner.next(in);
  json_character_block characters = json_character_block::classify(in);
  uint64_t follows_scalar = follows(characters.scalar(), prev_scalar);
  return {
    strings,
    characters,
    follows_scalar
  };
}

really_inline error_code json_scanner::finish(bool streaming) {
  return string_scanner.finish(streaming);
}

} // namespace stage1