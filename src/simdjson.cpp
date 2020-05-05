#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

#include "error.cpp"
#include "implementation.cpp"
#include "stage1_find_marks.cpp"
#include "stage2_build_tape.cpp"

SIMDJSON_POP_DISABLE_WARNINGS
