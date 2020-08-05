namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class structural_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;

  // Start a structural 
  really_inline structural_iterator(dom_parser_implementation &_dom_parser, uint32_t start_structural_index)
    : buf{_dom_parser.buf},
      next_structural{&_dom_parser.structural_indexes[start_structural_index]},
      dom_parser{_dom_parser} {
  }

  template<typename T>
  WARN_UNUSED really_inline error_code walk_document(T &visitor) noexcept;

  really_inline const uint8_t* advance() {
    return &buf[*(next_structural++)];
  }
  really_inline size_t remaining_len() {
    return dom_parser.len - *(next_structural-1);
  }

  really_inline bool at_end() {
    return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
  }
  really_inline bool at_beginning() {
    return next_structural == dom_parser.structural_indexes.get();
  }

}; // struct structural_iterator

template<typename T>
WARN_UNUSED really_inline error_code structural_iterator::walk_document(T &visitor) noexcept {
  // Variable used when one state reads a JSON value that the next state needs
  const uint8_t *value;

  //
  // Start the document
  //
  if (at_end()) { return visitor.error(EMPTY, "Empty document"); }

  SIMDJSON_TRY( visitor.start_document() );

  //
  // Read first value
  //
  switch (*(value = advance())) {
    case '{': goto generic_object_begin;
    case '[': goto generic_array_begin;
    default: SIMDJSON_TRY( visitor.root_primitive(value) ); goto document_end;
  }

//
// Object parser states
//
generic_object_begin:
  switch (*(value = advance())) {
    case '}': SIMDJSON_TRY( visitor.empty_object() ); goto container_end;
    case '"': SIMDJSON_TRY( visitor.start_object() ); goto object_colon;
    default: return visitor.error(TAPE_ERROR, "First field of object missing key");
  }

object_colon:
  switch (*advance()) {
    case ':': goto object_value;
    default: return visitor.error(TAPE_ERROR, "First field of object missing :");
  }

object_value: {
  // Pick up key from previous state
  const uint8_t *key = value;
  switch (*(value = advance())) {
    case '{':
      switch (*(value = advance())) {
        case '}': SIMDJSON_TRY( visitor.empty_object_field(key) ); goto object_next;
        case '"': SIMDJSON_TRY( visitor.start_object_field(key) ); key = value; goto object_colon;
        default:  return visitor.error(TAPE_ERROR, "First field of object missing key");
      }
    case '[':
      switch (*(value = advance())) {
        case ']': SIMDJSON_TRY( visitor.empty_array_field(key) ); goto object_next;
        default: SIMDJSON_TRY( visitor.start_array_field(key) ); goto array_value;
      }
    default:
      SIMDJSON_TRY( visitor.primitive_field(key, value) ); goto object_next;
  }
} // object_value:

object_next:
  switch (*advance()) {
    case ',':
      switch (*(value = advance())) {
        case '"': goto object_colon;
        default: return visitor.error(TAPE_ERROR, "No key in object field");
      }
    case '}': SIMDJSON_TRY( visitor.end_object() ); goto container_end;
    default:  return visitor.error(TAPE_ERROR, "No comma between object fields");
  }

generic_array_begin:
  switch (*(value = advance())) {
    case ']': SIMDJSON_TRY( visitor.empty_array() ); goto container_end;
    default:  SIMDJSON_TRY( visitor.start_array() ); goto array_value;
  }

array_value:
  // Pick up value from previous state
  switch (*value) {
    case '{':
      switch (*(value = advance())) {
        case '}': SIMDJSON_TRY( visitor.empty_object() ); goto array_next;
        case '"': SIMDJSON_TRY( visitor.start_object() ); goto object_colon;
        default:  return visitor.error(TAPE_ERROR, "First field of object missing key");
      }
    case '[':
      switch (*(value = advance())) {
        case ']': SIMDJSON_TRY( visitor.empty_array() ); goto array_next;
        default:  SIMDJSON_TRY( visitor.start_array() ); goto array_value;
      }
    default: SIMDJSON_TRY( visitor.primitive(value) ); goto array_next;
  }

array_next:
  switch (*advance()) {
    case ',': value = advance(); goto array_value;
    case ']': SIMDJSON_TRY( visitor.end_array() ); goto container_end;
    default:  return visitor.error(TAPE_ERROR, "Missing comma between fields");
  }

//
// After an object or array ends, we don't know whether the parent is an array or an object.
// This state figures that out by looking at what comes next.
//
container_end:
  switch (*advance()) {
    case ',':
      // We have a next element. Check if it's is an object field (, "key" : ....)
      switch (*(value = advance())) {
        case '"':
          // If it's a string, it is 99% certain it's a key (arrays don't generally mix in strings
          // with arrays/objects). If we detect it's not, we just pretend we didn't advance the
          // structural and let array_value handle it.
          switch (*advance()) {
            case ':': SIMDJSON_TRY( visitor.try_resume_object() ); goto object_value;
            default: next_structural--; break;
          }
        default: break;
      }
      SIMDJSON_TRY( visitor.try_resume_array() ); goto array_value;
    case ']': SIMDJSON_TRY( visitor.try_end_array() ); goto container_end;
    case '}': SIMDJSON_TRY( visitor.try_end_object() ); goto container_end;
    // If we just ended an array or object, and don't get ] } or , then we might be at off the end
    // of the document. If so, the structural will be [ or { since we set the n+1'th structural to
    // the value of the first structural.
    default: next_structural--; goto document_end;
  }

document_end:
  return visitor.end_document();

} // structural_iterator::walk_document()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
