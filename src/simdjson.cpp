#include "simdjson.h"

/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "error.cpp"
#include "implementation.cpp"
#include "stage1_find_marks.cpp"
#include "stage2_build_tape.cpp"
