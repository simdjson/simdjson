#ifndef JSONIOUTIL_H
#define JSONIOUTIL_H

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "common_defs.h"

// get a corpus; pad out to cache line so we can always use SIMD
// throws exceptions in case of failure
std::pair<u8 *, size_t> get_corpus(std::string filename);

#endif
