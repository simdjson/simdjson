/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    size_t i{next_json}; /* index of the structural character (0,1,2,3...) */
    size_t idx; /* location of the structural character in the input (buf)   */
    uint8_t c;    /* used to track the (structural) character we are looking at,
                   updated */
    /* by UPDATE_CHAR macro */
    size_t depth = 0; /* could have an arbitrary starting depth */
    pj.init();          /* sets is_valid to false          */
//    if (pj.byte_capacity < len) {
//        pj.error_code = simdjson::CAPACITY;
//        return pj.error_code;
//    }

    /*//////////////////////////// START STATE /////////////////////////////
     */
    SET_GOTO_START_CONTINUE()
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
    /* the root is used, if nothing else, to capture the size of the tape */
    depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
    if (depth >= pj.depth_capacity) {
        goto fail;
    }

    UPDATE_CHAR();
    switch (c) {
        case '{':
            pj.containing_scope_offset[depth] = pj.get_current_loc();
            SET_GOTO_START_CONTINUE();
            depth++;
            if (depth >= pj.depth_capacity) {
                goto fail;
            }
            pj.write_tape(
                    0, c); /* strangely, moving this to object_begin slows things down */
            goto object_begin;
        case '[':
            pj.containing_scope_offset[depth] = pj.get_current_loc();
            SET_GOTO_START_CONTINUE();
            depth++;
            if (depth >= pj.depth_capacity) {
                goto fail;
            }
            pj.write_tape(0, c);
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
            copy[len] = ' ';
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
            copy[len] = ' ';
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
            copy[len] = ' ';
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
            copy[len] = ' ';
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
            copy[len] = ' ';
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
    start_continue:
    /* the string might not be NULL terminated. */
    if (AT_EOF() && buf[idx+2] == '\0') {
        goto succeed;
    } else if(depth == 1 && !AT_EOF()) {
        goto succeedAndHasMore;
    } else {
        goto fail;
    }
    /*//////////////////////////// OBJECT STATES ///////////////////////////*/

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
            goto scope_end; /* could also go to object_continue */
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
            pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
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
            pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
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

    /*//////////////////////////// COMMON STATE ///////////////////////////*/

    scope_end:
    /* write our tape location to the header scope */
    depth--;
    pj.write_tape(pj.containing_scope_offset[depth], c);
    pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                             pj.get_current_loc());
    /* goto saved_state */
    GOTO_CONTINUE()

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
            /* we have not yet encountered ] so we need to come back for it */
            pj.containing_scope_offset[depth] = pj.get_current_loc();
            pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
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
            pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
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

    /*//////////////////////////// FINAL STATES ///////////////////////////*/
    succeedAndHasMore:
        depth--;
        if (pj.containing_scope_offset[depth] != 0) {
            fprintf(stderr, "internal bug\n");
            abort();
        }
        pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                                 pj.get_current_loc());
        pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */


        next_json = i;

        pj.valid = true;
        pj.error_code = simdjson::SUCCESS_AND_HAS_MORE;
        return pj.error_code;

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
//    if(i <= pj.n_structural_indexes) {
//        printf("%.32s    ...    %.32s\n", &buf[pj.structural_indexes[next_json]], &buf[100000 - 31]);
//        printf("last structural char (%u): %.1s\n", pj.structural_indexes[pj.n_structural_indexes-1], &buf[pj.structural_indexes[pj.n_structural_indexes-1]]);
//        printf("second to last structural char (%u): %.1s\n", pj.structural_indexes[pj.n_structural_indexes-2], &buf[pj.structural_indexes[pj.n_structural_indexes-2]]);
//        printf("structural char at 0: %.1s\n", &buf[pj.structural_indexes[pj.n_structural_indexes]]);
//    }
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
