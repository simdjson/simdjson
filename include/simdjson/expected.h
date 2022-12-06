#ifndef SIMDJSON_EXPECTED_H
#define SIMDJSON_EXPECTED_H

//
// If user defines SIMDJSON_EXPECTED_TYPE and SIMDJSON_UNEXPECTED_TYPE then use
// those types otherwise use standard library or else use the bundled
// implementation from Sy Brand
//

#if defined(SIMDJSON_EXPECTED_TYPE) && defined(SIMDJSON_UNEXPECTED_TYPE)

namespace simdjson {
template <typename T, typename E> using expected = SIMDJSON_EXPECTED_TYPE<T, E>;
template <typename E> using unexpected = SIMDJSON_UNEXPECTED_TYPE<E>;
} // namespace simdjson

#elif __cpp_lib_expected >= 202202L

#include <expected>
namespace simdjson {
template <typename T, typename E> using expected = std::expected<T, E>;
template <typename E> using unexpected = std::unexpected<E>;
} // namespace simdjson

#else

#include "nonstd/expected.hpp"
namespace simdjson {
template <typename T, typename E> using expected = tl::expected<T, E>;
template <typename E> using unexpected = tl::unexpected<E>;
} // namespace simdjson
#endif

#endif