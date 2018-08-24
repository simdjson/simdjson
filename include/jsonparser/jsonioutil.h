#ifndef JSONIOUTIL_H
#define JSONIOUTIL_H

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "common_defs.h"

// load a file in memory...
// get a corpus; pad out to cache line so we can always use SIMD
// throws exceptions in case of failure
// first element of the pair is a string (null terminated)
// whereas the second element is the length.
// caller is responsible to free (free std::pair<u8 *, size_t>.first)
std::pair<u8 *, size_t> get_corpus(std::string filename);

#endif
