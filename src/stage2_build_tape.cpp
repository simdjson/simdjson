#include "simdjson/stage2_build_tape.h"

#ifdef IS_X86_64
TARGET_HASWELL
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::HASWELL
#include "simdjson/stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::WESTMERE
#include "simdjson/stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE 
} // namespace simdjson
UNTARGET_REGION
#endif // IS_X86_64

#ifdef IS_ARM64
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::ARM64
#include "simdjson/stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE
} // namespace simdjson
#endif
