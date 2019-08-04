#include "simdjson/stage1_find_marks.h"
#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/stage1_find_marks_haswell.h"
#include "simdjson/stage1_find_marks_westmere.h"

#define TARGETED_ARCHITECTURE Architecture::HASWELL
#define TARGETED_REGION TARGET_HASWELL
#include "simdjson/stage1_find_marks_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#define TARGETED_ARCHITECTURE Architecture::WESTMERE
#define TARGETED_REGION TARGET_WESTMERE
#include "simdjson/stage1_find_marks_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#endif // IS_X86_64

#ifdef IS_ARM64

#include "simdjson/stage1_find_marks_arm64.h"

#define TARGETED_ARCHITECTURE Architecture::ARM64
#define TARGETED_REGION TARGET_ARM64
#include "simdjson/stage1_find_marks_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#endif // IS_ARM64
