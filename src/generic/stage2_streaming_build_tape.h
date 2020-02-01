namespace stage2 {

template<typename JsonVisitor>
struct streaming_structural_parser: structural_parser<JsonVisitor> {
  really_inline streaming_structural_parser(const uint8_t *_buf, size_t _len, JsonVisitor &_visitor, size_t _i) : structural_parser<JsonVisitor>(_buf, _len, _visitor, _i) {}

  // override to add streaming
  WARN_UNUSED really_inline int start(ret_address finish_parser) {
    this->visitor.pj.init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    this->advance_char();
    // Push the root scope (there is always at least one scope)
    if (this->push_start_scope(finish_parser, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline int finish() {
    if ( this->i + 1 > this->visitor.pj.n_structural_indexes ) {
      return this->set_error_code(TAPE_ERROR);
    }
    this->pop_root_scope();
    if (this->depth != 0) {
      return this->set_error_code(TAPE_ERROR);
    }
    if (this->visitor.pj.containing_scope_offset[this->depth] != 0) {
      return this->set_error_code(TAPE_ERROR);
    }
    bool finished = this->i + 1 == this->visitor.pj.n_structural_indexes;
    this->visitor.pj.valid = true;
    return this->set_error_code(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }
};

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  ParsedJsonWriter writer{pj};
  streaming_structural_parser parser(buf, len, writer, next_json);
  int result = parser.start(addresses.finish);
  if (result) { return result; }
  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser parsers
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_parser:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser parsers
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  next_json = parser.i;
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2