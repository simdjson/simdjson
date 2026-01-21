/**
 * Compile-time JSON Path and JSON Pointer accessors using C++26 reflection (P2996)
 *
 * This file validates JSON paths/pointers against struct definitions at compile time
 * and generates optimized accessor code with zero runtime overhead.
 *
 * ## How It Works
 *
 * **Compile Time**: Path is parsed, validated against struct, types are checked
 * **Runtime**: Direct navigation with no parsing or validation overhead
 *
 * Example:
 * ```cpp
 * struct User { std::string name; std::vector<std::string> emails; };
 *
 * std::string email;
 * path_accessor<User, ".emails[0]">::extract_field(doc, email);
 *
 * // Compile time validates:
 * // 1. User has "emails" field
 * // 2. "emails" is array-like
 * // 3. Element type is std::string
 * // 4. static_assert(^^std::string == ^^std::string)
 *
 * // Runtime just navigates:
 * // doc.get_object().find_field("emails").get_array().at(0).get(email)
 * ```
 *
 * ## Key Reflection APIs
 *
 * - `^^Type`: Reflect operator, converts type to std::meta::info
 * - `std::meta::nonstatic_data_members_of(type)`: Get all fields of a struct
 * - `std::meta::identifier_of(member)`: Get field name as string_view
 * - `std::meta::type_of(member)`: Get reflected type of a field
 * - `std::meta::is_array_type(type)`: Check if C-style array
 * - `std::meta::remove_extent(array)`: Extract element type from array
 * - `std::meta::members_of(type)`: Get all members including typedefs
 * - `std::meta::is_type(member)`: Check if member is a type (vs field)
 *
 * All operations execute at compile time in consteval contexts.
 */
#ifndef SIMDJSON_GENERIC_ONDEMAND_COMPILE_TIME_ACCESSORS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_COMPILE_TIME_ACCESSORS_H

#endif // SIMDJSON_CONDITIONAL_INCLUDE

// Arguably, we should just check SIMDJSON_STATIC_REFLECTION since it
// is unlikely that we will have reflection support without concepts support.
#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

#include <string_view>
#include <cstddef>
#include <array>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
/***
 * JSONPath implementation for compile-time access
 * RFC 9535 JSONPath: Query Expressions for JSON, https://www.rfc-editor.org/rfc/rfc9535
 */
namespace json_path {

// Note: value type must be fully defined before this header is included
// This is ensured by including this in amalgamated.h after value-inl.h

using ::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value;

// Path step types
enum class step_type {
  field,        // .field_name or ["field_name"]
  array_index   // [index]
};

// Represents a single step in a JSON path
template<std::size_t N>
struct path_step {
  step_type type;
  char key[N];  // Field name (empty for array indices)
  std::size_t index;  // Array index (0 for field access)

  constexpr path_step(step_type t, const char (&k)[N], std::size_t idx = 0)
    : type(t), index(idx) {
    for (std::size_t i = 0; i < N; ++i) {
      key[i] = k[i];
    }
  }

  constexpr std::string_view key_view() const {
    return {key, N - 1};
  }
};

// Helper to create field step
template<std::size_t N>
consteval auto make_field_step(const char (&name)[N]) {
  return path_step<N>(step_type::field, name, 0);
}

// Helper to create array index step
consteval auto make_index_step(std::size_t idx) {
  return path_step<1>(step_type::array_index, "", idx);
}

// Parse state for compile-time JSON path parsing
struct parse_result {
  bool success;
  std::size_t pos;
  std::string_view error_msg;
};

// Compile-time JSON path parser
// Supports subset: .field, ["field"], [index], nested combinations
template<constevalutil::fixed_string Path>
struct json_path_parser {
  static constexpr std::string_view path_str = Path.view();

  // Skip leading $ if present
  static consteval std::size_t skip_root() {
    if (!path_str.empty() && path_str[0] == '$') {
      return 1;
    }
    return 0;
  }

  // Count the number of steps in the path at compile time
  static consteval std::size_t count_steps() {
    std::size_t count = 0;
    std::size_t i = skip_root();

    while (i < path_str.size()) {
      if (path_str[i] == '.') {
        // Field access: .field
        ++i;
        if (i >= path_str.size()) break;

        // Skip field name
        while (i < path_str.size() && path_str[i] != '.' && path_str[i] != '[') {
          ++i;
        }
        ++count;
      } else if (path_str[i] == '[') {
        // Array or bracket notation
        ++i;
        if (i >= path_str.size()) break;

        if (path_str[i] == '"' || path_str[i] == '\'') {
          // Field access: ["field"] or ['field']
          char quote = path_str[i];
          ++i;
          while (i < path_str.size() && path_str[i] != quote) {
            ++i;
          }
          if (i < path_str.size()) ++i; // skip closing quote
          if (i < path_str.size() && path_str[i] == ']') ++i;
        } else {
          // Array index: [0], [123]
          while (i < path_str.size() && path_str[i] != ']') {
            ++i;
          }
          if (i < path_str.size()) ++i; // skip ]
        }
        ++count;
      } else {
        ++i;
      }
    }

    return count;
  }

  // Parse a field name at compile time
  static consteval std::size_t parse_field_name(std::size_t start, char* out, std::size_t max_len) {
    std::size_t len = 0;
    std::size_t i = start;

    while (i < path_str.size() && path_str[i] != '.' && path_str[i] != '[' && len < max_len - 1) {
      out[len++] = path_str[i++];
    }
    out[len] = '\0';
    return i;
  }

  // Parse an array index at compile time
  static consteval std::pair<std::size_t, std::size_t> parse_array_index(std::size_t start) {
    std::size_t index = 0;
    std::size_t i = start;

    while (i < path_str.size() && path_str[i] >= '0' && path_str[i] <= '9') {
      index = index * 10 + (path_str[i] - '0');
      ++i;
    }

    return {i, index};
  }
};

// Compile-time path accessor generator
template<typename T, constevalutil::fixed_string Path>
struct path_accessor {
  using value = ::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value;

  static constexpr auto parser = json_path_parser<Path>();
  static constexpr std::size_t num_steps = parser.count_steps();
  static constexpr std::string_view path_view = Path.view();

  // Compile-time accessor generation
  // If T is a struct, validates the path at compile time
  // If T is void, skips validation
  template<typename DocOrValue>
  static inline simdjson_result<value> access(DocOrValue& doc_or_val) noexcept {
    // Validate path at compile time if T is a struct
    if constexpr (std::is_class_v<T>) {
      constexpr bool path_valid = validate_path();
      static_assert(path_valid, "JSON path does not match struct definition");
    }

    // Parse the path at compile time to build access steps
    return access_impl<parser.skip_root()>(doc_or_val.get_value());
  }

  // Extract value at path directly into target with compile-time type validation
  // Example: std::string name; path_accessor<User, ".name">::extract_field(doc, name);
  template<typename DocOrValue, typename FieldType>
  static inline error_code extract_field(DocOrValue& doc_or_val, FieldType& target) noexcept {
    static_assert(std::is_class_v<T>, "extract_field requires T to be a struct type for validation");

    // Validate path exists in struct definition
    constexpr bool path_valid = validate_path();
    static_assert(path_valid, "JSON path does not match struct definition");

    // Get the type at the end of the path
    constexpr auto final_type = get_final_type();

    // Verify target type matches the field type
    static_assert(final_type == ^^FieldType, "Target type does not match the field type at the path");

    // All validation done at compile time - just navigate and extract
    auto json_value = access_impl<parser.skip_root()>(doc_or_val.get_value());
    if (json_value.error()) return json_value.error();

    return json_value.get(target);
  }

private:
  // Get the final type by walking the path through the struct type
  template<typename U = T>
  static consteval std::enable_if_t<std::is_class_v<U>, std::meta::info> get_final_type() {
    auto current_type = ^^T;
    std::size_t i = parser.skip_root();

    while (i < path_view.size()) {
      if (path_view[i] == '.') {
        // .field syntax
        ++i;
        std::size_t field_start = i;
        while (i < path_view.size() && path_view[i] != '.' && path_view[i] != '[') {
          ++i;
        }

        std::string_view field_name = path_view.substr(field_start, i - field_start);

        auto members = std::meta::nonstatic_data_members_of(
          current_type, std::meta::access_context::unchecked()
        );

        for (auto mem : members) {
          if (std::meta::identifier_of(mem) == field_name) {
            current_type = std::meta::type_of(mem);
            break;
          }
        }

      } else if (path_view[i] == '[') {
        ++i;
        if (i >= path_view.size()) break;

        if (path_view[i] == '"' || path_view[i] == '\'') {
          // ["field"] syntax
          char quote = path_view[i];
          ++i;
          std::size_t field_start = i;
          while (i < path_view.size() && path_view[i] != quote) {
            ++i;
          }

          std::string_view field_name = path_view.substr(field_start, i - field_start);
          if (i < path_view.size()) ++i; // skip quote
          if (i < path_view.size() && path_view[i] == ']') ++i;

          auto members = std::meta::nonstatic_data_members_of(
            current_type, std::meta::access_context::unchecked()
          );

          for (auto mem : members) {
            if (std::meta::identifier_of(mem) == field_name) {
              current_type = std::meta::type_of(mem);
              break;
            }
          }

        } else {
          // [index] syntax - extract element type
          while (i < path_view.size() && path_view[i] >= '0' && path_view[i] <= '9') {
            ++i;
          }
          if (i < path_view.size() && path_view[i] == ']') ++i;

          current_type = get_element_type_reflected(current_type);
        }
      } else {
        ++i;
      }
    }

    return current_type;
  }

private:
  // Walk path and extract directly into final field using compile-time reflection
  template<std::meta::info CurrentType, std::size_t PathPos, typename TargetType>
  static inline error_code extract_with_reflection(simdjson_result<value> current, TargetType& target_ref) noexcept {
    if (current.error()) return current.error();

    // Base case: end of path - extract into target
    if constexpr (PathPos >= path_view.size()) {
      return current.get(target_ref);
    }
    // Field access: .field_name
    else if constexpr (path_view[PathPos] == '.') {
      constexpr auto field_info = parse_next_field(PathPos);
      constexpr std::string_view field_name = std::get<0>(field_info);
      constexpr std::size_t next_pos = std::get<1>(field_info);

      constexpr auto member_info = find_member_by_name(CurrentType, field_name);
      static_assert(member_info != ^^void, "Field not found in struct");

      constexpr auto member_type = std::meta::type_of(member_info);

      auto obj_result = current.get_object();
      if (obj_result.error()) return obj_result.error();
      auto obj = obj_result.value_unsafe();
      auto field_value = obj.find_field_unordered(field_name);

      if constexpr (next_pos >= path_view.size()) {
        return field_value.get(target_ref);
      } else {
        return extract_with_reflection<member_type, next_pos>(field_value, target_ref);
      }
    }
    // Bracket notation: [index] or ["field"]
    else if constexpr (path_view[PathPos] == '[') {
      constexpr auto bracket_info = parse_bracket(PathPos);
      constexpr bool is_field = std::get<0>(bracket_info);
      constexpr std::size_t next_pos = std::get<2>(bracket_info);

      if constexpr (is_field) {
        constexpr std::string_view field_name = std::get<1>(bracket_info);
        constexpr auto member_info = find_member_by_name(CurrentType, field_name);
        static_assert(member_info != ^^void, "Field not found in struct");
        constexpr auto member_type = std::meta::type_of(member_info);

        auto obj_result = current.get_object();
        if (obj_result.error()) return obj_result.error();
        auto obj = obj_result.value_unsafe();
        auto field_value = obj.find_field_unordered(field_name);

        if constexpr (next_pos >= path_view.size()) {
          return field_value.get(target_ref);
        } else {
          return extract_with_reflection<member_type, next_pos>(field_value, target_ref);
        }
      } else {
        constexpr std::size_t index = std::get<3>(bracket_info);
        constexpr auto elem_type = get_element_type_reflected(CurrentType);
        static_assert(elem_type != ^^void, "Could not determine array element type");

        auto arr_result = current.get_array();
        if (arr_result.error()) return arr_result.error();
        auto arr = arr_result.value_unsafe();
        auto elem_value = arr.at(index);

        if constexpr (next_pos >= path_view.size()) {
          return elem_value.get(target_ref);
        } else {
          return extract_with_reflection<elem_type, next_pos>(elem_value, target_ref);
        }
      }
    }
    // Skip unexpected characters and continue
    else {
      return extract_with_reflection<CurrentType, PathPos + 1>(current, target_ref);
    }
  }

  // Find member by name in reflected type
  static consteval std::meta::info find_member_by_name(std::meta::info type_refl, std::string_view name) {
    auto members = std::meta::nonstatic_data_members_of(type_refl, std::meta::access_context::unchecked());
    for (auto mem : members) {
      if (std::meta::identifier_of(mem) == name) {
        return mem;
      }
    }
  }

  // Generate compile-time accessor code by walking the path
  template<std::size_t PathPos>
  static inline simdjson_result<value> access_impl(simdjson_result<value> current) noexcept {
    if (current.error()) return current;

    if constexpr (PathPos >= path_view.size()) {
      return current;
    } else if constexpr (path_view[PathPos] == '.') {
      constexpr auto field_info = parse_next_field(PathPos);
      constexpr std::string_view field_name = std::get<0>(field_info);
      constexpr std::size_t next_pos = std::get<1>(field_info);

      auto obj_result = current.get_object();
      if (obj_result.error()) return obj_result.error();

      auto obj = obj_result.value_unsafe();
      auto next_value = obj.find_field_unordered(field_name);

      return access_impl<next_pos>(next_value);

    } else if constexpr (path_view[PathPos] == '[') {
      constexpr auto bracket_info = parse_bracket(PathPos);
      constexpr bool is_field = std::get<0>(bracket_info);
      constexpr std::size_t next_pos = std::get<2>(bracket_info);

      if constexpr (is_field) {
        constexpr std::string_view field_name = std::get<1>(bracket_info);

        auto obj_result = current.get_object();
        if (obj_result.error()) return obj_result.error();

        auto obj = obj_result.value_unsafe();
        auto next_value = obj.find_field_unordered(field_name);

        return access_impl<next_pos>(next_value);

      } else {
        constexpr std::size_t index = std::get<3>(bracket_info);

        auto arr_result = current.get_array();
        if (arr_result.error()) return arr_result.error();

        auto arr = arr_result.value_unsafe();
        auto next_value = arr.at(index);

        return access_impl<next_pos>(next_value);
      }
    } else {
      return access_impl<PathPos + 1>(current);
    }
  }

  // Parse next field name
  static consteval auto parse_next_field(std::size_t start) {
    std::size_t i = start + 1;
    std::size_t field_start = i;
    while (i < path_view.size() && path_view[i] != '.' && path_view[i] != '[') {
      ++i;
    }
    std::string_view field_name = path_view.substr(field_start, i - field_start);
    return std::make_tuple(field_name, i);
  }

  // Parse bracket notation: returns (is_field, field_name, next_pos, index)
  static consteval auto parse_bracket(std::size_t start) {
    std::size_t i = start + 1; // skip '['

    if (i < path_view.size() && (path_view[i] == '"' || path_view[i] == '\'')) {
      // Field access
      char quote = path_view[i];
      ++i;
      std::size_t field_start = i;
      while (i < path_view.size() && path_view[i] != quote) {
        ++i;
      }
      std::string_view field_name = path_view.substr(field_start, i - field_start);
      if (i < path_view.size()) ++i; // skip closing quote
      if (i < path_view.size() && path_view[i] == ']') ++i;

      return std::make_tuple(true, field_name, i, std::size_t(0));
    } else {
      // Array index
      std::size_t index = 0;
      while (i < path_view.size() && path_view[i] >= '0' && path_view[i] <= '9') {
        index = index * 10 + (path_view[i] - '0');
        ++i;
      }
      if (i < path_view.size() && path_view[i] == ']') ++i;

      return std::make_tuple(false, std::string_view{}, i, index);
    }
  }

public:
  // Check if reflected type is array-like (C-style array or indexable container)
  // Uses reflection to test: 1) std::meta::is_array_type() for C arrays
  //                          2) std::meta::substitute() to test concepts::indexable_container concept
  static consteval bool is_array_like_reflected(std::meta::info type_reflection) {
    if (std::meta::is_array_type(type_reflection)) {
      return true;
    }

    if (std::meta::can_substitute(^^concepts::indexable_container_v, {type_reflection})) {
      return std::meta::extract<bool>(std::meta::substitute(^^concepts::indexable_container_v, {type_reflection}));
    }
    return false;
  }

  // Extract element type from reflected array or container
  // For C arrays: uses std::meta::remove_extent()
  // For containers: finds value_type member using std::meta::members_of()
  static consteval std::meta::info get_element_type_reflected(std::meta::info type_reflection) {
    if (std::meta::is_array_type(type_reflection)) {
      return std::meta::remove_extent(type_reflection);
    }

    auto members = std::meta::members_of(type_reflection, std::meta::access_context::unchecked());
    for (auto mem : members) {
      if (std::meta::is_type(mem)) {
        auto name = std::meta::identifier_of(mem);
        if (name == "value_type") {
          return mem;
        }
      }
    }
    return ^^void;
  }

private:
  // Check if type has member with given name
  template<typename Type>
  static consteval bool has_member(std::string_view member_name) {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Type, std::meta::access_context::unchecked());
    for (auto mem : members) {
      if (std::meta::identifier_of(mem) == member_name) {
        return true;
      }
    }
    return false;
  }

  // Get type of member by name
  template<typename Type>
  static consteval auto get_member_type(std::string_view member_name) {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Type, std::meta::access_context::unchecked());
    for (auto mem : members) {
      if (std::meta::identifier_of(mem) == member_name) {
        return std::meta::type_of(mem);
      }
    }
    return ^^void;
  }

  // Check if non-reflected type is array-like
  template<typename Type>
  static consteval bool is_container_type() {
    using BaseType = std::remove_cvref_t<Type>;
    if constexpr (requires { typename BaseType::value_type; }) {
      return true;
    }
    if constexpr (std::is_array_v<BaseType>) {
      return true;
    }
    return false;
  }

  // Extract element type from non-reflected container
  template<typename Type>
  using extract_element_type = std::conditional_t<
    requires { typename std::remove_cvref_t<Type>::value_type; },
    typename std::remove_cvref_t<Type>::value_type,
    std::conditional_t<
      std::is_array_v<std::remove_cvref_t<Type>>,
      std::remove_extent_t<std::remove_cvref_t<Type>>,
      void
    >
  >;

  // Validate path matches struct definition
  static consteval bool validate_path() {
    if constexpr (!std::is_class_v<T>) {
      return true;
    }

    auto current_type = ^^T;
    std::size_t i = parser.skip_root();

    while (i < path_view.size()) {
      if (path_view[i] == '.') {
        ++i;
        std::size_t field_start = i;
        while (i < path_view.size() && path_view[i] != '.' && path_view[i] != '[') {
          ++i;
        }

        std::string_view field_name = path_view.substr(field_start, i - field_start);

        bool found = false;
        auto members = std::meta::nonstatic_data_members_of(current_type, std::meta::access_context::unchecked());

        for (auto mem : members) {
          if (std::meta::identifier_of(mem) == field_name) {
            current_type = std::meta::type_of(mem);
            found = true;
            break;
          }
        }

        if (!found) {
          return false;
        }

      } else if (path_view[i] == '[') {
        ++i;
        if (i >= path_view.size()) return false;

        if (path_view[i] == '"' || path_view[i] == '\'') {
          char quote = path_view[i];
          ++i;
          std::size_t field_start = i;
          while (i < path_view.size() && path_view[i] != quote) {
            ++i;
          }

          std::string_view field_name = path_view.substr(field_start, i - field_start);
          if (i < path_view.size()) ++i;
          if (i < path_view.size() && path_view[i] == ']') ++i;

          bool found = false;
          auto members = std::meta::nonstatic_data_members_of(current_type, std::meta::access_context::unchecked());

          for (auto mem : members) {
            if (std::meta::identifier_of(mem) == field_name) {
              current_type = std::meta::type_of(mem);
              found = true;
              break;
            }
          }

          if (!found) {
            return false;
          }

        } else {
          while (i < path_view.size() && path_view[i] >= '0' && path_view[i] <= '9') {
            ++i;
          }

          if (i < path_view.size() && path_view[i] == ']') ++i;

          if (!is_array_like_reflected(current_type)) {
            return false;
          }

          auto new_type = get_element_type_reflected(current_type);

          if (new_type == ^^void) {
            return false;
          }

          current_type = new_type;
        }
      } else {
        ++i;
      }
    }

    return true;
  }
};

// Compile-time path accessor with validation
template<typename T, constevalutil::fixed_string Path, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_path_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = path_accessor<T, Path>;
  return accessor::access(doc_or_val);
}

// Overload without type parameter (no validation)
template<constevalutil::fixed_string Path, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_path_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = path_accessor<void, Path>;
  return accessor::access(doc_or_val);
}

// ============================================================================
// JSON Pointer Compile-Time Support (RFC 6901)
// ============================================================================

// JSON Pointer parser: /field/0/nested (slash-separated)
template<constevalutil::fixed_string Pointer>
struct json_pointer_parser {
  static constexpr std::string_view pointer_str = Pointer.view();

  // Unescape token: ~0 -> ~, ~1 -> /
  static consteval void unescape_token(std::string_view src, char* dest, std::size_t& out_len) {
    out_len = 0;
    for (std::size_t i = 0; i < src.size(); ++i) {
      if (src[i] == '~' && i + 1 < src.size()) {
        if (src[i + 1] == '0') {
          dest[out_len++] = '~';
          ++i;
        } else if (src[i + 1] == '1') {
          dest[out_len++] = '/';
          ++i;
        } else {
          dest[out_len++] = src[i];
        }
      } else {
        dest[out_len++] = src[i];
      }
    }
  }

  // Check if token is numeric
  static consteval bool is_numeric(std::string_view token) {
    if (token.empty()) return false;
    if (token[0] == '0' && token.size() > 1) return false;
    for (char c : token) {
      if (c < '0' || c > '9') return false;
    }
    return true;
  }

  // Parse numeric token to index
  static consteval std::size_t parse_index(std::string_view token) {
    std::size_t result = 0;
    for (char c : token) {
      result = result * 10 + (c - '0');
    }
    return result;
  }

  // Count tokens in pointer
  static consteval std::size_t count_tokens() {
    if (pointer_str.empty() || pointer_str == "/") return 0;

    std::size_t count = 0;
    std::size_t pos = pointer_str[0] == '/' ? 1 : 0;

    while (pos < pointer_str.size()) {
      ++count;
      std::size_t next_slash = pointer_str.find('/', pos);
      if (next_slash == std::string_view::npos) break;
      pos = next_slash + 1;
    }

    return count;
  }

  // Get Nth token
  static consteval std::string_view get_token(std::size_t token_index) {
    std::size_t pos = pointer_str[0] == '/' ? 1 : 0;
    std::size_t current_token = 0;

    while (current_token < token_index) {
      std::size_t next_slash = pointer_str.find('/', pos);
      pos = next_slash + 1;
      ++current_token;
    }

    std::size_t token_end = pointer_str.find('/', pos);
    if (token_end == std::string_view::npos) token_end = pointer_str.size();

    return pointer_str.substr(pos, token_end - pos);
  }
};

// JSON Pointer accessor
template<typename T, constevalutil::fixed_string Pointer>
struct pointer_accessor {
  using parser = json_pointer_parser<Pointer>;
  static constexpr std::string_view pointer_view = Pointer.view();
  static constexpr std::size_t token_count = parser::count_tokens();

  // Validate pointer against struct definition
  static consteval bool validate_pointer() {
    if constexpr (!std::is_class_v<T>) {
      return true;
    }

    auto current_type = ^^T;
    std::size_t pos = pointer_view[0] == '/' ? 1 : 0;

    while (pos < pointer_view.size()) {
      // Extract token up to next /
      std::size_t token_end = pointer_view.find('/', pos);
      if (token_end == std::string_view::npos) token_end = pointer_view.size();

      std::string_view token = pointer_view.substr(pos, token_end - pos);

      if (parser::is_numeric(token)) {
        if (!path_accessor<T, Pointer>::is_array_like_reflected(current_type)) {
          return false;
        }
        current_type = path_accessor<T, Pointer>::get_element_type_reflected(current_type);
      } else {
        bool found = false;
        auto members = std::meta::nonstatic_data_members_of(current_type, std::meta::access_context::unchecked());

        for (auto mem : members) {
          if (std::meta::identifier_of(mem) == token) {
            current_type = std::meta::type_of(mem);
            found = true;
            break;
          }
        }

        if (!found) return false;
      }

      pos = token_end + 1;
    }

    return true;
  }

  // Recursive accessor
  template<std::size_t TokenIndex>
  static inline simdjson_result<value> access_impl(simdjson_result<value> current) noexcept {
    if constexpr (TokenIndex >= token_count) {
      return current;
    } else {
      constexpr std::string_view token = parser::get_token(TokenIndex);

      if constexpr (parser::is_numeric(token)) {
        constexpr std::size_t index = parser::parse_index(token);
        auto arr = current.get_array().value_unsafe();
        auto next_value = arr.at(index);
        return access_impl<TokenIndex + 1>(next_value);
      } else {
        auto obj = current.get_object().value_unsafe();
        auto next_value = obj.find_field_unordered(token);
        return access_impl<TokenIndex + 1>(next_value);
      }
    }
  }

  // Access JSON value at pointer
  template<typename DocOrValue>
  static inline simdjson_result<value> access(DocOrValue& doc_or_val) noexcept {
    if constexpr (std::is_class_v<T>) {
      constexpr bool pointer_valid = validate_pointer();
      static_assert(pointer_valid, "JSON Pointer does not match struct definition");
    }

    if (pointer_view.empty() || pointer_view == "/") {
      if constexpr (requires { doc_or_val.get_value(); }) {
        return doc_or_val.get_value();
      } else {
        return doc_or_val;
      }
    }

    simdjson_result<value> current = doc_or_val.get_value();
    return access_impl<0>(current);
  }

  // Extract value at pointer directly into target with type validation
  template<typename DocOrValue, typename FieldType>
  static inline error_code extract_field(DocOrValue& doc_or_val, FieldType& target) noexcept {
    static_assert(std::is_class_v<T>, "extract_field requires T to be a struct type for validation");

    constexpr bool pointer_valid = validate_pointer();
    static_assert(pointer_valid, "JSON Pointer does not match struct definition");

    constexpr auto final_type = get_final_type();
    static_assert(final_type == ^^FieldType, "Target type does not match the field type at the pointer");

    simdjson_result<value> current_value = doc_or_val.get_value();
    auto json_value = access_impl<0>(current_value);
    if (json_value.error()) return json_value.error();

    return json_value.get(target);
  }

private:
  // Get final type by walking pointer through struct
  template<typename U = T>
  static consteval std::enable_if_t<std::is_class_v<U>, std::meta::info> get_final_type() {
    auto current_type = ^^T;
    std::size_t pos = pointer_view[0] == '/' ? 1 : 0;

    while (pos < pointer_view.size()) {
      std::size_t token_end = pointer_view.find('/', pos);
      if (token_end == std::string_view::npos) token_end = pointer_view.size();

      std::string_view token = pointer_view.substr(pos, token_end - pos);

      if (parser::is_numeric(token)) {
        current_type = path_accessor<T, "">::get_element_type_reflected(current_type);
      } else {
        auto members = std::meta::nonstatic_data_members_of(current_type, std::meta::access_context::unchecked());

        for (auto mem : members) {
          if (std::meta::identifier_of(mem) == token) {
            current_type = std::meta::type_of(mem);
            break;
          }
        }
      }

      pos = token_end + 1;
    }

    return current_type;
  }
};

// Compile-time JSON Pointer accessor with validation
template<typename T, constevalutil::fixed_string Pointer, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = pointer_accessor<T, Pointer>;
  return accessor::access(doc_or_val);
}

// Overload without type parameter (no validation)
template<constevalutil::fixed_string Pointer, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = pointer_accessor<void, Pointer>;
  return accessor::access(doc_or_val);
}

} // namespace json_path
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_ONDEMAND_COMPILE_TIME_ACCESSORS_H

