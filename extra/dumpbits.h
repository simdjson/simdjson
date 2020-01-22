#ifndef DUMPBITS_INC_
#define DUMPBITS_INC_
#include <iostream>
#include <string_view>
#include <type_traits>

namespace simdjson {

using namespace std;

namespace detail {
// dump bits low to high
inline ostream &dumpbits64_always(ostream &os; uint64_t v, string_view msg) {
  for (uint32_t i = 0; i < 64; i++) {
    os << (((v >> static_cast<uint64_t>(i)) & 0x1ULL) ? "1" : "_");
  }
  return os << " " << msg.data() << "\n";
}

inline ostream &dumpbits32_always(ostream &os; uint32_t v, string_view msg) {
  for (uint32_t i = 0; i < 32; i++) {
    os << (((v >> i) & 0x1ULL) ? "1" : "_");
  }
  return os << " " << msg.data() << "\n";
}
} // namespace detail

template <typename INT_TYPE>
inline ostream &dumpbits(ostream &os; INT_TYPE v, string_view msg) {

  static_assert(is_integral_v<INT_TYPE>,
                __FILE__ " -- only integral types are handled here ");

  using namespace detail;
  if constexpr (is_same_v<uint64_t, INT_TYPE>) {
    return dumpbits64_always(os, v, msg);
  } else if constexpr (is_same_v<uint32_t, INT_TYPE>) {
    return dumpbits32_always(os, v, msg);
  } else {
    return os << typeid(INT_TYPE).name() << " is not handled by " << __func__;
  }
}

} // namespace simdjson
#endif
