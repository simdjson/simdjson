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

// Implemented using Labels as Values which works in GCC and CLANG (and maybe also in Intel's compiler),
// but won't work in MSVC. This would need to be reimplemented differently
// if one wants to be standard compliant.
WARN_UNUSED
bool unified_machine(const u8 *buf, size_t len, ParsedJson &pj) {
    u32 i = 0; // index of the structural character (0,1,2,3...)
    u32 idx; // location of the structural character in the input (buf)
    u8 c; // used to track the (structural) character we are looking at, updated by UPDATE_CHAR macro
    u32 depth = 0;//START_DEPTH; // an arbitrary starting depth
    //void * ret_address[MAX_DEPTH]; // used to store "labels as value" (non-standard compiler extension)

    // a call site is the start of either an object or an array ('[' or '{')
    // this is the location of the previous call site 
    // (in the tape, at the given depth); 
    // we only need one.

    // We should also track the tape address of our containing
    // scope for two reasons. First, we will need to put an 
    // up pointer there at each call site so we can navigate
    // upwards. Second, when we encounter the end of the scope
    // we can put the current offset into a record for the 
    // scope so we know where it is

    //u32 containing_scope_offset[MAX_DEPTH];

    pj.init();

    // add a sentinel to the end to avoid premature exit
    // need to be able to find the \0 at the 'padded length' end of the buffer
    // FIXME: TERRIFYING!
    //size_t j;
    //for (j = len; buf[j] != 0; j++)
    //    ;
    //pj.structural_indexes[pj.n_structural_indexes++] = j;

// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR() { idx = pj.structural_indexes[i++]; c = buf[idx]; DEBUG_PRINTF("Got %c at %d (%d offset)\n", c, idx, i-1);}






////////////////////////////// START STATE /////////////////////////////
printf("at start\n");
    DEBUG_PRINTF("at start\n");
    pj.ret_address[depth] = &&start_continue; 
    pj.containing_scope_offset[depth] = pj.get_current_loc(); 
    pj.write_tape(0, 'r'); // r for root, 0 is going to get overwritten
    depth++;// everything starts at depth = 1, depth = 0 is just for the root
    if(depth > pj.depthcapacity) {
        goto fail;
    }
    printf("got char %c \n",c);
    UPDATE_CHAR();
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
            pj.write_tape(0, c);
            goto start_continue;
        case 'f': 
            if (!is_valid_false_atom(buf + idx)) {
                goto fail;
            }
            pj.write_tape(0, c);
            goto start_continue;
        case 'n': 
            if (!is_valid_null_atom(buf + idx)) {
                goto fail;
            }
            pj.write_tape(0, c);
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
    DEBUG_PRINTF("in start_object_close\n");
    UPDATE_CHAR();
    switch (c) {
        case 0: goto succeed;
        default: goto fail;
    }

////////////////////////////// OBJECT STATES /////////////////////////////

object_begin:
    printf("in object_begin %c \n",c);
    DEBUG_PRINTF("in object_begin\n");
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); 
    depth ++;
    if(depth > pj.depthcapacity) {
        goto fail;
    }
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
    printf("in object_key_state %c \n",c);

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
                  pj.write_tape(0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(0, c);
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
            pj.ret_address[depth] = &&object_continue; 
            goto object_begin;
        }
        case '[': {
            pj.ret_address[depth] = &&object_continue; 
            goto array_begin;
        }
        default: goto fail;
    }

object_continue:
    printf("in object_continue %c \n",c);

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
    depth--;
    pj.write_tape(pj.containing_scope_offset[depth], c);
    pj.annotate_previousloc(pj.containing_scope_offset[depth], pj.get_current_loc());
    // goto saved_state
    goto *pj.ret_address[depth];

    
////////////////////////////// ARRAY STATES /////////////////////////////

array_begin:
    printf("in array_begin %c \n",c);

    DEBUG_PRINTF("in array_begin\n");
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); 
    depth ++;
    if(depth > pj.depthcapacity) {
        goto fail;
    }
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
                  pj.write_tape(0, c);
                  break;
        case 'f': if (!is_valid_false_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(0, c);
                  break;
        case 'n': if (!is_valid_null_atom(buf + idx)) {
                    goto fail;
                  }
                  pj.write_tape(0, c);
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
            pj.ret_address[depth] = &&array_continue; 
            goto object_begin;
        }
        case '[': {
            pj.ret_address[depth] = &&array_continue; 
            goto array_begin;
        }
        default: goto fail;
    }

array_continue:
    printf("in array_begin %c \n",c);

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
    // we annotate the root node
    depth--;
    // next line allows us to go back to the start
    pj.write_tape(pj.containing_scope_offset[depth], 'r');// r is root
    // next line tells the root node how to go to the end
    pj.annotate_previousloc(pj.containing_scope_offset[depth], pj.get_current_loc());

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
