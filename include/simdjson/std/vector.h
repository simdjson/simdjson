#ifndef SIMDJSON_STD_VECTOR_H
#define SIMDJSON_STD_VECTOR_H

#ifndef SIMDJSON_SUPPORTS_DESERIALIZATION
#include "simdjson/generic/ondemand/deserialize.h"
#endif

#ifdef SIMDJSON_SUPPORTS_DESERIALIZATION
#include <vector>

namespace simdjson {

/**
 * STL containers have several constructors including one that takes a single
 * size argument. Thus, some compilers (Visual Studio) will not be able to
 * disambiguate between the size and container constructor. Users should
 * explicitly specify the type of the container as needed: e.g.,
 * doc.get<std::vector<int>>().
 */
template <typename T, typename AllocT, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      std::vector<T, AllocT> &out) noexcept(false) {

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<T, ValT>,
      "The specified type inside the vector must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<T>,
      "The specified type inside the vector must default constructible.");

  ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    if (auto const err = v.get<T>().get(out.emplace_back()); err) {
      // If an error occurs, the empty element that we just inserted gets
      // removed. We're not using a temp variable because if T is a heavy type,
      // we want the valid path to be the fast path and the slow path be the
      // path that has errors in it.
      static_cast<void>(out.pop_back());
      return err;
    }
  }
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

#endif // SIMDJSON_STD_VECTOR_H
