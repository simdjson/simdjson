#ifndef SIMDJSON_JSONIOUTIL_H
#define SIMDJSON_JSONIOUTIL_H

#include "simdjson/common_defs.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


// low-level function to allocate memory with padding so we can read passed the "length" bytes
// safely.
// if you must provide a pointer to some data, create it with this function:
// length is the max. size in bytes of the string
// caller is responsible to free the memory (free(...))
char * allocate_padded_buffer(size_t length);





// load a file in memory...
// get a corpus; pad out to cache line so we can always use SIMD
// throws exceptions in case of failure
// first element of the pair is a string (null terminated)
// whereas the second element is the length.
// caller is responsible to free (aligned_free((void*)result.data())))
// 
// throws an exception if the file cannot be opened, use try/catch
//      try {
//        p = get_corpus(filename);
//      } catch (const std::exception& e) { 
//        aligned_free((void*)p.data());
//        std::cout << "Could not load the file " << filename << std::endl;
//      }
std::string_view  get_corpus(const std::string& filename);


#endif
