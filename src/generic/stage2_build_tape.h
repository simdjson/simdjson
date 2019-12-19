// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

struct block_position {
  const uint8_t* block;
  int offset;

  really_inline operator const uint8_t*() const { return &block[offset]; }
};

struct bitmask_block_iterator {
  static const size_t BLOCK_SIZE = 64;

  uint64_t* next_bitmask;
  const uint8_t* next_block;

  really_inline bitmask_block_iterator(const uint8_t* buf, uint64_t* bitmask_blocks) :
    next_bitmask{bitmask_blocks},
    next_block{buf}
  {
  }

  really_inline bool at_eof() const {
    return *next_bitmask == 0;
  }

  really_inline block_position peek() const {
    return {
      next_block,
      trailing_zeroes(*next_bitmask)
    };
  }

  really_inline block_position next() {
    block_position current = peek();
    *next_bitmask = clear_lowest_bit(*next_bitmask);

    // If there are no more structurals in this block, advance to the next
    bool is_last_bit = *next_bitmask == 0;
    next_block += is_last_bit*BLOCK_SIZE;
    next_bitmask += is_last_bit;
    return current;
  }
};

#define PARSE_STRING()                                \
  {                                                   \
    int new_offset = parse_string(current.block, current.offset, pj); \
    if (unlikely(new_offset == 0)) {                  \
      goto fail;                                      \
    }                                                 \
    /* If it completely skipped any blocks, advance block */ \
    int skipped = new_offset - 2*bitmask_block_iterator::BLOCK_SIZE + 1; \
    skipped = skipped < 0 ? 0 : skipped; \
    structurals.next_block += ROUNDUP_N(skipped, bitmask_block_iterator::BLOCK_SIZE); \
  }
#define RETRY_SPACES_CASE(RETRY) \
  case ' ': \
  case '\n': \
  case '\t': \
  case '\r': \
    current.block += bitmask_block_iterator::BLOCK_SIZE; \
    structurals.next_block += bitmask_block_iterator::BLOCK_SIZE; \
    goto RETRY; \

#define RETRY_SPACES(RETRY) \
  switch (*current) { \
    case ' ': \
    case '\t': \
    case '\n': \
    case '\r': \
      current.block += bitmask_block_iterator::BLOCK_SIZE; \
      structurals.next_block += bitmask_block_iterator::BLOCK_SIZE; \
      goto RETRY; \
  }

#ifdef SIMDJSON_USE_COMPUTED_GOTO
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = &&array_continue;
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = &&object_continue;
#define SET_GOTO_START_CONTINUE() pj.ret_address[depth] = &&start_continue;
#define GOTO_CONTINUE() goto *pj.ret_address[depth];
#else
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = 'a';
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = 'o';
#define SET_GOTO_START_CONTINUE() pj.ret_address[depth] = 's';
#define GOTO_CONTINUE()                                                        \
  {                                                                            \
    if (pj.ret_address[depth] == 'a') {                                        \
      goto array_continue;                                                     \
    } else if (pj.ret_address[depth] == 'o') {                                 \
      goto object_continue;                                                    \
    } else {                                                                   \
      goto start_continue;                                                     \
    }                                                                          \
  }
#endif

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  pj.init();          // sets is_valid to false
  if (pj.byte_capacity < len) {
    pj.error_code = CAPACITY;
    return pj.error_code;
  }

  bitmask_block_iterator structurals(buf, pj.structural_blocks);
  block_position current = structurals.next();
  uint32_t depth = 0;

  /*//////////////////////////// START STATE /////////////////////////////
   */
  SET_GOTO_START_CONTINUE();
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
  /* the root is used, if nothing else, to capture the size of the tape */
  depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
  if (depth >= pj.depth_capacity) {
    goto fail;
  }

start_retry:
  switch (*current) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, *current); /* strangely, moving this to object_begin slows things down */
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, *current);
    goto array_begin;
    /* #define SIMDJSON_ALLOWANYTHINGINROOT
     * A JSON text is a serialized value.  Note that certain previous
     * specifications of JSON constrained a JSON text to be an object or an
     * array.  Implementations that generate only objects or arrays where a
     * JSON text is called for will be interoperable in the sense that all
     * implementations will accept these as conforming JSON texts.
     * https://tools.ietf.org/html/rfc8259
     * #ifdef SIMDJSON_ALLOWANYTHINGINROOT */
  case '"': {
    PARSE_STRING();
    break;
  }
  case 't': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the true value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', sizeof(uint64_t));
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + (current-buf))) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, *current);
    break;
  }
  case 'f': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the false
     * value.
     * this will almost never be called in practice */
    
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', sizeof(uint64_t));
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + (current-buf))) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, *current);
    break;
  }
  case 'n': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the null value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', sizeof(uint64_t));
    if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) + (current-buf))) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, *current);
    break;
  }
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice. We terminate with a
     * space
     * because we do not want to allow NULLs in the middle of a number
     * (whereas a
     * space in the middle of a number would be identified in stage 1). */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, (current-buf), false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    /* we need to make a copy to make sure that the string is NULL
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, (current-buf), true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  default:
    RETRY_SPACES(start_retry);
    goto fail;
  }
start_continue:
  // Validate that this is the last structural
  if (structurals.at_eof()) {
    goto succeed;
  } else {
    goto fail;
  }
  /*//////////////////////////// OBJECT STATES ///////////////////////////*/

// Beginning of object: } or "string":<value> next
object_begin:
   current = structurals.next();
object_begin_retry:
  switch (*current) {
  case '"': {
    PARSE_STRING();
    goto object_key_state;
  }
  case '}':
    goto scope_end; /* could also go to object_continue */
  default:
    RETRY_SPACES(object_begin_retry);
    goto fail;
  }

// After object key: : next
object_key_state:
  current = structurals.next();
object_key_state_retry:
  if (unlikely(*current != ':')) {
    RETRY_SPACES(object_key_state_retry);
    goto fail;
  }

  // After object key and : <value> next
  current = structurals.next();
object_value_retry:
  switch (*current) {
  case '"': {
    PARSE_STRING();
    break;
  }
  case 't':
    if (!is_valid_true_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break;
  case 'f':
    if (!is_valid_false_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break;
  case 'n':
    if (!is_valid_null_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(current.block, pj, current.offset, false)) {
      goto fail;
    }
    break;
  }
  case '-': {
    if (!parse_number(current.block, pj, current.offset, true)) {
      goto fail;
    }
    break;
  }
  case '{': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, *current); /* here the compilers knows what structurals is so this gets optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an object inside an object, so we need to increment the
     * depth                                                             */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, *current); /* here the compilers knows what structurals is so this gets optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an array inside an object, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    RETRY_SPACES(object_value_retry);
    goto fail;
  }

// After object key/value pair: , or } next
object_continue:
  current = structurals.next();
object_continue_retry:
  switch (*current) {
  case ',':
    goto object_next_key;
  case '}':
    goto scope_end;
  default:
    RETRY_SPACES(object_continue_retry);
    goto fail;
  }

// After ,: "string":<value> next
object_next_key:
  current = structurals.next();
object_next_key_retry:
  if (unlikely(*current != '"')) {
    RETRY_SPACES(object_next_key_retry);
    goto fail;
  }
  PARSE_STRING();
  goto object_key_state;

  /*//////////////////////////// COMMON STATE ///////////////////////////*/

scope_end:
  /* write our tape location to the header scope */
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], *current);
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  /* goto saved_state */
  GOTO_CONTINUE()

  /*//////////////////////////// ARRAY STATES ///////////////////////////*/
array_begin:
  current = structurals.next();
array_begin_retry:
  if (*current == ']') {
    goto scope_end;
  }
  RETRY_SPACES(array_begin_retry);
  // Fall into main_array_switch if we get here.

main_array_switch:
  /* we call update char on all paths in, so we can peek at current on the
   * on paths that can accept a close square brace (post-, and at start) */
  switch (*current) {
  case '"': {
    PARSE_STRING();
    break;
  }
  case 't':
    if (!is_valid_true_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break;
  case 'f':
    if (!is_valid_false_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break;
  case 'n':
    if (!is_valid_null_atom(current)) {
      goto fail;
    }
    pj.write_tape(0, *current);
    break; /* goto array_continue; */

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(current.block, pj, current.offset, false)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '-': {
    if (!parse_number(current.block, pj, current.offset, true)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '{': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, *current); /* here the compilers knows what structurals is so this gets optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an object inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, *current); /* here the compilers knows what structurals is so this gets optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an array inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    RETRY_SPACES(main_array_switch);
    goto fail;
  }

array_continue:
  current = structurals.next();
array_continue_retry:
  switch (*current) {
  case ',':
    current = structurals.next();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    RETRY_SPACES(array_continue_retry);
    goto fail;
  }

  /*//////////////////////////// FINAL STATES ///////////////////////////*/

succeed:
  depth--;
  if (depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if (pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */

  pj.valid = true;
  pj.error_code = simdjson::SUCCESS;
  return pj.error_code;
fail:
  /* we do not need the next line because this is done by pj.init(),
   * pessimistically.
   * pj.is_valid  = false;
   * At this point in the code, we have all the time in the world.
   * Note that we know exactly where we are in the document so we could,
   * without any overhead on the processing code, report a specific
   * location.
   * We could even trigger special code paths to assess what happened
   * carefully,
   * all without any added cost. */
  if (depth >= pj.depth_capacity) {
    pj.error_code = simdjson::DEPTH_ERROR;
    return pj.error_code;
  }
  switch (*current) {
  case '"':
    pj.error_code = simdjson::STRING_ERROR;
    return pj.error_code;
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
    pj.error_code = simdjson::NUMBER_ERROR;
    return pj.error_code;
  case 't':
    pj.error_code = simdjson::T_ATOM_ERROR;
    return pj.error_code;
  case 'n':
    pj.error_code = simdjson::N_ATOM_ERROR;
    return pj.error_code;
  case 'f':
    pj.error_code = simdjson::F_ATOM_ERROR;
    return pj.error_code;
  default:
    break;
  }
  pj.error_code = simdjson::TAPE_ERROR;
  return pj.error_code;
}
