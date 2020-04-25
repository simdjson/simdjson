#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
#if defined(SIMDJSON_CLANG_VISUAL_STUDIO)
SIMDJSON_DISABLE_GCC_WARNING(-Wmicrosoft-include)
#endif
SIMDJSON_DISABLE_DEPRECATED_WARNING

#include "error.cpp"
#include "implementation.cpp"
#include "stage1_find_marks.cpp"
#include "stage2_build_tape.cpp"

SIMDJSON_POP_DISABLE_WARNINGS
