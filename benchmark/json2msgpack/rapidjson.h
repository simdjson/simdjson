

#pragma once
#if SIMDJSON_EXCEPTIONS

#include "json2msgpack.h"

namespace json2msgpack {

using namespace rapidjson;

template <int parseflag>
struct rapidjson2msgpack {

  inline std::string_view to_msgpack(const char *json, uint8_t *buf);

private:
  inline void write_double(const double d) noexcept;
  inline void write_byte(const uint8_t b) noexcept;
  inline void write_uint32(const uint32_t w) noexcept;
  inline uint8_t *skip_uint32() noexcept;
  inline void write_uint32_at(const uint32_t w, uint8_t *p) noexcept;
  void write_string(const std::string &str) noexcept;
  inline void recursive_processor(const rapidjson::Value &v);

  rapidjson::Document doc{};
  uint8_t *buff{};
};

template <int parseflag>
std::string_view rapidjson2msgpack<parseflag>::to_msgpack(const char *json, uint8_t *buf) {
  buff = buf;
  doc.Parse<parseflag>(json);
  recursive_processor(doc);
  return std::string_view(reinterpret_cast<char *>(buf), size_t(buff - buf));
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_double(const double d) noexcept {
  *buff++ = 0xcb;
  ::memcpy(buff, &d, sizeof(d));
  buff += sizeof(d);
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_byte(const uint8_t b) noexcept {
  *buff = b;
  buff++;
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_string(const std::string &str) noexcept {
  write_byte(0xdb);
  write_uint32(str.size());
  ::memcpy(buff, str.data(), str.size());
  buff += str.size();
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_uint32(const uint32_t w) noexcept {
  ::memcpy(buff, &w, sizeof(w));
  buff += sizeof(w);
}

template <int parseflag>
uint8_t *rapidjson2msgpack<parseflag>::skip_uint32() noexcept {
  uint8_t *ret = buff;
  buff += sizeof(uint32_t);
  return ret;
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_uint32_at(const uint32_t w, uint8_t *p) noexcept {
  ::memcpy(p, &w, sizeof(w));
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::recursive_processor(const rapidjson::Value &v) {
  switch (v.GetType()) {

  case kArrayType:
    write_byte(0xdd);
    write_uint32(v.Size());
    for (Value::ConstValueIterator i = v.Begin(); i != v.End(); ++i) {
      recursive_processor(*i);
    }
    break;

  case kObjectType:
    write_byte(0xdf);
    write_uint32(v.Size());
    for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      write_string(m->name.GetString());
      recursive_processor(m->value);
    }
    break;

  case kStringType:
    write_string(v.GetString());
    break;
  case kNumberType:
    write_double(v.GetDouble());
    break;
  case kFalseType:
    write_byte(0xc2);
    break;
  case kTrueType:
    write_byte(0xc3);
    break;
  case kNullType:
    write_byte(0xc0);
    break;
  }
}

template <int parseflag>
struct rapidjson {
  using StringType = std::string_view;

  rapidjson2msgpack<parseflag> parser{};

  bool run(simdjson::padded_string &json, char *buffer,
           std::string_view &result) {
    result =
        parser.to_msgpack(json.data(), reinterpret_cast<uint8_t *>(buffer));

    return true;
  }
};

using rapidjson_lossless = rapidjson<kParseValidateEncodingFlag|kParseFullPrecisionFlag>;

BENCHMARK_TEMPLATE(json2msgpack, rapidjson_lossless)->UseManualTime();

} // namespace json2msgpack

#endif // SIMDJSON_EXCEPTIONS