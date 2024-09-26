#ifndef SIMDJSON_STD_LIST_H
#define SIMDJSON_STD_LIST_H

#ifndef SIMDJSON_SUPPORTS_DESERIALIZATION
#include "simdjson/generic/ondemand/deserialize.h"
#endif

#ifdef SIMDJSON_SUPPORTS_DESERIALIZATION
#include <string>

namespace simdjson {

template <typename CharT,
          typename TraitsT,
          typename AllocT,
          typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val, std::basic_string<CharT, TraitsT, AllocT> &out) noexcept(false) {
  using string_type = std::basic_string<CharT, TraitsT, AllocT>;

  if constexpr (std::same_as<string_type, string_type>) {
    SIMDJSON_TRY(val.get_string(out));
  } else {
    // todo: optimize performance
    std::string tmp;
    SIMDJSON_TRY(val.get_string(tmp));
    for (auto const ch : tmp) {
      out.push_back(ch);
    }
  }
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

#endif // SIMDJSON_STD_LIST_H
