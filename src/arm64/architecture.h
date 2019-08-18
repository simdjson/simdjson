#ifndef SIMDJSON_ARM64_ARCHITECTURE_H
#define SIMDJSON_ARM64_ARCHITECTURE_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "simdjson/simdjson.h"

namespace simdjson::arm64 {

static const Architecture ARCHITECTURE = Architecture::ARM64;

} // namespace simdjson::arm64

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_ARCHITECTURE_H
