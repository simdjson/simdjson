#ifndef SIMDJSON_FALLBACK_STAGE2_BUILD_TAPE_H
#define SIMDJSON_FALLBACK_STAGE2_BUILD_TAPE_H

#include "simdjson.h"

#include "fallback/implementation.h"
#include "fallback/stringparsing.h"
#include "fallback/numberparsing.h"

namespace simdjson {
namespace fallback {

#include "generic/atomparsing.h"
#include "generic/stage2_build_tape.h"
#include "generic/stage2_streaming_build_tape.h"

} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_STAGE2_BUILD_TAPE_H
