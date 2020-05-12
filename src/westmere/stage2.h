#ifndef SIMDJSON_WESTMERE_STAGE2_H
#define SIMDJSON_WESTMERE_STAGE2_H

#include "simdjson.h"
#include "westmere/implementation.h"
#include "westmere/stringparsing.h"
#include "westmere/numberparsing.h"

TARGET_WESTMERE
namespace simdjson {
namespace westmere {

#include "generic/atomparsing.h"
#include "generic/structural_iterator.h"
#include "generic/structural_parser.h"
#include "generic/streaming_structural_parser.h"

} // namespace westmere
} // namespace simdjson
UNTARGET_REGION
#endif // SIMDJSON_WESTMERE_STAGE2_H
