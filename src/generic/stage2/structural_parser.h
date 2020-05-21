// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

namespace stage2 {

namespace {

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
  uint32_t depth;

  really_inline structural_parser(parser &_doc_parser, uint32_t _depth=0) : structurals(_doc_parser), depth{_depth} {}

  really_inline parser &doc_parser() {
    return structurals.doc_parser;
  }

  really_inline document &doc() {
    return doc_parser().doc;
  }

  WARN_UNUSED really_inline bool start_scope() {
    bool exceeded_max_depth = depth >= doc_parser().max_depth();
    if (exceeded_max_depth) { log_error("Exceeded max depth!"); return true; }
    doc_parser().current_loc++;
    return false;
  }

  WARN_UNUSED really_inline bool start_document() {
    log_start_value("document");
    return start_scope();
  }

  WARN_UNUSED really_inline bool start_object() {
    log_start_value("object");
    return start_scope();
  }

  WARN_UNUSED really_inline bool start_array() {
    log_start_value("array");
    return start_scope();
  }

  // this function is responsible for annotating the start of the scope
  really_inline void end_scope(internal::tape_type start, internal::tape_type end, uint32_t start_loc, uint32_t count) noexcept {
    // write our doc.tape location to the header scope
    // The root scope gets written *at* the previous location.
    write_tape(start_loc, end);
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    // This is a load and an OR. It would be possible to just write once at doc.tape[d.tape_index]
    doc().tape[start_loc] = doc_parser().current_loc | (uint64_t(cntsat) << 32) | ((uint64_t(char(start))) << 56);
  }

  really_inline void end_object(uint32_t start_loc, uint32_t count) {
    log_end_value("object");
    end_scope(internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT, start_loc, count);
  }
  really_inline void end_array(uint32_t start_loc, uint32_t count) {
    log_end_value("array");
    end_scope(internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY, start_loc, count);
  }
  really_inline void end_document(uint32_t start_loc, uint32_t count) {
    log_end_value("document");
    end_scope(internal::tape_type::ROOT, internal::tape_type::ROOT, start_loc, count);
  }

  really_inline void write_tape(uint64_t val, internal::tape_type t) noexcept {
    doc().tape[doc_parser().current_loc++] = val | ((uint64_t(char(t))) << 56);
  }

  really_inline uint8_t *on_start_string() noexcept {
    // we advance the point, accounting for the fact that we have a NULL termination
    write_tape(doc_parser().current_string_buf_loc - doc().string_buf.get(), internal::tape_type::STRING);
    return doc_parser().current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline void on_end_string(uint8_t *dst) noexcept {
    uint32_t str_length = uint32_t(dst - (doc_parser().current_string_buf_loc + sizeof(uint32_t)));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(doc_parser().current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    doc_parser().current_string_buf_loc = dst + 1;
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
    number_writer writer{doc_parser()};
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
        write_tape(0, internal::tape_type::TRUE_VALUE);
        break;
      case 'f':
        log_value("false");
        if (!atomparsing::is_valid_false_atom(structurals.current())) { return true; }
        write_tape(0, internal::tape_type::FALSE_VALUE);
        break;
      case 'n':
        log_value("null");
        if (!atomparsing::is_valid_null_atom(structurals.current())) { return true; }
        write_tape(0, internal::tape_type::NULL_VALUE);
        break;
      default:
        log_error("IMPOSSIBLE: unrecognized parse_atom structural character");
        return true;
    }
    return false;
  }

  WARN_UNUSED really_inline bool parse_root_atom(size_t len) {
    switch (structurals.current_char()) {
      case 't':
        log_value("true");
        if (!atomparsing::is_valid_true_atom(structurals.current(), remaining_len(len))) { return true; }
        write_tape(0, internal::tape_type::TRUE_VALUE);
        return false;
      case 'f':
        log_value("false");
        if (!atomparsing::is_valid_false_atom(structurals.current(), remaining_len(len))) { return true; }
        write_tape(0, internal::tape_type::FALSE_VALUE);
        return false;
      case 'n':
        log_value("null");
        if (!atomparsing::is_valid_null_atom(structurals.current(), remaining_len(len))) { return true; }
        write_tape(0, internal::tape_type::NULL_VALUE);
        return false;
      default:
        log_error("IMPOSSIBLE: unrecognized parse_atom structural character");
        return true;
    }
  }

  WARN_UNUSED really_inline bool parse_value() {
    switch (structurals.current_char()) {
    case '"':
      return parse_string();
    case 't': case 'f': case 'n':
      return parse_atom();
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return parse_number(false);
    case '-':
      return parse_number(true);
    case '{':
      return parse_object();
    case '[':
      return parse_array();
    default:
      log_error("Non-value found when value was expected!");
      return true;
    }
  }

  WARN_UNUSED really_inline bool parse_root_value(size_t len) {
    // Parse the root value of the document. This is similar to parse_value(), but atoms and numbers
    // in particular get special treatment because those parsers normally rely on being in an object
    // or array (and thus the buffer having at least whitespace ] } or , after them).
    switch (structurals.current_char()) {
    case '{':
      return parse_object();
    case '[':
      return parse_array();
    case '"':
      return parse_string();
    case 't': case 'f': case 'n':
      return parse_root_atom(len);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return with_space_terminated_copy(len, [&](const uint8_t *copy, size_t idx) {
               return parse_number(&copy[idx], false);
             });
    case '-':
      return with_space_terminated_copy(len, [&](const uint8_t *copy, size_t idx) {
               return parse_number(&copy[idx], true);
             });
    default:
      log_error("Document starts with a non-value character");
      return true;
    }
  }

  WARN_UNUSED really_inline bool parse_object() {
    return parse_object(doc_parser(), depth+1);
  }

  WARN_UNUSED static bool parse_object(parser &doc_parser, uint32_t depth) {
    structural_parser parser(doc_parser, depth);
    return parser.parse_object_inline();
  }

  WARN_UNUSED really_inline bool parse_object_inline() {
    uint32_t start_loc = doc_parser().current_loc;
    if (start_object()) { return true; }
    switch (advance_char()) {
    case '"':
      {
        uint32_t count = 1;
        // Key
        if (parse_string(true)) { return true; }
        while (true) {

          // :
          if (advance_char() != ':' ) { log_error("Missing colon after key in object"); return true; }

          // Value
          advance_char();
          if (parse_value()) { return true; }

          switch (advance_char()) {
          case ',':
            count++;
            if (advance_char() != '"' ) { log_error("Key string missing at beginning of field in object"); return true; }
            if (parse_string(true)) { return true; }
            continue;
          case '}':
            end_object(start_loc, count);
            return false;
          default:
            log_error("No comma between object fields");
            return true;
          }
        }
      }
      break;
    case '}':
      end_object(start_loc, 0);
      return false;
    default:
      log_error("Object does not start with a key");
      return true;
    }
  }

  WARN_UNUSED really_inline bool parse_array() {
    return parse_array(doc_parser(), depth+1);
  }

  WARN_UNUSED static bool parse_array(parser &doc_parser, uint32_t depth) {
    structural_parser parser(doc_parser, depth);
    return parser.parse_array_inline();
  }

  WARN_UNUSED really_inline bool parse_array_inline() {
    uint32_t start_loc = doc_parser().current_loc;
    if (start_array()) { return true; }

    if (advance_char() == ']') {
      end_array(start_loc, 0);
      return false;
    }

    uint32_t count = 1;
    while (true) {
      if (parse_value()) { return true; }

      switch (advance_char()) {
      case ',':
        count++;
        advance_char();
        continue;
      case ']':
        end_array(start_loc, count);
        return false;
      default:
        log_error("Missing comma between array values");
        return true;
      }
    }
  }

  WARN_UNUSED really_inline error_code finish() {
    // the string might not be NULL terminated.
    if ( !structurals.at_end() ) {
      log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
      return on_error(TAPE_ERROR);
    }
    end_document(0, 1);

    return on_success(SUCCESS);
  }

  really_inline error_code on_error(error_code new_error_code) noexcept {
    doc_parser().error = new_error_code;
    return new_error_code;
  }
  really_inline error_code on_success(error_code success_code) noexcept {
    doc_parser().error = success_code;
    doc_parser().valid = true;
    return success_code;
  }

  WARN_UNUSED really_inline error_code error() {
    /* We do not need the next line because this is done by doc_parser().init_stage2(),
    * pessimistically.
    * doc_parser().is_valid  = false;
    * At this point in the code, we have all the time in the world.
    * Note that we know exactly where we are in the document so we could,
    * without any overhead on the processing code, report a specific
    * location.
    * We could even trigger special code paths to assess what happened
    * carefully,
    * all without any added cost. */
    if (depth >= doc_parser().max_depth()) {
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
    doc_parser().current_string_buf_loc = doc().string_buf.get();
    doc_parser().current_loc = 0;
    doc_parser().valid = false;
    doc_parser().error = UNINITIALIZED;
  }

  WARN_UNUSED really_inline error_code start(size_t len) {
    log_start();
    init(); // sets is_valid to false
    if (len > doc_parser().capacity()) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    structurals.advance_char();
    // Push the root scope (there is always at least one scope)
    if (start_document()) {
      return on_error(DEPTH_ERROR);
    }
    return SUCCESS;
  }

  really_inline char advance_char() {
    return structurals.advance_char();
  }

  really_inline size_t remaining_len(size_t len) {
    return len - structurals.idx;
  }

  template<typename F>
  really_inline bool with_space_terminated_copy(size_t len, const F& f) {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return true;
    }
    memcpy(copy, doc_parser().parsing_buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    bool result = f(reinterpret_cast<const uint8_t*>(copy), structurals.idx);
    free(copy);
    return result;
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

struct streaming_structural_parser: structural_parser {
  really_inline streaming_structural_parser(parser &_doc_parser) : structural_parser(_doc_parser) {}

  // override to add streaming
  WARN_UNUSED really_inline error_code start(UNUSED size_t len) {
    log_start();
    init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (start_document()) {
      return on_error(DEPTH_ERROR);
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline error_code finish() {
    if ( structurals.past_end() ) {
      log_error("IMPOSSIBLE: past the end of the JSON!");
      return on_error(TAPE_ERROR);
    }
    end_document(0, 1);
    if (depth != 0) {
      log_error("Unclosed objects or arrays!");
      return on_error(TAPE_ERROR);
    }
    bool finished = structurals.at_end();
    if (!finished) { log_value("(and has more)"); }
    return on_success(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }
}; // struct streaming_structural_parser

} // namespace {}

} // namespace stage2

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED error_code implementation::stage2(const uint8_t *buf, size_t len, parser &doc_parser) const noexcept {
  doc_parser.parsing_buf = buf;
  doc_parser.next_structural = 0;
  stage2::structural_parser parser(doc_parser);
  error_code result = parser.start(len);
  if (result) { return result; }

  if (parser.parse_root_value(len)) {
    return parser.error();
  }
  return parser.finish();
}

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED error_code implementation::stage2(const uint8_t *buf, size_t len, parser &doc_parser, size_t &next_json) const noexcept {
  doc_parser.parsing_buf = buf;
  doc_parser.next_structural = next_json;
  stage2::streaming_structural_parser parser(doc_parser);
  error_code result = parser.start(len);
  if (result) { return result; }

  if (parser.parse_root_value(len)) {
    return parser.error();
  }
  next_json = doc_parser.next_structural;
  return parser.finish();
}

WARN_UNUSED error_code implementation::parse(const uint8_t *buf, size_t len, parser &doc_parser) const noexcept {
  error_code code = stage1(buf, len, doc_parser, false);
  if (!code) {
    code = stage2(buf, len, doc_parser);
  }
  return code;
}
