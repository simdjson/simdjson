#ifndef SIMDJSON_STD_MEMORY_H
#define SIMDJSON_STD_MEMORY_H

#ifndef SIMDJSON_SUPPORTS_DESERIALIZATION
#include "simdjson/generic/ondemand/deserialize.h"
#endif

#ifdef SIMDJSON_SUPPORTS_DESERIALIZATION
#include <memory>

namespace simdjson {

/**
 * This CPO (Customization Point Object) will help deserialize into
 * `unique_ptr`s.
 *
 * If constructing T is nothrow, this conversion should be nothrow as well since
 * we return MEMALLOC if we're not able to allocate memory instead of throwing
 * the the error message.
 *
 * @tparam T The type inside the unique_ptr
 * @tparam Deleter The Deleter of the unique_ptr
 * @tparam ValT document/value type
 * @param val document/value
 * @param out output unique_ptr
 * @return status of the conversion
 */
template <typename T, typename Deleter, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      std::unique_ptr<T, Deleter>
                          &out) noexcept(nothrow_deserializable<T, ValT>) {

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<T, ValT>,
      "The specified type inside the unique_ptr must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<T>,
      "The specified type inside the unique_ptr must default constructible.");

  auto ptr = new (std::nothrow) T();
  if (ptr == nullptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.template get<T>(*ptr));
  out.reset(ptr);
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

#endif // SIMDJSON_STD_MEMORY_H
