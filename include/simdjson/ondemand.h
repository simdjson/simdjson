#ifndef SIMDJSON_ONDEMAND_H
#define SIMDJSON_ONDEMAND_H

#include "simdjson/builtin/ondemand.h"

namespace simdjson {
  /**
   * @copydoc simdjson::builtin::ondemand
   */
  namespace ondemand = builtin::ondemand;
  /**
   * @copydoc simdjson::builtin::builder
   */
  namespace builder = builtin::builder;

#if SIMDJSON_STATIC_REFLECTION
  /**
   * Create a JSON string from any user-defined type using static reflection.
   * Only available when SIMDJSON_STATIC_REFLECTION is enabled.
   */
  template<typename T>
    requires(!std::same_as<T, builtin::ondemand::document> &&
             !std::same_as<T, builtin::ondemand::value> &&
             !std::same_as<T, builtin::ondemand::object> &&
             !std::same_as<T, builtin::ondemand::array>)
  inline std::string to_json_string(const T& obj) {
    builder::string_builder str_builder;
    append(str_builder, obj);
    std::string_view view;
    if (str_builder.view().get(view) == SUCCESS) {
      return std::string(view);
    }
    return "";
  }
#endif

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_H
