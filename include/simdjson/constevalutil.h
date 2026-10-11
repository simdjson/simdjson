#ifndef SIMDJSON_CONSTEVALUTIL_H
#define SIMDJSON_CONSTEVALUTIL_H

#include <string>
#include <string_view>
#include <array>

namespace simdjson {
namespace constevalutil {
#if SIMDJSON_CONSTEVAL

constexpr static std::array<uint8_t, 256> json_quotable_character = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
constexpr static std::array<std::string_view, 32> control_chars = {
    "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006",
    "\\u0007", "\\b", "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",
    "\\u000e", "\\u000f", "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014",
    "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b",
    "\\u001c", "\\u001d", "\\u001e", "\\u001f"};
// unoptimized, meant for compile-time execution
consteval std::string consteval_to_quoted_escaped(std::string_view input) {
  std::string out = "\"";
  for (char c : input) {
    if (json_quotable_character[uint8_t(c)]) {
      if (c == '"') {
        out.append("\\\"");
      } else if (c == '\\') {
        out.append("\\\\");
      } else {
        std::string_view v = control_chars[uint8_t(c)];
        out.append(v);
      }
    } else {
      out.push_back(c);
    }
  }
  out.push_back('"');
  return out;
}
#endif  // SIMDJSON_CONSTEVAL


#if SIMDJSON_SUPPORTS_CONCEPTS
template <size_t N>
struct fixed_string {
    constexpr fixed_string(const char (&str)[N])  {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
    }
    constexpr fixed_string(const unsigned char (&str)[N])  {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = static_cast<char>(str[i]);
        }
    }
    char data[N];
    constexpr std::string_view view() const { return {data, N - 1}; }
    constexpr size_t size() const { return N ; }
    constexpr operator std::string_view() const { return view(); }
    constexpr char operator[](std::size_t index) const { return data[index]; }
    constexpr bool operator==(const fixed_string& other) const {
        if (N != other.size()) {
            return false;
        }
        for (std::size_t i = 0; i < N; ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }
};
template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <fixed_string str>
struct string_constant {
    static constexpr std::string_view value = str.view();
};

#if SIMDJSON_CONSTEVAL
consteval std::size_t quoted_escaped_size(std::string_view input) {
  return consteval_to_quoted_escaped(input).size();
}

template <fixed_string str>
consteval auto make_quoted_escaped() {
  std::array<char, quoted_escaped_size(str.view())> out{};
  std::string escaped = consteval_to_quoted_escaped(str.view());
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = escaped[i];
  }
  return out;
}

// JSON-escaped and quoted form of str.
template <fixed_string str>
inline constexpr auto quoted_escaped = make_quoted_escaped<str>();
#endif // SIMDJSON_CONSTEVAL
#endif // SIMDJSON_SUPPORTS_CONCEPTS

} // namespace constevalutil
} // namespace simdjson
#endif // SIMDJSON_CONSTEVALUTIL_H
