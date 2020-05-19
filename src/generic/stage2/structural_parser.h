// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

namespace stage2 {

using internal::ret_address;

#ifdef SIMDJSON_USE_COMPUTED_GOTO
#define INIT_ADDRESSES() { &&array_begin, &&array_continue, &&error, &&finish, &&object_begin, &&object_continue }
#define GOTO(address) { goto *(address); }
#define CONTINUE(address) { goto *(address); }
#else // SIMDJSON_USE_COMPUTED_GOTO
#define INIT_ADDRESSES() { '[', 'a', 'e', 'f', '{', 'o' };
#define GOTO(address)                 \
  {                                   \
    switch(address) {                 \
      case '[': goto array_begin;     \
      case 'a': goto array_continue;  \
      case 'e': goto error;           \
      case 'f': goto finish;          \
      case '{': goto object_begin;    \
      case 'o': goto object_continue; \
    }                                 \
  }
// For the more constrained end_xxx() situation
#define CONTINUE(address)             \
  {                                   \
    switch(address) {                 \
      case 'a': goto array_continue;  \
      case 'o': goto object_continue; \
      case 'f': goto finish;          \
    }                                 \
  }
#endif // SIMDJSON_USE_COMPUTED_GOTO

struct unified_machine_addresses {
  ret_address array_begin;
  ret_address array_continue;
  ret_address error;
  ret_address finish;
  ret_address object_begin;
  ret_address object_continue;
};

#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { return addresses.error; } }

struct number_writer {
  parser &doc_parser;
  
  really_inline void write_s64(int64_t value) noexcept {
    write_tape(0, internal::tape_type::INT64);
    std::memcpy(&doc_parser.doc.tape[doc_parser.current_loc], &value, sizeof(value));
    ++doc_parser.current_loc;
  }
  really_inline void write_u64(uint64_t value) noexcept {
    write_tape(0, internal::tape_type::UINT64);
    doc_parser.doc.tape[doc_parser.current_loc++] = value;
  }
  really_inline void write_double(double value) noexcept {
    write_tape(0, internal::tape_type::DOUBLE);
    static_assert(sizeof(value) == sizeof(doc_parser.doc.tape[doc_parser.current_loc]), "mismatch size");
    memcpy(&doc_parser.doc.tape[doc_parser.current_loc++], &value, sizeof(double));
    // doc.tape[doc.current_loc++] = *((uint64_t *)&d);
  }
  really_inline void write_tape(uint64_t val, internal::tape_type t) noexcept {
    doc_parser.doc.tape[doc_parser.current_loc++] = val | ((uint64_t(char(t))) << 56);
  }
}; // struct number_writer

struct structural_parser {
  structural_iterator structurals;
  parser &doc_parser;
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc{};
  uint32_t depth;

  really_inline structural_parser(
    const uint8_t *buf,
    size_t len,
    parser &_doc_parser,
    uint32_t next_structural = 0
  ) : structurals(buf, len, _doc_parser.structural_indexes.get(), next_structural), doc_parser{_doc_parser}, depth{0} {}

  WARN_UNUSED really_inline bool start_scope(internal::tape_type type, ret_address continue_state) {
    doc_parser.containing_scope[depth].tape_index = doc_parser.current_loc;
    doc_parser.containing_scope[depth].count = 0;
    write_tape(0, type); // if the document is correct, this gets rewritten later
    doc_parser.ret_address[depth] = continue_state;
    depth++;
    return depth >= doc_parser.max_depth();
  }

  WARN_UNUSED really_inline bool start_document(ret_address continue_state) {
    return start_scope(internal::tape_type::ROOT, continue_state);
  }

  WARN_UNUSED really_inline bool start_object(ret_address continue_state) {
    return start_scope(internal::tape_type::START_OBJECT, continue_state);
  }

  WARN_UNUSED really_inline bool start_array(ret_address continue_state) {
    return start_scope(internal::tape_type::START_ARRAY, continue_state);
  }

  // this function is responsible for annotating the start of the scope
  really_inline void end_scope(internal::tape_type type) noexcept {
    depth--;
    // write our doc.tape location to the header scope
    // The root scope gets written *at* the previous location.
    write_tape(doc_parser.containing_scope[depth].tape_index, type);
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t start_tape_index = doc_parser.containing_scope[depth].tape_index;
    const uint32_t count = doc_parser.containing_scope[depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    // This is a load and an OR. It would be possible to just write once at doc.tape[d.tape_index]
    doc_parser.doc.tape[start_tape_index] |= doc_parser.current_loc | (uint64_t(cntsat) << 32);
  }

  really_inline void end_object() {
    end_scope(internal::tape_type::END_OBJECT);
  }
  really_inline void end_array() {
    end_scope(internal::tape_type::END_ARRAY);
  }
  really_inline void end_document() {
    end_scope(internal::tape_type::ROOT);
  }

  really_inline void write_tape(uint64_t val, internal::tape_type t) noexcept {
    doc_parser.doc.tape[doc_parser.current_loc++] = val | ((uint64_t(char(t))) << 56);
  }

  // increment_count increments the count of keys in an object or values in an array.
  // Note that if you are at the level of the values or elements, the count
  // must be increment in the preceding depth (depth-1) where the array or
  // the object resides.
  really_inline void increment_count() {
    doc_parser.containing_scope[depth - 1].count++; // we have a key value pair in the object at parser.depth - 1
  }

  really_inline uint8_t *on_start_string() noexcept {
    /* we advance the point, accounting for the fact that we have a NULL
      * termination         */
    write_tape(current_string_buf_loc - doc_parser.doc.string_buf.get(), internal::tape_type::STRING);
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline bool on_end_string(uint8_t *dst) noexcept {
    uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
    return true;
  }

  WARN_UNUSED really_inline bool parse_string() {
    uint8_t *dst = on_start_string();
    dst = stringparsing::parse_string(structurals.current(), dst);
    if (dst == nullptr) {
      return true;
    }
    return !on_end_string(dst);
  }

  WARN_UNUSED really_inline bool parse_number(const uint8_t *src, bool found_minus) {
    number_writer writer{doc_parser};
    return !numberparsing::parse_number(src, found_minus, writer);
  }
  WARN_UNUSED really_inline bool parse_number(bool found_minus) {
    return parse_number(structurals.current(), found_minus);
  }

  WARN_UNUSED really_inline bool parse_atom() {
    switch (structurals.current_char()) {
      case 't':
        if (!atomparsing::is_valid_true_atom(structurals.current())) { return true; }
        write_tape(0, internal::tape_type::TRUE_VALUE);
        break;
      case 'f':
        if (!atomparsing::is_valid_false_atom(structurals.current())) { return true; }
        write_tape(0, internal::tape_type::FALSE_VALUE);
        break;
      case 'n':
        if (!atomparsing::is_valid_null_atom(structurals.current())) { return true; }
        write_tape(0, internal::tape_type::NULL_VALUE);
        break;
      default:
        return true;
    }
    return false;
  }

  WARN_UNUSED really_inline bool parse_single_atom() {
    switch (structurals.current_char()) {
      case 't':
        if (!atomparsing::is_valid_true_atom(structurals.current(), structurals.remaining_len())) { return true; }
        write_tape(0, internal::tape_type::TRUE_VALUE);
        break;
      case 'f':
        if (!atomparsing::is_valid_false_atom(structurals.current(), structurals.remaining_len())) { return true; }
        write_tape(0, internal::tape_type::FALSE_VALUE);
        break;
      case 'n':
        if (!atomparsing::is_valid_null_atom(structurals.current(), structurals.remaining_len())) { return true; }
        write_tape(0, internal::tape_type::NULL_VALUE);
        break;
      default:
        return true;
    }
    return false;
  }

  WARN_UNUSED really_inline ret_address parse_value(const unified_machine_addresses &addresses, ret_address continue_state) {
    switch (structurals.current_char()) {
    case '"':
      FAIL_IF( parse_string() );
      return continue_state;
    case 't': case 'f': case 'n':
      FAIL_IF( parse_atom() );
      return continue_state;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      FAIL_IF( parse_number(false) );
      return continue_state;
    case '-':
      FAIL_IF( parse_number(true) );
      return continue_state;
    case '{':
      FAIL_IF( start_object(continue_state) );
      return addresses.object_begin;
    case '[':
      FAIL_IF( start_array(continue_state) );
      return addresses.array_begin;
    default:
      return addresses.error;
    }
  }

  WARN_UNUSED really_inline error_code finish() {
    // the string might not be NULL terminated.
    if ( !structurals.at_end(doc_parser.n_structural_indexes) ) {
      return on_error(TAPE_ERROR);
    }
    end_document();
    if (depth != 0) {
      return on_error(TAPE_ERROR);
    }
    if (doc_parser.containing_scope[depth].tape_index != 0) {
      return on_error(TAPE_ERROR);
    }

    return on_success(SUCCESS);
  }

  really_inline error_code on_error(error_code new_error_code) noexcept {
    doc_parser.error = new_error_code;
    return new_error_code;
  }
  really_inline error_code on_success(error_code success_code) noexcept {
    doc_parser.error = success_code;
    doc_parser.valid = true;
    return success_code;
  }

  WARN_UNUSED really_inline error_code error() {
    /* We do not need the next line because this is done by doc_parser.init_stage2(),
    * pessimistically.
    * doc_parser.is_valid  = false;
    * At this point in the code, we have all the time in the world.
    * Note that we know exactly where we are in the document so we could,
    * without any overhead on the processing code, report a specific
    * location.
    * We could even trigger special code paths to assess what happened
    * carefully,
    * all without any added cost. */
    if (depth >= doc_parser.max_depth()) {
      return on_error(DEPTH_ERROR);
    }
    switch (structurals.current_char()) {
    case '"':
      return on_error(STRING_ERROR);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      return on_error(NUMBER_ERROR);
    case 't':
      return on_error(T_ATOM_ERROR);
    case 'n':
      return on_error(N_ATOM_ERROR);
    case 'f':
      return on_error(F_ATOM_ERROR);
    default:
      return on_error(TAPE_ERROR);
    }
  }

  really_inline void init() {
    current_string_buf_loc = doc_parser.doc.string_buf.get();
    doc_parser.current_loc = 0;
    doc_parser.valid = false;
    doc_parser.error = UNINITIALIZED;
  }

  WARN_UNUSED really_inline error_code start(size_t len, ret_address finish_state) {
    init(); // sets is_valid to false
    if (len > doc_parser.capacity()) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    structurals.advance_char();
    // Push the root scope (there is always at least one scope)
    if (start_document(finish_state)) {
      return on_error(DEPTH_ERROR);
    }
    return SUCCESS;
  }

  really_inline char advance_char() {
    return structurals.advance_char();
  }
};

// Redefine FAIL_IF to use goto since it'll be used inside the function now
#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { goto error; } }

} // namespace stage2

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED error_code implementation::stage2(const uint8_t *buf, size_t len, parser &doc_parser) const noexcept {
  static constexpr stage2::unified_machine_addresses addresses = INIT_ADDRESSES();
  stage2::structural_parser parser(buf, len, doc_parser);
  error_code result = parser.start(len, addresses.finish);
  if (result) { return result; }

  //
  // Read first value
  //
  switch (parser.structurals.current_char()) {
  case '{':
    FAIL_IF( parser.start_object(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.start_array(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF( parser.parse_single_atom() );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.structurals.with_space_terminated_copy([&](const uint8_t *copy, size_t idx) {
        return parser.parse_number(&copy[idx], false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.structurals.with_space_terminated_copy([&](const uint8_t *copy, size_t idx) {
        return parser.parse_number(&copy[idx], true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser states
//
object_begin:
  switch (parser.advance_char()) {
  case '"': {
    parser.increment_count();
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  }
  case '}':
    parser.end_object();
    goto scope_end;
  default:
    goto error;
  }

object_key_state:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.increment_count();
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  case '}':
    parser.end_object();
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.doc_parser.ret_address[parser.depth] );

//
// Array parser states
//
array_begin:
  if (parser.advance_char() == ']') {
    parser.end_array();
    goto scope_end;
  }
  parser.increment_count();

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.increment_count();
    parser.advance_char();
    goto main_array_switch;
  case ']':
    parser.end_array();
    goto scope_end;
  default:
    goto error;
  }

finish:
  return parser.finish();

error:
  return parser.error();
}

WARN_UNUSED error_code implementation::parse(const uint8_t *buf, size_t len, parser &doc_parser) const noexcept {
  error_code code = stage1(buf, len, doc_parser, false);
  if (!code) {
    code = stage2(buf, len, doc_parser);
  }
  return code;
}
