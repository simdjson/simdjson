#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "json2msgpack.h"

namespace json2msgpack {

namespace jsonifier_api = ::jsonifier::generic;

class msgpack_string_buffer {
public:
  using size_type = uint64_t;
  using reference = char &;
  using const_reference = const char &;

  explicit simdjson_inline msgpack_string_buffer(uint8_t *location) noexcept
      : start{reinterpret_cast<char *>(location)} {}

  simdjson_inline reference operator[](size_type index) noexcept { return start[index]; }
  simdjson_inline char *data() noexcept { return start; }
  simdjson_inline size_type size() const noexcept { return length; }
  simdjson_inline void resize(size_type new_length) noexcept { length = new_length; }

private:
  char *start;
  size_type length{};
};

struct jsonifier2msgpack {
  inline bool to_msgpack(const simdjson::padded_string &json, uint8_t *buf, std::string_view &result);

private:
  simdjson_inline void write_double(const double d) noexcept;
  simdjson_inline void write_byte(const uint8_t b) noexcept;
  simdjson_inline void write_uint32(const uint32_t w) noexcept;
  simdjson_inline uint8_t *skip_uint32() noexcept;
  simdjson_inline void write_uint32_at(const uint32_t w, uint8_t *p) noexcept;
  simdjson_inline void write_key(std::string_view key) noexcept;
  simdjson_inline bool write_string(const jsonifier_api::value &element);
  simdjson_inline bool write_element(const jsonifier_api::value &element);
  inline bool write_container(const jsonifier_api::value &element, ::jsonifier::json_type type);

  ::jsonifier::jsonifier_core<> parser{};
  uint8_t *buff{};
};

bool jsonifier2msgpack::to_msgpack(const simdjson::padded_string &json, uint8_t *buf, std::string_view &result) {
  buff = buf;
  jsonifier_api::document doc = parser.iterate(json);
  if (!write_element(doc) || !doc.atEnd()) { return false; }
  result = std::string_view(reinterpret_cast<char *>(buf), size_t(buff - buf));
  return true;
}

void jsonifier2msgpack::write_double(const double d) noexcept {
  *buff++ = 0xcb;
  ::memcpy(buff, &d, sizeof(d));
  buff += sizeof(d);
}

void jsonifier2msgpack::write_byte(const uint8_t b) noexcept {
  *buff = b;
  buff++;
}

void jsonifier2msgpack::write_uint32(const uint32_t w) noexcept {
  ::memcpy(buff, &w, sizeof(w));
  buff += sizeof(w);
}

uint8_t *jsonifier2msgpack::skip_uint32() noexcept {
  uint8_t *ret = buff;
  buff += sizeof(uint32_t);
  return ret;
}

void jsonifier2msgpack::write_uint32_at(const uint32_t w, uint8_t *p) noexcept {
  ::memcpy(p, &w, sizeof(w));
}

void jsonifier2msgpack::write_key(std::string_view key) noexcept {
  write_byte(0xdb);
  write_uint32(uint32_t(key.size()));
  ::memcpy(buff, key.data(), key.size());
  buff += key.size();
}

bool jsonifier2msgpack::write_string(const jsonifier_api::value &element) {
  write_byte(0xdb);
  uint8_t *location = skip_uint32();
  msgpack_string_buffer output{buff};
  if (element.getString(output) != jsonifier_api::error_code::success) { return false; }
  write_uint32_at(uint32_t(output.size()), location);
  buff += output.size();
  return true;
}

bool jsonifier2msgpack::write_element(const jsonifier_api::value &element) {
  const ::jsonifier::json_type type = element.type();
  switch (type) {
  case ::jsonifier::json_type::string:
    return write_string(element);
  case ::jsonifier::json_type::number: {
    double value{};
    if (element.getDouble(value) != jsonifier_api::error_code::success) { return false; }
    write_double(value);
    return true;
  }
  case ::jsonifier::json_type::boolean: {
    bool value{};
    if (element.getBool(value) != jsonifier_api::error_code::success) { return false; }
    write_byte(uint8_t(0xc2 + value));
    return true;
  }
  case ::jsonifier::json_type::null:
    if (!element.isNull()) { return false; }
    write_byte(0xc0);
    return true;
  default:
    return write_container(element, type);
  }
}

bool jsonifier2msgpack::write_container(const jsonifier_api::value &element, ::jsonifier::json_type type) {
  uint32_t counter = 0;
  if (type == ::jsonifier::json_type::array) {
    jsonifier_api::array array = element.getArray();
    write_byte(0xdd);
    uint8_t *location = skip_uint32();
    for (jsonifier_api::value child : array) {
      counter++;
      if (!write_element(child)) { return false; }
    }
    write_uint32_at(counter, location);
    return true;
  }
  if (type == ::jsonifier::json_type::object) {
    jsonifier_api::object object = element.getObject();
    write_byte(0xdf);
    uint8_t *location = skip_uint32();
    for (jsonifier_api::field field : object) {
      counter++;
      std::string_view key;
      if (field.error() != jsonifier_api::error_code::success) { return false; }
      if (field.unescapedKey(key) != jsonifier_api::error_code::success) { return false; }
      write_key(key);
      if (!write_element(field.value())) { return false; }
    }
    write_uint32_at(counter, location);
    return true;
  }
  return false;
}

struct jsonifier_generic : jsonifier2msgpack {
  bool run(simdjson::padded_string &json, char *buffer, std::string_view &result) {
    return to_msgpack(json, reinterpret_cast<uint8_t *>(buffer), result);
  }
};

BENCHMARK_TEMPLATE(json2msgpack, jsonifier_generic)->UseManualTime();

} // namespace json2msgpack

#endif // SIMDJSON_COMPETITION_JSONIFIER
