#ifndef SIMDJSON_HASWELL_STAGE2_H
#define SIMDJSON_HASWELL_STAGE2_H

#include "simdjson.h"
#include "haswell/implementation.h"
#include "haswell/stringparsing.h"
#include "haswell/numberparsing.h"

TARGET_HASWELL
namespace simdjson {
namespace haswell {

#include "generic/atomparsing.h"
#include "generic/structural_iterator.h"
#include "generic/structural_parser.h"
#include "generic/streaming_structural_parser.h"

} // namespace haswell
} // namespace simdjson
UNTARGET_REGION

#endif // SIMDJSON_HASWELL_STAGE2_H
