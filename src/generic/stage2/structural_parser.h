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
  dom_parser_implementation &parser;
  
  really_inline void write_s64(int64_t value) noexcept {
    append_tape(0, internal::tape_type::INT64);
    std::memcpy(&parser.doc->tape[parser.current_loc], &value, sizeof(value));
    ++parser.current_loc;
  }
  really_inline void write_u64(uint64_t value) noexcept {
    append_tape(0, internal::tape_type::UINT64);
    parser.doc->tape[parser.current_loc++] = value;
  }
  really_inline void write_double(double value) noexcept {
    append_tape(0, internal::tape_type::DOUBLE);
    static_assert(sizeof(value) == sizeof(parser.doc->tape[parser.current_loc]), "mismatch size");
    memcpy(&parser.doc->tape[parser.current_loc++], &value, sizeof(double));
    // doc->tape[doc->current_loc++] = *((uint64_t *)&d);
  }
  really_inline void append_tape(uint64_t val, internal::tape_type t) noexcept {
    parser.doc->tape[parser.current_loc++] = val | ((uint64_t(char(t))) << 56);
  }
}; // struct number_writer

struct structural_parser {
  structural_iterator structurals;
  dom_parser_implementation &parser;
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc{};
  uint32_t depth;

  really_inline structural_parser(
    dom_parser_implementation &_parser,
    uint32_t next_structural = 0
  ) : structurals(_parser.buf, _parser.len, _parser.structural_indexes.get(), next_structural), parser{_parser}, depth{0} {}

  WARN_UNUSED really_inline bool start_scope(ret_address continue_state) {
    parser.containing_scope[depth].tape_index = parser.current_loc;
    parser.containing_scope[depth].count = 0;
    parser.current_loc++; // We don't actually *write* the start element until the end.
    parser.ret_address[depth] = continue_state;
    depth++;
    bool exceeded_max_depth = depth >= parser.max_depth();
    if (exceeded_max_depth) { log_error("Exceeded max depth!"); }
    return exceeded_max_depth;
  }

  WARN_UNUSED really_inline bool start_document(ret_address continue_state) {
    log_start_value("document");
    return start_scope(continue_state);
  }

  WARN_UNUSED really_inline bool start_object(ret_address continue_state) {
    log_start_value("object");
    return start_scope(continue_state);
  }

  WARN_UNUSED really_inline bool start_array(ret_address continue_state) {
    log_start_value("array");
    return start_scope(continue_state);
  }

  // this function is responsible for annotating the start of the scope
  really_inline void end_scope(internal::tape_type start, internal::tape_type end) noexcept {
    depth--;
    // write our doc->tape location to the header scope
    // The root scope gets written *at* the previous location.
    append_tape(parser.containing_scope[depth].tape_index, end);
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t start_tape_index = parser.containing_scope[depth].tape_index;
    const uint32_t count = parser.containing_scope[depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    // This is a load and an OR. It would be possible to just write once at doc->tape[d.tape_index]
    write_tape(start_tape_index, parser.current_loc | (uint64_t(cntsat) << 32), start);
  }

  really_inline void end_object() {
    log_end_value("object");
    end_scope(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  really_inline void end_array() {
    log_end_value("array");
    end_scope(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }
  really_inline void end_document() {
    log_end_value("document");
    end_scope(internal::tape_type::ROOT, internal::tape_type::ROOT);
  }

  really_inline void append_tape(uint64_t val, internal::tape_type t) noexcept {
    parser.doc->tape[parser.current_loc++] = val | ((uint64_t(char(t))) << 56);
  }

  really_inline void write_tape(uint32_t loc, uint64_t val, internal::tape_type t) noexcept {
    parser.doc->tape[loc] = val | ((uint64_t(char(t))) << 56);
  }

  // increment_count increments the count of keys in an object or values in an array.
  // Note that if you are at the level of the values or elements, the count
  // must be increment in the preceding depth (depth-1) where the array or
  // the object resides.
  really_inline void increment_count() {
    parser.containing_scope[depth - 1].count++; // we have a key value pair in the object at parser.depth - 1
  }

  really_inline uint8_t *on_start_string() noexcept {
    // we advance the point, accounting for the fact that we have a NULL termination
    append_tape(current_string_buf_loc - parser.doc->string_buf.get(), internal::tape_type::STRING);
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline void on_end_string(uint8_t *dst) noexcept {
    uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
  }

  WARN_UNUSED really_inline bool parse_string(bool key = false) {
    log_value(key ? "key" : "string");
    uint8_t *dst = on_start_string();
    dst = stringparsing::parse_string(structurals.current(), dst);
    if (dst == nullptr) {
      log_error("Invalid escape in string");
      return true;
    }
    on_end_string(dst);
    return false;
  }

  WARN_UNUSED really_inline bool parse_number(const uint8_t *src, bool found_minus) {
    log_value("number");
    number_writer writer{parser};
    bool succeeded = numberparsing::parse_number(src, found_minus, writer);
    if (!succeeded) { log_error("Invalid number"); }
    return !succeeded;
  }
  WARN_UNUSED really_inline bool parse_number(bool found_minus) {
    return parse_number(structurals.current(), found_minus);
  }

  WARN_UNUSED really_inline bool parse_atom() {
    switch (structurals.current_char()) {
      case 't':
        log_value("true");
        if (!atomparsing::is_valid_true_atom(structurals.current())) { return true; }
        append_tape(0, internal::tape_type::TRUE_VALUE);
        break;
      case 'f':
        log_value("false");
        if (!atomparsing::is_valid_false_atom(structurals.current())) { return true; }
        append_tape(0, internal::tape_type::FALSE_VALUE);
        break;
      case 'n':
        log_value("null");
        if (!atomparsing::is_valid_null_atom(structurals.current())) { return true; }
        append_tape(0, internal::tape_type::NULL_VALUE);
        break;
      default:
        log_error("IMPOSSIBLE: unrecognized parse_atom structural character");
        return true;
    }
    return false;
  }

  WARN_UNUSED really_inline bool parse_single_atom() {
    switch (structurals.current_char()) {
      case 't':
        log_value("true");
        if (!atomparsing::is_valid_true_atom(structurals.current(), structurals.remaining_len())) { return true; }
        append_tape(0, internal::tape_type::TRUE_VALUE);
        break;
      case 'f':
        log_value("false");
        if (!atomparsing::is_valid_false_atom(structurals.current(), structurals.remaining_len())) { return true; }
        append_tape(0, internal::tape_type::FALSE_VALUE);
        break;
      case 'n':
        log_value("null");
        if (!atomparsing::is_valid_null_atom(structurals.current(), structurals.remaining_len())) { return true; }
        append_tape(0, internal::tape_type::NULL_VALUE);
        break;
      default:
        log_error("IMPOSSIBLE: unrecognized parse_atom structural character");
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
      log_error("Non-value found when value was expected!");
      return addresses.error;
    }
  }

  WARN_UNUSED really_inline error_code finish() {
    // the string might not be NULL terminated.
    if ( !structurals.at_end(parser.n_structural_indexes) ) {
      log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
      return TAPE_ERROR;
    }
    end_document();
    if (depth != 0) {
      log_error("Unclosed objects or arrays!");
      return TAPE_ERROR;
    }
    if (parser.containing_scope[depth].tape_index != 0) {
      log_error("IMPOSSIBLE: root scope tape index did not start at 0!");
      return TAPE_ERROR;
    }

    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code error() {
    /* We do not need the next line because this is done by parser.init_stage2(),
    * pessimistically.
    * parser.is_valid  = false;
    * At this point in the code, we have all the time in the world.
    * Note that we know exactly where we are in the document so we could,
    * without any overhead on the processing code, report a specific
    * location.
    * We could even trigger special code paths to assess what happened
    * carefully,
    * all without any added cost. */
    if (depth >= parser.max_depth()) {
      return DEPTH_ERROR;
    }
    switch (structurals.current_char()) {
    case '"':
      return STRING_ERROR;
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
      return NUMBER_ERROR;
    case 't':
      return T_ATOM_ERROR;
    case 'n':
      return N_ATOM_ERROR;
    case 'f':
      return F_ATOM_ERROR;
    default:
      return TAPE_ERROR;
    }
  }

  really_inline void init() {
    current_string_buf_loc = parser.doc->string_buf.get();
    parser.current_loc = 0;
  }

  WARN_UNUSED really_inline error_code start(size_t len, ret_address finish_state) {
    log_start();
    init(); // sets is_valid to false
    if (len > parser.capacity()) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    structurals.advance_char();
    // Push the root scope (there is always at least one scope)
    if (start_document(finish_state)) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  really_inline char advance_char() {
    return structurals.advance_char();
  }

  really_inline void log_value(const char *type) {
    logger::log_line(structurals, "", type, "");
  }

  static really_inline void log_start() {
    logger::log_start();
  }

  really_inline void log_start_value(const char *type) {
    logger::log_line(structurals, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(structurals, "-", type, "");
  }

  really_inline void log_error(const char *error) {
    logger::log_line(structurals, "", "ERROR", error);
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
WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  this->doc = &_doc;
  static constexpr stage2::unified_machine_addresses addresses = INIT_ADDRESSES();
  stage2::structural_parser parser(*this);
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
    parser.log_error("Document starts with a non-value character");
    goto error;
  }

//
// Object parser states
//
object_begin:
  switch (parser.advance_char()) {
  case '"': {
    parser.increment_count();
    FAIL_IF( parser.parse_string(true) );
    goto object_key_state;
  }
  case '}':
    parser.end_object();
    goto scope_end;
  default:
    parser.log_error("Object does not start with a key");
    goto error;
  }

object_key_state:
  if (parser.advance_char() != ':' ) { parser.log_error("Missing colon after key in object"); goto error; }
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.increment_count();
    if (parser.advance_char() != '"' ) { parser.log_error("Key string missing at beginning of field in object"); goto error; }
    FAIL_IF( parser.parse_string(true) );
    goto object_key_state;
  case '}':
    parser.end_object();
    goto scope_end;
  default:
    parser.log_error("No comma between object fields");
    goto error;
  }

scope_end:
  CONTINUE( parser.parser.ret_address[parser.depth] );

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
    parser.log_error("Missing comma between array values");
    goto error;
  }

finish:
  return parser.finish();

error:
  return parser.error();
}
