#include "simdjson/portability.h"
#include "simdjson/stage1_find_marks.h"

#ifdef IS_X86_64
#include "simdjson/stage1_find_marks_haswell.h"
#include "simdjson/stage1_find_marks_westmere.h"

TARGET_HASWELL
namespace simdjson {
#define ARCHITECTURE Architecture::HASWELL
#include "simdjson/stage1_find_marks_macros.h"
#undef ARCHITECTURE
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
#define ARCHITECTURE Architecture::WESTMERE
#include "simdjson/stage1_find_marks_macros.h"
#undef ARCHITECTURE
} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#ifdef IS_ARM64
#include "simdjson/stage1_find_marks_arm64.h"
namespace simdjson {
#define ARCHITECTURE Architecture::NEON
#include "simdjson/stage1_find_marks_macros.h"
#undef ARCHITECTURE
}

} // namespace simdjson
#endif // IS_ARM64