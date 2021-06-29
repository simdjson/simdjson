

namespace simdjson {

inline std::string_view trim(const std::string_view str) {
    // We can almost surely do better by rolling our own find_first_not_of function.
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string_view ::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

inline simdjson_result<std::string_view> trim(simdjson_result<std::string_view> x) {
    if (x.error()) { return x; }
    return trim(x.value());
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::document& x) {
  using namespace SIMDJSON_IMPLEMENTATION::ondemand;
  json_type t;
  auto error = x.type().get(t);
  if(error != SUCCESS) { return error; }
  switch (t)
  {
    case json_type::array:
      return to_json_string(x.get_array());
    case json_type::object:
      return to_json_string(x.get_object());
    default:
      return trim(x.raw_json_token());
  }
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::value& x) {
  using namespace SIMDJSON_IMPLEMENTATION::ondemand;
  SIMDJSON_IMPLEMENTATION::ondemand::json_type t;
  auto error = x.type().get(t);
  if(error != SUCCESS) { return error; }
  switch (t)
  {
    case json_type::array:
      return to_json_string(x.get_array());
    case json_type::object:
      return to_json_string(x.get_object());
    default:
      return trim(x.raw_json_token());
  }
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::object& x) {
    return trim(x.raw_json_token());
}

inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::array& x) {
    return trim(x.raw_json_token());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> x) {
    if (x.error()) { return x.error(); }
    return to_json_string(x.value());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> x) {
    if (x.error()) { return x.error(); }
    return to_json_string(x.value());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> x) {
    if (x.error()) { return x.error(); }
    return to_json_string(x.value());
}

inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> x) {
    if (x.error()) { return x.error(); }
    return to_json_string(x.value());
}
} // namespace simdjson


#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value x) {
    std::string_view v;
    auto error = simdjson::to_json_string(x).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      throw simdjson::simdjson_error(error);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value x) {
    std::string_view v;
    auto error = simdjson::to_json_string(x).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      return (out << error);
    }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array value) {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      throw simdjson::simdjson_error(error);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array value) {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      return (out << error);
    }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& value)  {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      throw simdjson::simdjson_error(error);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& value)  {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      return (out << error);
    }
}
#endif

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object value) {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      throw simdjson::simdjson_error(error);
    }
}
inline std::ostream& operator<<(std::ostream& out,  simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object> x) {
    if (x.error()) { throw  simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object value) {
    std::string_view v;
    auto error = simdjson::to_json_string(value).get(v);
    if(error == simdjson::SUCCESS) {
      return (out << v);
    } else {
      return (out << error);
    }
}
#endif
