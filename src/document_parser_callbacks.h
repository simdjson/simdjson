#ifndef SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
#define SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H

#include "simdjson.h"

namespace simdjson {
namespace dom {

//
// Parser callbacks
//

really_inline void parser::write_tape(uint64_t val, internal::tape_type t) noexcept {
  doc.tape[current_loc++] = val | ((uint64_t(char(t))) << 56);
}

} // namespace simdjson
} // namespace dom

#endif // SIMDJSON_DOCUMENT_PARSER_CALLBACKS_H
