#include "simdjson.h"
#include "haswell/implementation.h"
#include "haswell/stringparsing.h"
#include "haswell/numberparsing.h"

TARGET_HASWELL
namespace simdjson {
namespace haswell {

#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/structural_parser.h"
#include "generic/stage2/streaming_structural_parser.h"

} // namespace haswell
} // namespace simdjson
UNTARGET_REGION
