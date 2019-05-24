#include <cassert>
#include <cstring>

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/numberparsing.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stringparsing.h"
#include "simdjson/simdjson.h"

#include <iostream>
#define PATH_SEP '/'


WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint64_t tv = *reinterpret_cast<const uint64_t *>("true    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  // We have to use an integer constant because the space in the cast
  // below would lead to values illegally being qualified
  // uint64_t fv = *reinterpret_cast<const uint64_t *>("false   ");
  // using this constant (that is the same false) but nulls out the
  // unused bits solves that
  uint64_t fv = 0x00000065736c6166; // takes into account endianness
  uint64_t mask5 = 0x000000ffffffffff;
  // we can't use the 32 bit value for checking for errors otherwise
  // the last character of false (it being 5 byte long!) would be
  // ignored
  uint64_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask5) ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint64_t nv = *reinterpret_cast<const uint64_t *>("null    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}


/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER
int unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
#ifndef ALLOW_SAME_PAGE_BUFFER_OVERRUN
  memset((uint8_t*)buf + len, 0, SIMDJSON_PADDING); // to please valgrind
#endif
  uint32_t i = 0; // index of the structural character (0,1,2,3...)
  uint32_t idx;   // location of the structural character in the input (buf)
  uint8_t c; // used to track the (structural) character we are looking at, updated
        // by UPDATE_CHAR macro
  uint32_t depth = 0; // could have an arbitrary starting depth
  pj.init();
  if(pj.bytecapacity < len) {
      return simdjson::CAPACITY;
  }
// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR()                                                          \
  {                                                                            \
    idx = pj.structural_indexes[i++];                                          \
    c = buf[idx];                                                              \
  }


  ////////////////////////////// START STATE /////////////////////////////
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
  pj.ret_address[depth] = &&start_continue;
#else
  pj.ret_address[depth] = 's';
#endif
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); // r for root, 0 is going to get overwritten
  // the root is used, if nothing else, to capture the size of the tape
  depth++; // everything starts at depth = 1, depth = 0 is just for the root, the root may contain an object, an array or something else.
  if (depth >= pj.depthcapacity) {
    goto fail;
  }

  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&start_continue;
#else
    pj.ret_address[depth] = 's';
#endif
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }
    pj.write_tape(0, c); // strangely, moving this to object_begin slows things down
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&start_continue;
#else
    pj.ret_address[depth] = 's';
#endif    
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
#define SIMDJSON_ALLOWANYTHINGINROOT
    // A JSON text is a serialized value.  Note that certain previous
    // specifications of JSON constrained a JSON text to be an object or an
    // array.  Implementations that generate only objects or arrays where a
    // JSON text is called for will be interoperable in the sense that all
    // implementations will accept these as conforming JSON texts.
    // https://tools.ietf.org/html/rfc8259
#ifdef SIMDJSON_ALLOWANYTHINGINROOT
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the true value.
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { 
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'f': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the false value.
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { 
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'n': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the null value.
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { 
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = '\0';
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
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { 
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { 
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
#endif // ALLOWANYTHINGINROOT
  default:
    goto fail;
  }
start_continue:
  // the string might not be NULL terminated.
  if(i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  ////////////////////////////// OBJECT STATES /////////////////////////////

object_begin:
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    goto object_key_state;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto fail;
  }

object_key_state:
  UPDATE_CHAR();
  if (c != ':') {
    goto fail;
  }
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
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // here the compilers knows what c is so this gets optimized
    // we have not yet encountered } so we need to come back for it
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&object_continue;
#else
    pj.ret_address[depth] = 'o';
#endif
    // we found an object inside an object, so we need to increment the depth
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c);  // here the compilers knows what c is so this gets optimized
    // we have not yet encountered } so we need to come back for it
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&object_continue;
#else
    pj.ret_address[depth] = 'o';
#endif    
    // we found an array inside an object, so we need to increment the depth
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

object_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    if (c != '"') {
      goto fail;
    } else {
      if (!parse_string(buf, len, pj, depth, idx)) {
        goto fail; 
      }
      goto object_key_state;
    }
  case '}':
    goto scope_end;
  default:
    goto fail;
  }

  ////////////////////////////// COMMON STATE /////////////////////////////

scope_end:
  // write our tape location to the header scope
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previousloc(pj.containing_scope_offset[depth],
                          pj.get_current_loc());
  // goto saved_state
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  goto *pj.ret_address[depth];
#else
  if(pj.ret_address[depth] == 'a') {
    goto array_continue;
  } else if (pj.ret_address[depth] == 'o') {
    goto object_continue;
  } else goto start_continue;
#endif

  ////////////////////////////// ARRAY STATES /////////////////////////////
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  // we call update char on all paths in, so we can peek at c on the
  // on paths that can accept a close square brace (post-, and at start)
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
    break; // goto array_continue;

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
    break; // goto array_continue;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; // goto array_continue;
  }
  case '{': {
    // we have not yet encountered ] so we need to come back for it
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); //  here the compilers knows what c is so this gets optimized
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&array_continue;
#else
    pj.ret_address[depth] = 'a';
#endif
    // we found an object inside an array, so we need to increment the depth
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    // we have not yet encountered ] so we need to come back for it
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // here the compilers knows what c is so this gets optimized
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&array_continue;
#else
    pj.ret_address[depth] = 'a';
#endif
    // we found an array inside an array, so we need to increment the depth
    depth++;
    if (depth >= pj.depthcapacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

array_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto fail;
  }

  ////////////////////////////// FINAL STATES /////////////////////////////

succeed:
  depth --;
  if(depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if(pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previousloc(pj.containing_scope_offset[depth],
                          pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); // r is root



  pj.isvalid  = true;
  return simdjson::SUCCESS;
fail:
  // At this point in the code, we have all the time in the world.
  // Note that we know exactly where we are in the document so we could,
  // without any overhead on the processing code, report a specific location.
  // We could even trigger special code paths to assess what happened carefully,
  // all without any added cost.
  if (depth >= pj.depthcapacity) {
    return simdjson::DEPTH_ERROR;
  }
  switch(c) {
    case '"': 
      return simdjson::STRING_ERROR;
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
      return simdjson::NUMBER_ERROR;
    case 't':
      return simdjson::T_ATOM_ERROR;
    case 'n':
      return simdjson::N_ATOM_ERROR;
    case 'f':
      return simdjson::F_ATOM_ERROR;
    default: 
      break;
  }
  return simdjson::TAPE_ERROR; 
}

int unified_machine(const char *buf, size_t len, ParsedJson &pj) {
  return unified_machine(reinterpret_cast<const uint8_t*>(buf), len, pj);
}
