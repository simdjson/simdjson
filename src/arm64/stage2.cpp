#ifndef SIMDJSON_ARM64_STAGE2_H
#define SIMDJSON_ARM64_STAGE2_H

#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/stringparsing.h"
#include "arm64/numberparsing.h"

namespace simdjson {
namespace arm64 {

#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/structural_parser.h"
#include "generic/stage2/streaming_structural_parser.h"

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_STAGE2_H
