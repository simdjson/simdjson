#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_MEMORYMAPPEDLEXSOURCE_H
#define HTCW_MEMORYMAPPEDLEXSOURCE_H
// platformIO screams if this isn't present:
#ifndef ARDUINO
#ifdef __cplusplus
#include "MemoryMappedLexSource.hpp"

using namespace lex;

#else

#error MemoryMappedLexSource requires a C++ compiler

#endif
#endif
#endif