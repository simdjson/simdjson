#pragma once
#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "json2msgpack.h"

namespace json2msgpack {

using namespace nlohmann;

struct nlohmann_json2msgpack {
  inline std::string_view to_msgpack(const simdjson::padded_string &json,
                                     uint8_t *buf);

private:
  inline void write_double(const double d) noexcept;
  inline void write_byte(const uint8_t b) noexcept;
  inline void write_uint32(const uint32_t w) noexcept;
  inline void write_string(const std::string& str);
  inline void recursive_processor(basic_json<> element);

  uint8_t *buff{};
};

std::string_view nlohmann_json2msgpack::to_msgpack(const simdjson::padded_string &json,
                             uint8_t *buf) {
  buff = buf;
  auto val = nlohmann::json::parse(json.data(), json.data() + json.size());
  recursive_processor(val);
  return std::string_view(reinterpret_cast<char *>(buf), size_t(buff - buf));
}

void nlohmann_json2msgpack::write_double(const double d) noexcept {
  *buff++ = 0xcb;
  ::memcpy(buff, &d, sizeof(d));
  buff += sizeof(d);
}

void nlohmann_json2msgpack::write_byte(const uint8_t b) noexcept {
  *buff = b;
  buff++;
}

void nlohmann_json2msgpack::write_uint32(const uint32_t w) noexcept {
  ::memcpy(buff, &w, sizeof(w));
  buff += sizeof(w);
}

void nlohmann_json2msgpack::write_string(const std::string & str) {
  write_byte(0xdb);
  write_uint32(uint32_t(str.size()));
  ::memcpy(buff, str.data(), str.size());
  buff += str.size();
}

void nlohmann_json2msgpack::recursive_processor(json element) {
  switch (element.type()) {
  case nlohmann::detail::value_t::array: {
    uint32_t counter = 0;
    write_byte(0xdd);
    std::vector<json> array = element.get<std::vector<json>>();
    write_uint32(uint32_t(array.size()));
    for (auto child : array) {
       recursive_processor(child);
    }
  } break;
  case nlohmann::detail::value_t::object: {
    write_byte(0xdf);
    std::map<std::string,json> object = element.get<std::map<std::string,json>>();
    write_uint32(uint32_t(object.size()));
    for (auto field : object) {
      write_string(field.first);
      recursive_processor(field.second);
    }
  } break;

  case nlohmann::detail::value_t::number_integer:
  case nlohmann::detail::value_t::number_unsigned:
  case nlohmann::detail::value_t::number_float:
    write_double(double(element));
    break;
  case nlohmann::detail::value_t::string:
    write_string(std::string(element));
    break;
  case nlohmann::detail::value_t::boolean:
    write_byte(0xc2 + bool(element));
    break;
  case nlohmann::detail::value_t::null:
    write_byte(0xc0);
    break;
  case nlohmann::detail::value_t::discarded:
  case nlohmann::detail::value_t::binary:
  default:
    printf("unexpected\n");
    break;
  }
}

struct nlohmann_json {
  using StringType = std::string_view;

  nlohmann_json2msgpack parser{};

  bool run(simdjson::padded_string &json, char *buffer,
           std::string_view &result) {
    result = parser.to_msgpack(json, reinterpret_cast<uint8_t *>(buffer));
    return true;
  }
};

BENCHMARK_TEMPLATE(json2msgpack, nlohmann_json)->UseManualTime();

} // namespace json2msgpack

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON



