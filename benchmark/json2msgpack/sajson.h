#pragma once
#ifdef SIMDJSON_COMPETITION_SAJSON

#include "json2msgpack.h"

namespace json2msgpack {

using namespace sajson;


struct sajson2msgpack {
  inline std::string_view to_msgpack(char *json, size_t size, uint8_t *buf);
  virtual ~sajson2msgpack() { free(ast_buffer); }

private:
  inline void write_double(const double d) noexcept;
  inline void write_byte(const uint8_t b) noexcept;
  inline void write_uint32(const uint32_t w) noexcept;
  inline void write_string(const char * s, size_t length) noexcept;
  inline void recursive_processor(const sajson::value &v);

  uint8_t *buff{};
  size_t ast_buffer_size{0};
  size_t *ast_buffer{nullptr};
};


std::string_view sajson2msgpack::to_msgpack(char *json, size_t size, uint8_t *buf) {
  buff = buf;

  if (!ast_buffer) {
    ast_buffer_size = size;
    ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
  }
  auto doc = parse(
      bounded_allocation(ast_buffer, ast_buffer_size),
      mutable_string_view(size, json)
  );

  auto root = doc.get_root();
  recursive_processor(root);
  return std::string_view(reinterpret_cast<char *>(buf), size_t(buff - buf));
}

void sajson2msgpack::write_string(const char * c, size_t len) noexcept {
  write_byte(0xdb);
  write_uint32(uint32_t(len));
  ::memcpy(buff, c, len);
  buff += len;
}

void sajson2msgpack::write_double(const double d) noexcept {
  *buff++ = 0xcb;
  ::memcpy(buff, &d, sizeof(d));
  buff += sizeof(d);
}

void sajson2msgpack::write_byte(const uint8_t b) noexcept {
  *buff = b;
  buff++;
}

void sajson2msgpack::write_uint32(const uint32_t w) noexcept {
  ::memcpy(buff, &w, sizeof(w));
  buff += sizeof(w);
}

void sajson2msgpack::recursive_processor(const sajson::value &node) {
  using namespace sajson;
  switch (node.get_type()) {
  case TYPE_NULL:
    write_byte(0xc0);
    break;
  case TYPE_FALSE:
    write_byte(0xc2);
    break;
  case TYPE_TRUE:
    write_byte(0xc3);
    break;
  case TYPE_ARRAY: {
    auto length = node.get_length();
    write_byte(0xdf);
    write_uint32(uint32_t(length));
    for (size_t i = 0; i < length; ++i) {
      recursive_processor(node.get_array_element(i));
    }
    break;
  }
  case TYPE_OBJECT: {
    auto length = node.get_length();
    write_byte(0xdd);
    write_uint32(uint32_t(length));
    for (auto i = 0u; i < length; ++i) {
      auto s = node.get_object_key(i);
      write_string(s.data(), s.length());
      recursive_processor(node.get_object_value(i));
    }
    break;
  }
  case TYPE_STRING:
    write_string(node.as_cstring(), node.get_string_length());
    break;
  case TYPE_DOUBLE:
  case TYPE_INTEGER:
    write_double(node.get_number_value());
    break;
  default:
    assert(false && "unknown node type");
  }
}


struct sajson {
  using StringType = std::string_view;

  sajson2msgpack parser{};

  bool run(simdjson::padded_string &json, char *buffer,
           std::string_view &result) {
    result =
        parser.to_msgpack(json.data(), json.size(), reinterpret_cast<uint8_t *>(buffer));

    return true;
  }
};

BENCHMARK_TEMPLATE(json2msgpack, sajson)->UseManualTime();

} // namespace json2msgpack

#endif // SIMDJSON_COMPETITION_SAJSON
