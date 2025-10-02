/**
 * Compile-time JSON Path accessor using C++26 reflection
 * This file provides functionality to pre-compile JSON paths at compile time
 * and generate accessor code using reflection.
 */
#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_PATH_ACCESSOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_PATH_ACCESSOR_H

#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

#include <string_view>
#include <cstddef>
#include <array>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
namespace json_path {

// Note: value type must be fully defined before this header is included
// This is ensured by including this in amalgamated.h after value-inl.h

using ::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value;

// Concept: Indexable container that is not a string or associative container
// Accepts: std::vector, std::array, std::deque (have operator[], value_type, not string_like)
// Rejects: std::string (string_like), std::list (no operator[]), std::map (has key_type)
template<typename Container>
concept indexable_container = requires {
  typename Container::value_type;
  requires !concepts::string_like<Container>;
  requires !requires { typename Container::key_type; };  // Reject maps/sets
  requires requires(Container& c, std::size_t i) {
    { c[i] } -> std::convertible_to<typename Container::value_type>;
  };
};

// Variable template to use with std::meta::substitute
template<typename Container>
constexpr bool indexable_container_v = indexable_container<Container>;

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

private:
  // Recursive template to generate compile-time accessor code
  // PathPos parameter is the position in the path string (compile-time constant)
  template<std::size_t PathPos>
  static inline simdjson_result<value> access_impl(simdjson_result<value> current) noexcept {
    if (current.error()) return current;

    // Base case: if we've consumed the entire path, return current value
    if constexpr (PathPos >= path_view.size()) {
      return current;
    } else if constexpr (path_view[PathPos] == '.') {
      // Field access - extract field name at compile time
      constexpr auto field_info = parse_next_field(PathPos);
      constexpr std::string_view field_name = std::get<0>(field_info);
      constexpr std::size_t next_pos = std::get<1>(field_info);

      // Generate field access code
      auto obj_result = current.get_object();
      if (obj_result.error()) return obj_result.error();

      auto obj = obj_result.value_unsafe();
      auto next_value = obj.find_field_unordered(field_name);

      // Recursively process next step at compile time
      return access_impl<next_pos>(next_value);

    } else if constexpr (path_view[PathPos] == '[') {
      // Array or bracket notation
      constexpr auto bracket_info = parse_bracket(PathPos);
      constexpr bool is_field = std::get<0>(bracket_info);
      constexpr std::size_t next_pos = std::get<2>(bracket_info);

      if constexpr (is_field) {
        // Field access with bracket notation
        constexpr std::string_view field_name = std::get<1>(bracket_info);

        auto obj_result = current.get_object();
        if (obj_result.error()) return obj_result.error();

        auto obj = obj_result.value_unsafe();
        auto next_value = obj.find_field_unordered(field_name);

        return access_impl<next_pos>(next_value);

      } else {
        // Array index access
        constexpr std::size_t index = std::get<3>(bracket_info);

        auto arr_result = current.get_array();
        if (arr_result.error()) return arr_result.error();

        auto arr = arr_result.value_unsafe();
        auto next_value = arr.at(index);

        return access_impl<next_pos>(next_value);
      }
    } else {
      // Skip unexpected characters and continue
      return access_impl<PathPos + 1>(current);
    }
  }

  // Helper: Parse next field name at compile time
  static consteval auto parse_next_field(std::size_t start) {
    std::size_t i = start + 1; // skip '.'
    std::size_t field_start = i;
    while (i < path_view.size() && path_view[i] != '.' && path_view[i] != '[') {
      ++i;
    }
    std::string_view field_name = path_view.substr(field_start, i - field_start);
    return std::make_tuple(field_name, i);
  }

  // Helper: Parse bracket notation at compile time
  // Returns: (is_field, field_name, next_pos, index)
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

private:
  // Helper: Check if a type has a member with given name using reflection
  template<typename Type>
  static consteval bool has_member(std::string_view member_name) {
#if SIMDJSON_STATIC_REFLECTION
    constexpr auto members = std::meta::nonstatic_data_members_of(
      ^^Type, std::meta::access_context::unchecked()
    );

    for (auto mem : members) {
      std::string_view name = std::meta::identifier_of(mem);
      if (name == member_name) {
        return true;
      }
    }
#endif
    return false;
  }

  // Helper: Get type of member by name using reflection
  template<typename Type>
  static consteval auto get_member_type(std::string_view member_name) {
#if SIMDJSON_STATIC_REFLECTION
    constexpr auto members = std::meta::nonstatic_data_members_of(
      ^^Type, std::meta::access_context::unchecked()
    );

    for (auto mem : members) {
      std::string_view name = std::meta::identifier_of(mem);
      if (name == member_name) {
        return std::meta::type_of(mem);
      }
    }
#endif
    return ^^void; // Return void if not found
  }

  // Helper: Check if type represents a JSON array (indexable sequence container)
  //
  // Rationale:
  // - We're validating JSON path semantics: path[index] requires subscript access
  // - JSON arrays are ordered sequences with numeric indexed access
  // - Runtime JSON parsing uses operator[] for array element access
  //
  // Requirements:
  // 1. Must support operator[](size_t) for indexed access
  // 2. Must represent a sequence (have value_type)
  // 3. Must NOT be a string (strings are JSON strings, not arrays)
  // 4. Must NOT be associative (maps/sets have different JSON semantics)
  //
  // Helper to check if a reflected type satisfies the indexable_container concept
  // We use std::meta::substitute to evaluate the concept against a reflected type
  static consteval bool is_array_like_reflected(std::meta::info type_reflection) {
#if SIMDJSON_STATIC_REFLECTION
    // C-style arrays
    if (std::meta::is_array_type(type_reflection)) {
      return true;
    }

    // Test if the reflected type satisfies our indexable_container concept
    // substitute evaluates indexable_container_v<T> where T is the reflected type
    if (std::meta::can_substitute(^^indexable_container_v, {type_reflection})) {
      return std::meta::extract<bool>(std::meta::substitute(^^indexable_container_v, {type_reflection}));
    }
    return false;
#endif
    return false;
  }

  // Helper: Get element type from reflected array-like type
  static consteval std::meta::info get_element_type_reflected(std::meta::info type_reflection) {
#if SIMDJSON_STATIC_REFLECTION
    // Check for C-style arrays first using reflection predicates
    if (std::meta::is_array_type(type_reflection)) {
      // For C-style arrays (e.g., int[10]), extract element type using std::meta::remove_extent
      return std::meta::remove_extent(type_reflection);
    }

    // Look for value_type member in the reflected type (standard containers)
    auto members = std::meta::members_of(type_reflection, std::meta::access_context::unchecked());
    for (auto mem : members) {
      if (std::meta::is_type(mem)) {
        auto name = std::meta::identifier_of(mem);
        if (name == "value_type") {
          // Return the reflected type of value_type
          return mem;
        }
      }
    }
#endif
    return ^^void;
  }

  // Helper: Check if a non-reflected type is array-like (for template metaprogramming)
  template<typename Type>
  static consteval bool is_container_type() {
    using BaseType = std::remove_cvref_t<Type>;

    // Has value_type (std::vector, std::array, std::list, etc.)
    if constexpr (requires { typename BaseType::value_type; }) {
      return true;
    }

    // C-style array
    if constexpr (std::is_array_v<BaseType>) {
      return true;
    }

    return false;
  }

  // Helper: Extract element type from non-reflected container
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

  // Validate that the path matches the struct definition using reflection
  static consteval bool validate_path() {
#if SIMDJSON_STATIC_REFLECTION
    if constexpr (!std::is_class_v<T>) {
      // If T is void or not a class, we can't validate - allow it
      return true;
    }

    auto current_type = ^^T;
    std::size_t i = parser.skip_root();

    while (i < path_view.size()) {
      if (path_view[i] == '.') {
        // Field access - validate member exists
        ++i;
        std::size_t field_start = i;
        while (i < path_view.size() && path_view[i] != '.' && path_view[i] != '[') {
          ++i;
        }

        std::string_view field_name = path_view.substr(field_start, i - field_start);

        // Check if current type has this member
        bool found = false;
        auto members = std::meta::nonstatic_data_members_of(
          current_type, std::meta::access_context::unchecked()
        );

        for (auto mem : members) {
          if (std::meta::identifier_of(mem) == field_name) {
            current_type = std::meta::type_of(mem);
            found = true;
            break;
          }
        }

        if (!found) {
          return false; // Member not found
        }

      } else if (path_view[i] == '[') {
        ++i;
        if (i >= path_view.size()) return false;

        if (path_view[i] == '"' || path_view[i] == '\'') {
          // Field access with bracket notation
          char quote = path_view[i];
          ++i;
          std::size_t field_start = i;
          while (i < path_view.size() && path_view[i] != quote) {
            ++i;
          }

          std::string_view field_name = path_view.substr(field_start, i - field_start);
          if (i < path_view.size()) ++i; // skip closing quote
          if (i < path_view.size() && path_view[i] == ']') ++i;

          // Check if current type has this member
          bool found = false;
          auto members = std::meta::nonstatic_data_members_of(
            current_type, std::meta::access_context::unchecked()
          );

          for (auto mem : members) {
            if (std::meta::identifier_of(mem) == field_name) {
              current_type = std::meta::type_of(mem);
              found = true;
              break;
            }
          }

          if (!found) {
            return false; // Member not found
          }

        } else {
          // Array index - verify current type is array-like and extract element type
          while (i < path_view.size() && path_view[i] >= '0' && path_view[i] <= '9') {
            ++i;
          }

          if (i < path_view.size() && path_view[i] == ']') ++i;

          // Check if current type is array-like
          if (!is_array_like_reflected(current_type)) {
            return false; // Not an array/container type
          }

          // Extract element type and continue validation
          auto new_type = get_element_type_reflected(current_type);

          // If we couldn't extract element type (returns ^^void), fail validation
          if (new_type == ^^void) {
            return false; // Could not determine element type
          }

          current_type = new_type;
        }
      } else {
        ++i;
      }
    }

    return true; // Path validated successfully
#else
    return true; // No reflection available, allow everything
#endif
  }
};

// User-facing API: compile-time path accessor
// When used with a struct type T, validates the path at compile time
// Example: at_path_compiled<User, ".name">(doc)
template<typename T, constevalutil::fixed_string Path, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_path_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = path_accessor<T, Path>;
  return accessor::access(doc_or_val);
}

// Convenience overload without type parameter (no validation, just compile-time parsing)
// Example: at_path_compiled<".name">(doc)
template<constevalutil::fixed_string Path, typename DocOrValue>
inline simdjson_result<::simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> at_path_compiled(DocOrValue& doc_or_val) noexcept {
  using accessor = path_accessor<void, Path>;
  return accessor::access(doc_or_val);
}

} // namespace json_path
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_PATH_ACCESSOR_H
// Updated: Clean concept-based implementation

