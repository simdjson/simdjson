#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>

#include "jsonparser/common_defs.h"
#include "jsonparser/simdjson_internal.h"
#include "jsonparser/jsoncharutils.h"
#include "jsonparser/numberparsing.h"
#include "jsonparser/stringparsing.h"

#include <iostream>
//#define DEBUG
#define PATH_SEP '/'

#if defined(DEBUG) && !defined(DEBUG_PRINTF)
#include <string.h>
#include <stdio.h>
#define DEBUG_PRINTF(format, ...) printf("%s:%s:%d:" format, \
                                         strrchr(__FILE__, PATH_SEP) + 1, \
                                         __func__, __LINE__,  ## __VA_ARGS__)
#elif !defined(DEBUG_PRINTF)
#define DEBUG_PRINTF(format, ...) do { } while(0)
#endif

using namespace std;

WARN_UNUSED
really_inline bool is_valid_true_atom(const u8 * loc) {
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
really_inline bool is_valid_false_atom(const u8 * loc) {
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
really_inline bool is_valid_null_atom(const u8 * loc) {
    u64 nv = *(const u64 *)"null    ";
    u64 mask4 = 0x00000000ffffffff;
    u32 error = 0;
    u64 locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
    std::memcpy(&locval, loc, sizeof(u64));
    error = (locval & mask4) ^ nv;
    error |= is_not_structural_or_whitespace(loc[4]);
    return error == 0;
}

WARN_UNUSED
bool unified_machine(const u8 *buf, size_t len, ParsedJson &pj) {
    u32 i = 0;
    u32 idx;
    u8 c;
    u32 depth = START_DEPTH; // an arbitrary starting depth
    void * ret_address[MAX_DEPTH];

    u32 last_loc = 0; // this is the location of the previous call site; only need one
    // We should also track the tape address of our containing
    // scope for two reasons. First, we will need to put an 
    // up pointer there at each call site so we can navigate
    // upwards. Second, when we encounter the end of the scope
    // we can put the current offset into a record for the 
    // scope so we know where it is

    u32 containing_scope_offset[MAX_DEPTH];

    pj.init();

    // add a sentinel to the end to avoid premature exit
    // need to be able to find the \0 at the 'padded length' end of the buffer
    // FIXME: TERRIFYING!
    size_t j;
    for (j = len; buf[j] != 0; j++)
        ;
    pj.structural_indexes[pj.n_structural_indexes++] = j;

#define UPDATE_CHAR() { idx = pj.structural_indexes[i++]; c = buf[idx]; DEBUG_PRINTF("Got %c at %d (%d offset)\n", c, idx, i-1);}

    // format: call site has 2 entries: 56-bit + '{' or '[' entries pointing first to header then to this location
    //         scope has 2 entries: 56 + '_' entries pointing first to call site then to the last entry in this scope

#define OPEN_SCOPE() { \
    pj.write_saved_loc(last_loc, pj.save_loc(depth), '_'); \
    pj.write_tape(depth, last_loc, '_'); \
    containing_scope_offset[depth] = pj.save_loc(depth); \
    pj.write_tape(depth, 0, '_'); \
    }

#define ESTABLISH_CALLSITE(RETURN_LABEL, SITE_LABEL) { \
    pj.write_tape(depth, containing_scope_offset[depth], c); \
    last_loc = pj.save_loc(depth); \
    pj.write_tape(depth, 0, c); \
    ret_address[depth] = RETURN_LABEL; \
    depth++; \
    goto SITE_LABEL; \
    } 

////////////////////////////// START STATE /////////////////////////////

    DEBUG_PRINTF("at start\n");
    UPDATE_CHAR();
    // do these two speculatively as we will always do
    // them except on fail, in which case it doesn't matter
    ret_address[depth] = &&start_continue;
    containing_scope_offset[depth] = pj.save_loc(depth);
    pj.write_tape(depth, 0, c); // dummy entries

    last_loc = pj.save_loc(depth);
    pj.write_tape(depth, 0, c); // dummy entries
    depth++;

    switch (c) {
        case '{': goto object_begin;
        case '[': goto array_begin;
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
            goto start_continue;
        }
        case 't': 
            if (!is_valid_true_atom(buf + idx)) {
                goto fail;
            }
            pj.write_tape(depth, 0, c);
            goto start_continue;
        case 'f': 
            if (!is_valid_false_atom(buf + idx)) {
                goto fail;
            }
            pj.write_tape(depth, 0, c);
            goto start_continue;
        case 'n': 
            if (!is_valid_null_atom(buf + idx)) {
                goto fail;
            }
            pj.write_tape(depth, 0, c);
            goto start_continue;
        case '0': {
            if (!parse_number(buf, len, pj, depth, idx, true, false)) {
                goto fail;
            }
            goto start_continue;
        }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':  {
            if (!parse_number(buf, len, pj, depth, idx, false, false)) {
                goto fail;
            }
            goto start_continue;
        }
        case '-': {
            if (!parse_number(buf, len, pj, depth, idx, false, true)) {
                goto fail;
            }
            goto start_continue;
        }
#endif // ALLOWANYTHINGINROOT
        default: goto fail;
    }

start_continue:
    // land here after popping our outer object if an object
    DEBUG_PRINTF("in start_object_close\n");
    UPDATE_CHAR();
    switch (c) {
        case 0: goto succeed;
        default: goto fail;
    }

////////////////////////////// OBJECT STATES /////////////////////////////

object_begin:
    DEBUG_PRINTF("in object_begin\n");
    OPEN_SCOPE();
    UPDATE_CHAR();
    switch (c) {
        case '"': {
            if (!parse_string(buf, len, pj, depth, idx)) {
                goto fail;
            }
            goto object_key_state;
        }
        case '}': goto scope_end;
        default: goto fail;
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
        case 't': if (!is_valid_true_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case '0': {
            if (!parse_number(buf, len, pj, depth, idx, true, false)) {
                goto fail;
            }
            break;
        }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':  {
            if (!parse_number(buf, len, pj, depth, idx, false, false)) {
                goto fail;
            }
            break;
        }
        case '-': {
            if (!parse_number(buf, len, pj, depth, idx, false, true)) {
                goto fail;
            }
            break;
        }
        case '{': {
            ESTABLISH_CALLSITE(&&object_continue, object_begin);
        }
        case '[': {
            ESTABLISH_CALLSITE(&&object_continue, array_begin);
        }
        default: goto fail;
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
        case '}': goto scope_end;
        default: goto fail;
    }

////////////////////////////// COMMON STATE /////////////////////////////

scope_end: 
    // write our tape location to the header scope
    pj.write_saved_loc(containing_scope_offset[depth], pj.save_loc(depth), '_');
    depth--;
    // goto saved_state
    goto *ret_address[depth];

    
////////////////////////////// ARRAY STATES /////////////////////////////

array_begin:
    DEBUG_PRINTF("in array_begin\n");
    OPEN_SCOPE();
    // fall through

    UPDATE_CHAR();
    if (c == ']') {
        goto scope_end;
    }

main_array_switch:
    // we call update char on all paths in, so we can peek at c on the
    // on paths that can accept a close square brace (post-, and at start)
    switch (c) {
        case '"': {
            if (!parse_string(buf, len, pj, depth, idx)) {
                goto fail;
            }
            goto array_continue;
        }
        case 't': if (!is_valid_true_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(depth, 0, c);
                  break;
        
        case '0': {
            if (!parse_number(buf, len, pj, depth, idx, true, false)) {
                goto fail;
            }
            break;
        }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':  {
            if (!parse_number(buf, len, pj, depth, idx, false, false)) {
                goto fail;
            }
            break;
        }
        case '-': {
            if (!parse_number(buf, len, pj, depth, idx, false, true)) {
                goto fail;
            }
            break;
        }
        case '{': {
            ESTABLISH_CALLSITE(&&array_continue, object_begin);
        }
        case '[': {
            ESTABLISH_CALLSITE(&&array_continue, array_begin);
        }
        default: goto fail;
    }

array_continue:
    DEBUG_PRINTF("in array_continue\n");
    UPDATE_CHAR();
    switch (c) {
        case ',': UPDATE_CHAR(); goto main_array_switch;
        case ']': goto scope_end;
        default: goto fail;
    }

////////////////////////////// FINAL STATES /////////////////////////////

succeed:
    DEBUG_PRINTF("in succeed\n");
#ifdef DEBUG
    pj.dump_tapes();
#endif
    return true;
    
fail:
    DEBUG_PRINTF("in fail\n");
#ifdef DEBUG
    pj.dump_tapes();
#endif
    return false;    
}
