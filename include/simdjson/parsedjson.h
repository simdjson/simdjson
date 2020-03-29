// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_PARSEDJSON_H
#define SIMDJSON_PARSEDJSON_H

#include "simdjson/document.h"

namespace simdjson {

/**
 * @deprecated Use `document::parser` instead.
 */
using ParsedJson = dom::parser;

} // namespace simdjson
#endif
