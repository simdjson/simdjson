#ifndef __PARSED_JSON_WRITER_H__
#define __PARSED_JSON_WRITER_H__

namespace simdjson {

struct ParsedJsonWriter {
  ParsedJson &pj;

  really_inline ErrorValues on_error(ErrorValues error_code) {
    pj.error_code = error_code;
    return error_code;
  }
  really_inline ErrorValues on_success(ErrorValues success_code) {
    pj.error_code = success_code;
    pj.valid = true;
    return success_code;
  }
  really_inline bool on_start_document(uint32_t depth) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, 'r');
    return true;
  }
  really_inline bool on_start_object(uint32_t depth) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, '{');
    return true;
  }
  really_inline bool on_start_array(uint32_t depth) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, '[');
    return true;
  }
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    pj.write_tape(pj.containing_scope_offset[depth], 'r');
    return true;
  }
  really_inline bool on_end_object(uint32_t depth) {
    // write our tape location to the header scope
    pj.write_tape(pj.containing_scope_offset[depth], '}');
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    return true;
  }
  really_inline bool on_end_array(uint32_t depth) {
    // write our tape location to the header scope
    pj.write_tape(pj.containing_scope_offset[depth], ']');
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    return true;
  }

  really_inline bool on_true_atom() {
    pj.write_tape(0, 't');
    return true;
  }
  really_inline bool on_false_atom() {
    pj.write_tape(0, 'f');
    return true;
  }
  really_inline bool on_null_atom() {
    pj.write_tape(0, 'n');
    return true;
  }

  really_inline uint8_t *on_start_string() {
    /* we advance the point, accounting for the fact that we have a NULL
      * termination         */
    pj.write_tape(pj.current_string_buf_loc - pj.string_buf.get(), '"');
    return pj.current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline bool on_end_string(uint8_t *dst) {
    uint32_t str_length = dst - (pj.current_string_buf_loc + sizeof(uint32_t));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(pj.current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    pj.current_string_buf_loc = dst + 1;
    return true;
  }

  really_inline bool on_number_s64(int64_t value) {
    pj.write_tape_s64(value);
    return true;
  }
  really_inline bool on_number_u64(uint64_t value) {
    pj.write_tape_u64(value);
    return true;
  }
  really_inline bool on_number_double(double value) {
    pj.write_tape_double(value);
    return true;
  }
};

} // namespace simdjson

#endif // __PARSED_JSON_WRITER_H__
