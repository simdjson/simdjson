#ifndef SIMDJSON_WESTMERE_ARCHITECTURE_H
#define SIMDJSON_WESTMERE_ARCHITECTURE_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/simdjson.h"

TARGET_WESTMERE
namespace simdjson::westmere {

static const Architecture ARCHITECTURE = Architecture::WESTMERE;

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_WESTMERE_ARCHITECTURE_H
