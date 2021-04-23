

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {



template <class serializer>
inline simdjson::error_code string_builder<serializer>::append(document& element) noexcept {
  json_type t;
  auto e = element.type().get(t);
  if(e != simdjson::SUCCESS) { return e; }
  switch (t) {
    case ondemand::json_type::array:
      {
        array x;
        simdjson::error_code error = element.get_array().get(x);
        if(error == simdjson::SUCCESS) {
          append(x);
        }
        return error;
      }
    case ondemand::json_type::object:
      {
        object x;
        simdjson::error_code error = element.get_object().get(x);
        if(error == simdjson::SUCCESS) {
          append(x);
        }
        return error;
      }
    case ondemand::json_type::number:
      // Assume it fits in a double. We do not detect integer types. This could be improved.
      {
        double x;
        simdjson::error_code error = element.get_double().get(x);
        if(error == simdjson::SUCCESS) {
          format.number(x);
        }
        return error;
      }
    case ondemand::json_type::string:
      {
        std::string_view x;
        simdjson::error_code error = element.get_string().get(x);
        if(error == simdjson::SUCCESS) {
          format.string(x);
        }
        return error;
      }
    case ondemand::json_type::boolean:
      {
        bool x;
        simdjson::error_code error = element.get_bool().get(x);
        if(error == simdjson::SUCCESS) {
          x ? format.true_atom() : format.false_atom();
        }
        return error;
      }
    case ondemand::json_type::null:
      format.null_atom();
      return simdjson::SUCCESS;
  }
  return simdjson::INCORRECT_TYPE;
}

template <class serializer>
inline simdjson::error_code string_builder<serializer>::append(value element) noexcept {
  json_type t;
  auto e = element.type().get(t);
  if(e != simdjson::SUCCESS) { return e; }
  switch (t) {
    case ondemand::json_type::array:
      {
        array x;
        simdjson::error_code error = element.get_array().get(x);
        if(error == simdjson::SUCCESS) {
          append(x);
        }
        return error;
      }
    case ondemand::json_type::object:
      {
        object x;
        simdjson::error_code error = element.get_object().get(x);
        if(error == simdjson::SUCCESS) {
          append(x);
        }
        return error;
      }
    case ondemand::json_type::number:
      // Assume it fits in a double. We do not detect integer types. This could be improved.
      {
        double x;
        simdjson::error_code error = element.get_double().get(x);
        if(error == simdjson::SUCCESS) {
          format.number(x);
        }
        return error;
      }
    case ondemand::json_type::string:
      {
        std::string_view x;
        simdjson::error_code error = element.get_string().get(x);
        if(error == simdjson::SUCCESS) {
          format.string(x);
        }
        return error;
      }
      break;
    case ondemand::json_type::boolean:
      {
        bool x;
        simdjson::error_code error = element.get_bool().get(x);
        if(error == simdjson::SUCCESS) {
          x ? format.true_atom() : format.false_atom();
        }
        return error;
      }
    case ondemand::json_type::null:
      format.null_atom();
      return simdjson::SUCCESS;
  }
  return simdjson::INCORRECT_TYPE;
}

template <class serializer>
inline simdjson::error_code string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::field x) noexcept {
  // Performance note: There is a sizeable performance opportunity here to avoid unescaping
  // and the re-escaping the key!!!!
  std::string_view v;
  auto error = x.unescaped_key().get(v);
  if (error) { return error; }
  format.key(v);
  return append(x.value());
}


template <class serializer>
inline simdjson::error_code string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array x) noexcept {
  format.start_array();
  bool first{true};
  for(simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> v: x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value element;
    simdjson::error_code error = std::move(v).get(element);
    if(error != simdjson::SUCCESS) { return error; }
    if(first) { first = false; }  else { format.comma(); };
    error = append(element);
    if(error != simdjson::SUCCESS) { return error; }
  }
  format.end_array();
  return simdjson::SUCCESS;
}

template <class serializer>
inline simdjson::error_code string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object x) noexcept {
  format.start_object();
  bool first{true};
  for(simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::field> r: x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::field element;
    simdjson::error_code error = std::move(r).get(element);
    if(error != simdjson::SUCCESS) { return error; }
    if(first) { first = false; }  else { format.comma(); };
    error = append(element);
    if(error != simdjson::SUCCESS) { return error; }
  }
  format.end_object();
  return simdjson::SUCCESS;
}

template <class serializer>
simdjson_really_inline void string_builder<serializer>::clear() {
  format.clear();
}

template <class serializer>
simdjson_really_inline std::string_view string_builder<serializer>::str() const {
  return format.str();
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
