// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR()                                                          \
  {                                                                            \
    idx = pj.structural_indexes[i++];                                          \
    c = buf[idx];                                                              \
  }

// Equivalent to UPDATE_CHAR() and then a comparison with c, but does not
// update the idx variable: this is useful for the ':' structural character
// since its location is irrelevant. Not having to update the idx variable
// gives the compiler extra freedom which may translate into better performance.
#define CHECK_CHAR(validchar)                                                  \
  {                                                                            \
    if((buf[pj.structural_indexes[i++]] != validchar)) goto fail;                \
  }
#ifdef SIMDJSON_USE_COMPUTED_GOTO
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = &&array_continue;
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = &&object_continue;
#define GOTO_CONTINUE() goto *pj.ret_address[depth];
#else
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = true;
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = false;
#define GOTO_CONTINUE()                                                        \
  {                                                                            \
    if (pj.ret_address[depth]) {                                               \
      goto array_continue;                                                     \
    } else  {                                                                  \
      goto object_continue;                                                    \
    }                                                                          \
  }
#endif


enum {
  check_index_end_true = true,
  check_index_end_false = false
};

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/

typedef struct {
  uint32_t current_depth;
  uint32_t current_index;
} stage2_status;

 /**
 * Unified_machine_init is meant to be always called at least one.
 * It sets up the state of the stage 2 parser. If there is a single
 * atom in the JSON document, the parsing will complete.
 * If an object or an array if found, then it will return  simdjson::SUCCESS_AND_HAS_MORE.
 */
WARN_UNUSED  int
unified_machine_init(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status& s) {
  uint32_t depth = 0;
  pj.init();          /* sets is_valid to false          */
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }
  uint32_t i = 0;
  uint32_t idx; 
  uint8_t c;
  /*//////////////////////////// START STATE /////////////////////////////
   */
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
  /* the root is used, if nothing else, to capture the size of the tape */
  depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
  s.current_depth = depth;
  s.current_index = i;
  if (depth >= pj.depth_capacity) {
    goto fail;
  }
  // we are now going to check whether we have a single atom
  UPDATE_CHAR()
  switch (c) {
  case '{':
  case '[':
    return simdjson::SUCCESS_AND_HAS_MORE; // we are in the general case
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
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
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
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
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
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
    if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
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
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,
                      false)) {
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
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  default:
    goto fail;
  }
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
  if (depth >= pj.depth_capacity) {
    pj.error_code = simdjson::DEPTH_ERROR;
    return pj.error_code;
  }
  switch (c) {
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



/************
 * The unified_machine_continue function should be called when we are looking at the
 * start of an object or array ('[' or '{') as indicated by index.
 * Meanwhile when CHECK_INDEX_END is set to true, index_end should point either at the 
 * location of the start of an array or object
 * ('[' or '{') that we do not want to parse or at an index beyond the last structural index.
 * If the parsing does not terminate with success or failure, it will return 
 * simdjson::SUCCESS_AND_HAS_MORE and you need to call unified_machine_continue again.
 ***********/
 template <bool CHECK_INDEX_END>
WARN_UNUSED  int
unified_machine_continue(const uint8_t *buf, size_t len, ParsedJson &pj, stage2_status &s, uint32_t index_end) {
  uint32_t idx; /* location of the structural character in the input (buf)   */
  uint8_t c;    /* used to track the (structural) character we are looking at,
                   updated */
  uint32_t depth = s.current_depth;
  uint32_t i = s.current_index;/* index of the structural character (0,1,2,3...) */
  UPDATE_CHAR();
  // next switch case is executed once.
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(
        0, c); /* strangely, moving this to object_begin slows things down */
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
  default:
    goto fail;
  }
  /*//////////////////////////// OBJECT STATES ///////////////////////////*/

object_begin:
  UPDATE_CHAR();
  if(unlikely(c == '}')) {
    goto scope_end;
  }
  if(unlikely(c != '"')) {
    goto fail;
  }
  if((!parse_string(buf, len, pj, depth, idx))) {
    goto fail;
  }


object_key_state:
  CHECK_CHAR(':'); // we don't need to know where the ':' is
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
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
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break;
  }
  case '{': {
    if(CHECK_INDEX_END) {
      if(i > index_end) goto success_and_more;
    }
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an object inside an object, so we need to increment the
     * depth                                                             */
    depth++;
    if(depth < pj.depth_capacity) {
      goto object_begin;
    }
    goto fail;
  }
  case '[': {
    if(CHECK_INDEX_END) {
      if(i > index_end) goto success_and_more;
    }
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an array inside an object, so we need to increment the depth
     */
    depth++;
    if(depth < pj.depth_capacity) {
      goto array_begin;
    }
    goto fail;
  }
  default:
    goto fail;
  }

object_continue:
  UPDATE_CHAR();
  // Next bit could be a switch case. Short switch cases tend to be compiled
  // as sequences of branches. The compiler can't tell which branch is more likely
  // but we *know* that most objects contain more than one entry so that ',' is a common result,
  // hence we build our own sequence of branches, starting with the most likely result.
  if( c == ',' ) {// common case
    UPDATE_CHAR();
    if( c == '"' ) { // common case
      if (parse_string(buf, len, pj, depth, idx)) { // common case
        goto object_key_state;
      }
    }
  } else if (c == '}') {
    goto scope_end;
  } 
  goto fail;

  /*//////////////////////////// COMMON STATE ///////////////////////////*/



scope_end:
  /* write our tape location to the header scope */
  depth--;

  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  // The GOTO_CONTINUE is guarded. Instead we could have a "goto" at depth 1 that
  // goes straight to the else clause, thus saving a branch. It does not
  // appear to be obviously profitable to do so.
  if(depth > 1) {
    GOTO_CONTINUE()
  } else {
    if (i + 1 == pj.n_structural_indexes) {
      goto succeed;
    } else {
      goto fail;
    }
  }
  /*//////////////////////////// ARRAY STATES ///////////////////////////*/
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; /* could also go to array_continue */
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at c on the
   * on paths that can accept a close square brace (post-, and at start) */
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
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
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '{': {
    if(CHECK_INDEX_END) {
      if(i > index_end) goto success_and_more;
    }
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an object inside an array, so we need to increment the depth
     */
    depth++;
    if(depth < pj.depth_capacity) {
      goto object_begin;
    }
    goto fail;
  }
  case '[': {
    if(CHECK_INDEX_END) {
      if(i > index_end) goto success_and_more;
    }
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an array inside an array, so we need to increment the depth
     */
    depth++;
    if (depth < pj.depth_capacity) {
      goto array_begin;
    }
    goto fail; 
    
  }
  default:
    goto fail;
  }

array_continue:
  UPDATE_CHAR();
  // Next bit could be a switch case. Short switch cases tend to be compiled
  // as sequences of branches. The compiler can't tell which branch is more likely
  // but we *know* that most arrays contain more than one entry so that ',' is a common result,
  // hence we build our own sequence of branches, starting with the most likely result.
  if(c == ',') {
    UPDATE_CHAR();
    goto main_array_switch;
  } else if(c == ']') {
    goto scope_end;
  } else {
    goto fail;
  }

  /*//////////////////////////// FINAL STATES ///////////////////////////*/
success_and_more:
  s.current_depth = depth;
  s.current_index = i - 1;
  return simdjson::SUCCESS_AND_HAS_MORE;
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
  switch (c) {
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
