#include "simdjson.h"

#if defined(SIMDJSON_CLANG_VISUAL_STUDIO)
SIMDJSON_DISABLE_GCC_WARNING(-Wmicrosoft-include)
#endif


#include "error.cpp"
#include "implementation.cpp"
#include "stage1_find_marks.cpp"
#include "stage2_build_tape.cpp"
