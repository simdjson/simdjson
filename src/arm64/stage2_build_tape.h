#ifndef SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
#define SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H

#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/stringparsing.h"
#include "arm64/numberparsing.h"

namespace simdjson::arm64 {

#include "generic/atomparsing.h"
#include "generic/stage2_build_tape.h"
#include "generic/stage2_streaming_build_tape.h"

} // namespace simdjson::arm64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
