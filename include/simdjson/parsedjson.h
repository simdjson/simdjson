// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include "simdjson/document.h"

namespace simdjson {

/**
 * @deprecated Use `dom::parser` instead.
 */
using ParsedJson [[deprecated("Use dom::parser instead")]] = dom::parser;

} // namespace simdjson
#endif
