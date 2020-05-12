#include "simdjson.h"
#include "westmere/implementation.h"
#include "westmere/stringparsing.h"
#include "westmere/numberparsing.h"

TARGET_WESTMERE
namespace simdjson {
namespace westmere {

#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/structural_parser.h"
#include "generic/stage2/streaming_structural_parser.h"

} // namespace westmere
} // namespace simdjson
UNTARGET_REGION
