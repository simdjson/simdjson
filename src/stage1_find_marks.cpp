#include "simdjson/portability.h"

#ifdef IS_X86_64
#include "stage1_find_marks_haswell.h"
#include "stage1_find_marks_westmere.h"
#endif // IS_X86_64

#ifdef IS_ARM64
#include "stage1_find_marks_arm64.h"
#endif // IS_ARM64
