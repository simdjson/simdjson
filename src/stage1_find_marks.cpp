#include "simdjson/portability.h"

#ifdef IS_X86_64
#include "haswell/stage1_find_marks.h"
#include "westmere/stage1_find_marks.h"
#endif // IS_X86_64

#ifdef IS_ARM64
#include "arm64/stage1_find_marks.h"
#endif // IS_ARM64
