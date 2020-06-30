#ifndef SIMDBJSON_DIRENT_PORTABLE_INC_
#define SIMDBJSON_DIRENT_PORTABLE_INC_

#if (!defined(_WIN32) && !defined(_WIN64) && !(__MINGW32__) && !(__MINGW64__))
#include <dirent.h>
#else
#include "toni_ronnko_dirent.h"
#endif

#endif // SIMDBJSON_DIRENT_PORTABLE_INC_
