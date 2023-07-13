#ifndef SIMDJSON_SRC_GENERIC_STAGE2_BASE_H

#ifdef SIMDJSON_IN_EDITOR_IMPL
#define SIMDJSON_SRC_GENERIC_STAGE2_BASE_H
#include "generic/base.h"
#endif // SIMDJSON_IN_EDITOR_IMPL

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

class json_iterator;
class structural_iterator;
struct tape_builder;
struct tape_writer;

} // namespace stage2
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_BASE_H