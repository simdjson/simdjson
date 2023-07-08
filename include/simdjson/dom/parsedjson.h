// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_PARSEDJSON_H
#define SIMDJSON_DOM_PARSEDJSON_H

namespace simdjson {

namespace dom {
class parser;
}

/**
 * @deprecated Use `dom::parser` instead.
 */
using ParsedJson [[deprecated("Use dom::parser instead")]] = dom::parser;

} // namespace simdjson

#endif // SIMDJSON_DOM_PARSEDJSON_H
