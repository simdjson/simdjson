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
    case '}': SIMDJSON_TRY( visitor.empty_object() ); goto generic_next;
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
    case '}': SIMDJSON_TRY( visitor.end_object() ); goto generic_next;
    default:  return visitor.error(TAPE_ERROR, "No comma between object fields");
  }

generic_array_begin:
  switch (*(value = advance())) {
    case ']': SIMDJSON_TRY( visitor.empty_array() ); goto generic_next;
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
    case ']': SIMDJSON_TRY( visitor.end_array() ); goto generic_next;
    default:  return visitor.error(TAPE_ERROR, "Missing comma between fields");
  }

//
// After an object or array ends, we don't know whether the parent is an array or an object.
// We figure that out by looking at what comes next.
//
// , "key": - object
// , "key", - array
// , "key"] - array
// , <value> - array
// ]
// }
//
generic_next:
  switch (*advance()) {
  case ',':
    // The next thing after the comma is either a key or a value.
    switch (*(value = advance())) {
      // If it's a string, it might be a key ...
      case '"':
        switch (*advance()) {
          // "key": ... -> object
          // "value", ... -> array with string value
          // "value"] -> end of array with string value
          case ':': SIMDJSON_TRY( visitor.try_resume_object() ); goto object_value;
          case ',': SIMDJSON_TRY( visitor.try_resume_array(value) ); goto array_value;
          case ']': SIMDJSON_TRY( visitor.try_end_array(value) ); goto generic_next;
          default: return visitor.error(TAPE_ERROR, "Missing comma or colon between values");
        }
      // , [ ... -> array with array value
      // , { ... -> array with object value
      // , <value> -> array with primitive value
      case '[': SIMDJSON_TRY( visitor.try_resume_array() ); goto generic_array_begin;
      case '{': SIMDJSON_TRY( visitor.try_resume_array() ); goto generic_object_begin;
      default: SIMDJSON_TRY( visitor.try_resume_array() ); goto array_value;
    }
  // ] -> end array, still unsure what comes next
  // } -> end object, still unsure what comes next
  case ']': SIMDJSON_TRY( visitor.try_end_array() ); goto generic_next; // TODO while loop?
  case '}': SIMDJSON_TRY( visitor.try_end_object() ); goto generic_next; // TODO while loop?
  default:
    // If we just ended an array or object, and don't get ] } or , then we might be at document end.
    // We are guaranteed ] } and , will never be at document end.
    // Since we overcorrected, we back up one element.
    next_structural--;
    goto document_end;
  }

document_end:
  return visitor.end_document();

} // structural_iterator::walk_document()

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
