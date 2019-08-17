#include "simdjson/stage2_build_tape.h"

#ifdef IS_X86_64

#include "stringparsing_haswell.h"
#define TARGETED_ARCHITECTURE Architecture::HASWELL
#define TARGETED_REGION TARGET_HASWELL
#include "stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#include "stringparsing_westmere.h"
#define TARGETED_ARCHITECTURE Architecture::WESTMERE
#define TARGETED_REGION TARGET_WESTMERE
#include "stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#endif // IS_X86_64

#ifdef IS_ARM64

#include "stringparsing_arm64.h"
#define TARGETED_ARCHITECTURE Architecture::ARM64
#define TARGETED_REGION TARGET_ARM64
#include "stage2_build_tape_common.h"
#undef TARGETED_ARCHITECTURE
#undef TARGETED_REGION

#endif // IS_ARM64
