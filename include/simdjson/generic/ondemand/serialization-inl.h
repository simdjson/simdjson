

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {



template <class serializer>
inline void string_builder<serializer>::append(document& element) {
  switch (element.type()) {
    case ondemand::json_type::array:
      append(array(element));
      break;
    case ondemand::json_type::object:
      append(object(element));
      break;
    case ondemand::json_type::number:
      // Assume it fits in a double. We do not detect integer types. This could be improved.
      format.number(double(element));
      break;
    case ondemand::json_type::string:
      format.string(std::string_view(element));
      break;
    case ondemand::json_type::boolean:
      bool(element) ? format.true_atom() : format.false_atom();
      break;
    case ondemand::json_type::null:
      format.null_atom();
      break;
  }}

template <class serializer>
inline void string_builder<serializer>::append(value element) {
  switch (element.type()) {
    case ondemand::json_type::array:
      append(array(element));
      break;
    case ondemand::json_type::object:
      append(object(element));
      break;
    case ondemand::json_type::number:
      // Assume it fits in a double. We do not detect integer types. This could be improved.
      format.number(double(element));
      break;
    case ondemand::json_type::string:
      format.string(std::string_view(element));
      break;
    case ondemand::json_type::boolean:
      bool(element) ? format.true_atom() : format.false_atom();
      break;
    case ondemand::json_type::null:
      format.null_atom();
      break;
  }
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::field x) {
  // Performance note: There is a sizeable performance opportunity here to avoid unescaping
  // and the re-escaping the key!!!!
  format.key(x.unescaped_key());
  append(x.value());
}


template <class serializer>
inline void string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array x) {
  format.start_array();
  bool first{true};
  for(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value element: x) {
    if(first) { first = false; }  else { format.comma(); };
    append(element);
  }
  format.end_array();
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object x) {
  format.start_object();
  bool first{true};
  for(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::field element: x) {
    if(first) { first = false; }  else { format.comma(); };
    append(element);
  }
  format.end_object();
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
