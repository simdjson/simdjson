#include "simdjson/stage1_find_marks.h"
#include "simdjson/portability.h"

#ifdef IS_X86_64
#include "simdjson/stage1_find_marks_haswell.h"
#include "simdjson/stage1_find_marks_westmere.h"

TARGET_HASWELL
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::HASWELL
#include "./stage1_find_marks_common.cpp"
#undef TARGETED_ARCHITECTURE
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::WESTMERE
#include "./stage1_find_marks_common.cpp"
#undef TARGETED_ARCHITECTURE
} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#ifdef IS_ARM64
#include "simdjson/stage1_find_marks_arm64.h"
namespace simdjson {
#define TARGETED_ARCHITECTURE Architecture::NEON
#include "./stage1_find_marks_common.cpp"
#undef TARGETED_ARCHITECTURE
} // namespace simdjson
#endif // IS_ARM64