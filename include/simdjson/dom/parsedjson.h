// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_PARSEDJSON_H
#define SIMDJSON_DOM_PARSEDJSON_H

#include "simdjson/dom/document.h"

namespace simdjson {

/**
 * @deprecated Use `dom::parser` instead.
 */
using ParsedJson [[deprecated("Use dom::parser instead")]] = dom::parser;

} // namespace simdjson

#endif // SIMDJSON_DOM_PARSEDJSON_H
