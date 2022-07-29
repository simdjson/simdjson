

#pragma once
#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "json2msgpack.h"

namespace json2msgpack {

using namespace rapidjson;

template <int parseflag>
struct rapidjson2msgpack {
  inline std::string_view to_msgpack(char *json, uint8_t *buf);

private:
  inline void write_double(const double d) noexcept;
  inline void write_byte(const uint8_t b) noexcept;
  inline void write_uint32(const uint32_t w) noexcept;
  inline void write_uint32_at(const uint32_t w, uint8_t *p) noexcept;
  void write_string(const char * s, size_t length) noexcept;
  inline void recursive_processor(Value &v);

  uint8_t *buff{};
};

template <int parseflag>
std::string_view rapidjson2msgpack<parseflag>::to_msgpack(char *json, uint8_t *buf) {
  buff = buf;
  Document doc{};
  if(parseflag & kParseInsituFlag) {
    doc.ParseInsitu<parseflag>(json);
  } else {
    doc.Parse<parseflag>(json);
  }
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
void rapidjson2msgpack<parseflag>::write_string(const char * c, size_t len) noexcept {
  write_byte(0xdb);
  write_uint32(uint32_t(len));
  ::memcpy(buff, c, len);
  buff += len;
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_uint32(const uint32_t w) noexcept {
  ::memcpy(buff, &w, sizeof(w));
  buff += sizeof(w);
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::write_uint32_at(const uint32_t w, uint8_t *p) noexcept {
  ::memcpy(p, &w, sizeof(w));
}

template <int parseflag>
void rapidjson2msgpack<parseflag>::recursive_processor(Value &v) {
  switch (v.GetType()) {
  case kArrayType:
    write_byte(0xdd);
    write_uint32(v.Size());
    for (Value::ValueIterator i = v.Begin(); i != v.End(); ++i) {
      recursive_processor(*i);
    }
    break;
  case kObjectType:
    write_byte(0xdf);
    write_uint32(uint32_t(v.MemberEnd()-v.MemberBegin()));
    for (Value::MemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      write_string(m->name.GetString(), m->name.GetStringLength());
      recursive_processor(m->value);
    }
    break;
  case kStringType:
    write_string(v.GetString(), v.GetStringLength());
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
struct rapidjson_base {
  using StringType = std::string_view;

  rapidjson2msgpack<parseflag> parser{};

  bool run(simdjson::padded_string &json, char *buffer,
           std::string_view &result) {
    result =
        parser.to_msgpack(json.data(), reinterpret_cast<uint8_t *>(buffer));

    return true;
  }
};


using rapidjson = rapidjson_base<kParseValidateEncodingFlag|kParseFullPrecisionFlag>;

BENCHMARK_TEMPLATE(json2msgpack, rapidjson)->UseManualTime();

#if SIMDJSON_COMPETITION_ONDEMAND_APPROX
using rapidjson_approx = rapidjson_base<kParseValidateEncodingFlag>;

BENCHMARK_TEMPLATE(json2msgpack, rapidjson_approx)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_APPROX

#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
using rapidjson_insitu = rapidjson_base<kParseValidateEncodingFlag|kParseInsituFlag>;

BENCHMARK_TEMPLATE(json2msgpack, rapidjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace json2msgpack

#endif // SIMDJSON_COMPETITION_RAPIDJSON
