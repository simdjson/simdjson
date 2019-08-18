#ifndef SIMDJSON_HASWELL_ARCHITECTURE_H
#define SIMDJSON_HASWELL_ARCHITECTURE_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/simdjson.h"

TARGET_HASWELL
namespace simdjson::haswell {

static const Architecture ARCHITECTURE = Architecture::HASWELL;

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_ARCHITECTURE_H
