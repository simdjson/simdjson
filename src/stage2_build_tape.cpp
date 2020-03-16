#include "simdjson.h"
#include <cassert>
#include <cstring>
#include "jsoncharutils.h"
#include "document_parser_callbacks.h"

using namespace simdjson;

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/stage2_build_tape.h"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/stage2_build_tape.h"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/stage2_build_tape.h"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/stage2_build_tape.h"
#endif
