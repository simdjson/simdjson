#ifndef SIMDJSON_ARM64_STAGE2_H
#define SIMDJSON_ARM64_STAGE2_H

#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/stringparsing.h"
#include "arm64/numberparsing.h"

namespace simdjson {
namespace arm64 {

#include "generic/atomparsing.h"
#include "generic/structural_iterator.h"
#include "generic/structural_parser.h"
#include "generic/streaming_structural_parser.h"

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_STAGE2_H
