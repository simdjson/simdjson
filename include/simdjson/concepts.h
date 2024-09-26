#ifndef SIMDJSON_CONCEPTS_H
#define SIMDJSON_CONCEPTS_H
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#ifdef __cpp_concepts
// Deliberately empty. In the future, if the library needs to define concepts, it
// can be done here.
namespace simdjson {
namespace concepts {
} // namespace concepts
} // namespace simdjson
#endif

#endif