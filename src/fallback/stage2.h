#ifndef SIMDJSON_FALLBACK_STAGE2_H
#define SIMDJSON_FALLBACK_STAGE2_H

#include "simdjson.h"

#include "fallback/implementation.h"
#include "fallback/stringparsing.h"
#include "fallback/numberparsing.h"

namespace simdjson {
namespace fallback {

#include "generic/atomparsing.h"
#include "generic/structural_iterator.h"
#include "generic/structural_parser.h"
#include "generic/streaming_structural_parser.h"

} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_STAGE2_H
