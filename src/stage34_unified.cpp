#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/numberparsing.h"
#include "simdjson/parsedjson.h"
#include "simdjson/stringparsing.h"

#include <iostream>
//#define DEBUG
#define PATH_SEP '/'

#if defined(DEBUG) && !defined(DEBUG_PRINTF)
#include <stdio.h>
#include <string.h>
#define DEBUG_PRINTF(format, ...)                                              \
  printf("%s:%s:%d:" format, strrchr(__FILE__, PATH_SEP) + 1, __func__,        \
         __LINE__, ##__VA_ARGS__)
#elif !defined(DEBUG_PRINTF)
#define DEBUG_PRINTF(format, ...)                                              \
  do {                                                                         \
  } while (0)
#endif

using namespace std;

WARN_UNUSED
really_inline bool is_valid_true_atom(const u8 *loc) {
  u64 tv = *(const u64 *)"true    ";
  u64 mask4 = 0x00000000ffffffff;
  u32 error = 0;
  u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(u64));
  error = (locval & mask4) ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const u8 *loc) {
  u64 fv = *(const u64 *)"false   ";
  u64 mask5 = 0x000000ffffffffff;
  u32 error = 0;
  u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(u64));
  error = (locval & mask5) ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const u8 *loc) {
  u64 nv = *(const u64 *)"null    ";
  u64 mask4 = 0x00000000ffffffff;
  u32 error = 0;
  u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(u64));
  error = (locval & mask4) ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

// Implemented using Labels as Values which works in GCC and CLANG (and maybe
// also in Intel's compiler), but won't work in MSVC. This would need to be
// reimplemented differently if one wants to be standard compliant.
WARN_UNUSED
bool unified_machine(const u8 *buf, size_t len, ParsedJson &pj) {
  u32 i = 0; // index of the structural character (0,1,2,3...)
  u32 idx;   // location of the structural character in the input (buf)
  u8 c; // used to track the (structural) character we are looking at, updated
        // by UPDATE_CHAR macro
  u32 depth = 0; // could have an arbitrary starting depth
  pj.init();
  if(pj.bytecapacity < len) {
      printf("insufficient capacity\n");
      return false;
  }
// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR()                                                          \
  {                                                                            \
    idx = pj.structural_indexes[i++];                                          \
    c = buf[idx];                                                              \
    DEBUG_PRINTF("Got %c at %d (%d offset) (depth %d)\n", c, idx, i - 1,       \
                 depth);                                                       \
  }


  ////////////////////////////// START STATE /////////////////////////////
  DEBUG_PRINTF("at start\n");
  pj.ret_address[depth] = &&start_continue;
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); // r for root, 0 is going to get overwritten
  // the root is used, if nothing else, to capture the size of the tape
  depth++; // everything starts at depth = 1, depth = 0 is just for the root, the root may contain an object, an array or something else.
  if (depth > pj.depthcapacity) {
    goto fail;
  }
  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.write_tape(0, c); // strangely, moving this to object_begin slows things down
    goto object_begin;
  case '[':
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
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    char * copy = (char *) malloc(len + 1 + 64);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number((const u8 *)copy, pj, idx, false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    char * copy = (char *) malloc(len + 1 + 64);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number((const u8 *)copy, pj, idx, true)) {
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
#ifdef SIMDJSON_ALLOWANYTHINGINROOT
  depth--; // for fall-through cases (e.g., documents containing just a string)
  pj.annotate_previousloc(pj.containing_scope_offset[depth],
                          pj.get_current_loc());
#endif     // ALLOWANYTHINGINROOT
start_continue:
  DEBUG_PRINTF("in start_object_close\n");
  // the string might not be NULL terminated.
  if(i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  ////////////////////////////// OBJECT STATES /////////////////////////////

object_begin:
  DEBUG_PRINTF("in object_begin\n");
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
  DEBUG_PRINTF("in object_key_state\n");
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
    pj.ret_address[depth] = &&object_continue;
    // we found an object inside an object, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c);  // here the compilers knows what c is so this gets optimized
    // we have not yet encountered } so we need to come back for it
    pj.ret_address[depth] = &&object_continue;
    // we found an array inside an object, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

object_continue:
  DEBUG_PRINTF("in object_continue\n");
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
  goto *pj.ret_address[depth];

  ////////////////////////////// ARRAY STATES /////////////////////////////
array_begin:
  DEBUG_PRINTF("in array_begin\n");
  //pj.containing_scope_offset[depth] = pj.get_current_loc();
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
    pj.ret_address[depth] = &&array_continue;
    // we found an object inside an array, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    // we have not yet encountered ] so we need to come back for it
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // here the compilers knows what c is so this gets optimized
    pj.ret_address[depth] = &&array_continue;
    // we found an array inside an array, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

array_continue:
  DEBUG_PRINTF("in array_continue\n");
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
  DEBUG_PRINTF("in succeed, depth = %d \n", depth);
  if(depth != 0) {
    printf("internal bug\n");
    abort();
  }
  if(pj.containing_scope_offset[depth] != 0) {
    printf("internal bug\n");
    abort();
  }
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); // r is root


#ifdef DEBUG
  pj.dump_raw_tape();
#endif
  pj.isvalid  = true;
  return true;

fail:
  DEBUG_PRINTF("in fail\n");
#ifdef DEBUG
  pj.dump_tapes();
#endif
  return false;
}
