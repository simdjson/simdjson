#pragma once

#ifdef SIMDJSON_COMPETITION_BOOSTJSON

#include "json2msgpack.h"

namespace json2msgpack {

struct boostjson2msgpack {
  inline std::string_view to_msgpack(const boost::json::value &root, uint8_t *buf) {
    buff = buf;
    recursive_processor(root);
    return std::string_view(reinterpret_cast<char *>(buf), size_t(buff - buf));
  }

private:
  uint8_t *buff{};

  inline void write_double(const double d) noexcept {
    *buff++ = 0xcb;
    ::memcpy(buff, &d, sizeof(d));
    buff += sizeof(d);
  }

  inline void write_byte(const uint8_t b) noexcept {
    *buff = b;
    buff++;
  }

  inline void write_uint32(const uint32_t w) noexcept {
    ::memcpy(buff, &w, sizeof(w));
    buff += sizeof(w);
  }

  inline void write_string(const std::string & str) {
    write_byte(0xdb);
    write_uint32(uint32_t(str.size()));
    ::memcpy(buff, str.data(), str.size());
    buff += str.size();
  }

  inline void recursive_processor(const boost::json::value &element) {
    switch(element.kind()) {
    case boost::json::kind::array: {
      write_byte(0xdd);
      const auto &array = element.as_array();
      write_uint32(static_cast<uint32_t>(array.size()));
      for (const auto &child : array) {
        recursive_processor(child);
      }
    } break;

    case boost::json::kind::object: {
      write_byte(0xdf);
      const auto &object = element.as_object();
      write_uint32(static_cast<uint32_t>(object.size()));
      for (const auto &child : object) {
        write_string(child.key_c_str());
        recursive_processor(child.value());
      }
    } break;

    case boost::json::kind::int64:
    case boost::json::kind::uint64:
    case boost::json::kind::double_:
      write_double(element.to_number<double>());
      break;

    case boost::json::kind::string:
      write_string(element.as_string().c_str());
      break;

    case boost::json::kind::bool_:
      write_byte(0xc2 + element.as_bool());
      break;

    case boost::json::kind::null:
      write_byte(0xc0);
      break;

    default:
      printf("unexpected\n");
      break;
    }
  }
};

struct boostjson {
  using StringType=std::string;

  boostjson2msgpack parser{};

  bool run(simdjson::padded_string &json, char *buffer, std::string_view &result) {
    auto root = boost::json::parse(json);
    result = parser.to_msgpack(root, reinterpret_cast<uint8_t *>(buffer));
    return true;
  }
};

BENCHMARK_TEMPLATE(json2msgpack, boostjson)->UseManualTime();

} // namespace json2msgpack

#endif // SIMDJSON_COMPETITION_BOOSTJSON
