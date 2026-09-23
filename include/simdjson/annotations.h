#ifndef SIMDJSON_ANNOTATIONS_H
#define SIMDJSON_ANNOTATIONS_H

/**
 * @file annotations.h
 * @brief Provides compile-time annotations for simdjson structures.
 * This header defines annotations that can be applied to data members of structures
 * (and to the structures and enumerations themselves) to control how they are
 * serialized/deserialized with simdjson. The set of annotations is modelled after
 * the attributes of the Rust serde library.
 *
 * Member annotations:
 *
 *   [[= simdjson::rename<"name">]]             use "name" as the JSON key
 *   [[= simdjson::alias<"a", "b">]]            also accept "a" and "b" when deserializing
 *   [[= simdjson::skip]]                       never serialize nor deserialize
 *   [[= simdjson::skip_serializing]]           never serialize
 *   [[= simdjson::skip_deserializing]]         never deserialize (keeps its current value)
 *   [[= simdjson::skip_serializing_if<pred>]]  do not serialize when pred(value) is true
 *   [[= simdjson::default_value]]              a missing key is not an error
 *   [[= simdjson::default_from<factory>]]      a missing key sets the member to factory()
 *   [[= simdjson::with<Adapter>]]              custom (de)serialization via Adapter
 *   [[= simdjson::flatten]]                    inline the members of a nested structure
 *
 * Structure (container) annotations:
 *
 *   [[= simdjson::rename_all<simdjson::case_style::camel_case>]]  rename every member
 *   [[= simdjson::default_value]]              no missing key is an error
 *   [[= simdjson::deny_unknown_fields]]        unknown keys are a deserialization error
 *   [[= simdjson::transparent]]                (de)serialize as the single member
 *
 * Enumeration annotations: rename_all on the enumeration, rename and alias on the
 * enumerators (e.g., `enum class color { red [[= simdjson::rename<"RED">]] };`).
 *
 * This is currently experimental and subject to change (syntax and semantics may evolve).
 */

#if SIMDJSON_STATIC_REFLECTION

#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace simdjson {

// Structural compile-time string -- char array avoids the pointer-based
// 'reflect_constant failed' that occurs with const char* / string_view members.
template <size_t N>
struct fixed_string {
    char data[N];

    consteval fixed_string(const char (&s)[N]) noexcept {
        for (size_t i = 0; i < N; ++i) { data[i] = s[i]; }
    }

    consteval std::string_view view() const noexcept { return {data, N - 1}; }

    consteval bool operator==(const fixed_string&) const noexcept = default;
};

/**
 * Naming conventions for simdjson::rename_all, mirroring serde's rename_all.
 * Except for lowercase and uppercase, the C++ identifier is first split into
 * words at underscores and at case changes ("userId", "user_id" and "UserId" all
 * give the words "user" and "id"; "HTTPServer" gives "HTTP" and "Server"), and
 * the words are then joined according to the convention.
 */
enum class case_style {
  lowercase,            ///< every letter lowercased, nothing else changes: userId -> userid
  uppercase,            ///< every letter uppercased, nothing else changes: user_id -> USER_ID
  pascal_case,          ///< UserId
  camel_case,           ///< userId
  snake_case,           ///< user_id
  screaming_snake_case, ///< USER_ID
  kebab_case,           ///< user-id
  screaming_kebab_case  ///< USER-ID
};

namespace detail {
    template <fixed_string Name>
    struct rename_t {
        static constexpr auto name = Name;
        // Exposed as a pointer and a size: std::meta::extract requires structural types.
        static constexpr const char *key_data = Name.data;
        static constexpr size_t key_size = Name.view().size();
    };
    template <fixed_string... Names>
    struct alias_t {
        static_assert(sizeof...(Names) > 0, "simdjson::alias requires at least one name");
        static constexpr std::string_view keys[] = {Names.view()...};
        static constexpr const std::string_view *keys_data = keys;
        static constexpr size_t keys_count = sizeof...(Names);
    };
    struct skip_tag {};
    struct skip_serializing_tag {};
    struct skip_deserializing_tag {};
    template <auto Predicate>
    struct skip_serializing_if_t {
        static constexpr auto predicate = Predicate;
    };
    struct default_value_tag {};
    template <auto Factory>
    struct default_from_t {
        static constexpr auto factory = Factory;
    };
    template <typename Adapter>
    struct with_t {
        using adapter = Adapter;
    };
    template <case_style Style>
    struct rename_all_t {
        static constexpr case_style style = Style;
    };
    struct deny_unknown_fields_tag {};
    struct transparent_tag {};
    struct flatten_tag {};

    // Predicates usable with skip_serializing_if.
    struct is_none_t {
        template <typename T>
        constexpr bool operator()(const T& v) const noexcept { return !v; }
    };
    struct is_empty_t {
        template <typename T>
        constexpr bool operator()(const T& v) const noexcept { return v.empty(); }
    };
} // namespace detail

// Usage: [[= simdjson::rename<"first_name">]] std::string firstName;
template <fixed_string Name>
inline constexpr detail::rename_t<Name> rename{};

// Usage: [[= simdjson::alias<"userName", "login">]] std::string user_name;
// The aliases are accepted (in addition to the regular key) when deserializing.
// Serialization always uses the regular key. If the JSON object contains more
// than one of the names, which one is used is unspecified.
template <fixed_string... Names>
inline constexpr detail::alias_t<Names...> alias{};

// Usage: [[= simdjson::skip]] int internalCache;
inline constexpr detail::skip_tag skip{};

// Usage: [[= simdjson::skip_serializing]] std::string password;
inline constexpr detail::skip_serializing_tag skip_serializing{};

// Usage: [[= simdjson::skip_deserializing]] int computed;
// The member is never assigned during deserialization (it keeps its current
// value) and a matching key in the JSON input is treated as unknown.
inline constexpr detail::skip_deserializing_tag skip_deserializing{};

// Usage: [[= simdjson::skip_serializing_if<simdjson::is_none>]] std::optional<int> x;
// The predicate is called with the member value; when it returns true, the key
// is omitted from the output.
template <auto Predicate>
inline constexpr detail::skip_serializing_if_t<Predicate> skip_serializing_if{};

// Predicate: true for an empty std::optional, a null smart pointer, etc.
inline constexpr detail::is_none_t is_none{};
// Predicate: true for an empty string or container.
inline constexpr detail::is_empty_t is_empty{};

// Usage: [[= simdjson::default_value]] int port = 8080;
// When the key is missing from the JSON input, the member is left untouched
// (with get<T>(), it keeps its default member initializer) instead of reporting
// NO_SUCH_FIELD. Applied to a structure, it applies to all of its members.
inline constexpr detail::default_value_tag default_value{};

// Usage: [[= simdjson::default_from<make_port>]] int port;
// When the key is missing from the JSON input, the member is assigned the
// result of calling the factory (a constexpr callable taking no argument, such
// as a captureless lambda or a pointer to a function).
template <auto Factory>
inline constexpr detail::default_from_t<Factory> default_from{};

// Usage: [[= simdjson::with<unix_time>]] std::chrono::system_clock::time_point t;
// Adapter is a type that provides one or both of
//   static void serialize(simdjson::builder::string_builder &b, const T &value);
//   static simdjson::error_code deserialize(simdjson::ondemand::value &v, T &out);
// (the parameters may also be declared auto&). When one of them is missing, the
// default behaviour is used in that direction.
template <typename Adapter>
inline constexpr detail::with_t<Adapter> with{};

// Usage: [[= simdjson::flatten]] pagination page;
// The members of the nested structure are (de)serialized as if they were members
// of the enclosing structure: {"id":1,"limit":10,"offset":0} rather than
// {"id":1,"page":{"limit":10,"offset":0}}. The nested structure's own annotations
// (rename_all, default_value, ...) apply to its members.
inline constexpr detail::flatten_tag flatten{};

// Usage: struct [[= simdjson::rename_all<simdjson::case_style::camel_case>]] S {...};
// Also applies to enumerations. An explicit rename on a member takes precedence.
template <case_style Style>
inline constexpr detail::rename_all_t<Style> rename_all{};

// Usage: struct [[= simdjson::deny_unknown_fields]] S {...};
// Deserialization fails with UNKNOWN_FIELD when the JSON object has a key that
// is not deserialized into a member (including the keys of skipped members).
inline constexpr detail::deny_unknown_fields_tag deny_unknown_fields{};

// Usage: struct [[= simdjson::transparent]] user_id { int64_t value; };
// A structure with a single data member is (de)serialized as that member alone:
// user_id{42} becomes 42 rather than {"value":42}.
inline constexpr detail::transparent_tag transparent{};

namespace detail {

// True when the entity (a data member, an enumerator or a type) carries an
// annotation of type tag.
consteval bool has_annotation(std::meta::info entity, std::meta::info tag) {
  return !std::meta::annotations_of_with_type(entity, tag).empty();
}

// Returns the type of the (first) annotation of entity that is a specialization
// of the class template tmpl, or std::meta::info{} when there is none.
consteval std::meta::info annotation_of_template(std::meta::info entity, std::meta::info tmpl) {
  for (std::meta::info ann : std::meta::annotations_of(entity)) {
    std::meta::info type = std::meta::type_of(ann);
    if (std::meta::has_template_arguments(type) && std::meta::template_of(type) == tmpl) {
      return type;
    }
  }
  return std::meta::info{};
}

// Value of the static data member `name` of the class `type`.
template <typename T>
consteval T static_member_value(std::meta::info type, std::string_view name) {
  for (std::meta::info m : std::meta::static_data_members_of(type, std::meta::access_context::unchecked())) {
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name) {
      return std::meta::extract<T>(m);
    }
  }
  return T{};
}

consteval bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }
consteval bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
consteval bool is_digit(char c) { return c >= '0' && c <= '9'; }
consteval char to_upper(char c) { return is_lower(c) ? char(c - 'a' + 'A') : c; }
consteval char to_lower(char c) { return is_upper(c) ? char(c - 'A' + 'a') : c; }

// Split an identifier into words: at underscores, at a lowercase letter or digit
// followed by an uppercase letter (userId), and before the last capital of an
// acronym followed by a lowercase letter (HTTPServer -> HTTP, Server).
consteval std::vector<std::string> split_identifier(std::string_view id) {
  std::vector<std::string> words;
  std::string current;
  for (size_t i = 0; i < id.size(); i++) {
    char c = id[i];
    if (c == '_') {
      if (!current.empty()) { words.push_back(current); current.clear(); }
      continue;
    }
    if (is_upper(c) && !current.empty()) {
      char prev = current.back();
      bool next_is_lower = (i + 1 < id.size()) && is_lower(id[i + 1]);
      if (is_lower(prev) || is_digit(prev) || (is_upper(prev) && next_is_lower)) {
        words.push_back(current);
        current.clear();
      }
    }
    current.push_back(c);
  }
  if (!current.empty()) { words.push_back(current); }
  return words;
}

consteval std::string apply_case_style(std::string_view id, case_style style) {
  std::string result;
  if (style == case_style::lowercase || style == case_style::uppercase) {
    for (char c : id) {
      result.push_back(style == case_style::lowercase ? to_lower(c) : to_upper(c));
    }
    return result;
  }
  std::vector<std::string> words = split_identifier(id);
  for (size_t w = 0; w < words.size(); w++) {
    const std::string &word = words[w];
    if (style == case_style::pascal_case || style == case_style::camel_case) {
      for (size_t i = 0; i < word.size(); i++) {
        bool capital = (i == 0) && (style == case_style::pascal_case || w > 0);
        result.push_back(capital ? to_upper(word[i]) : to_lower(word[i]));
      }
    } else {
      bool upper = style == case_style::screaming_snake_case || style == case_style::screaming_kebab_case;
      bool kebab = style == case_style::kebab_case || style == case_style::screaming_kebab_case;
      if (w > 0) { result.push_back(kebab ? '-' : '_'); }
      for (char c : word) { result.push_back(upper ? to_upper(c) : to_lower(c)); }
    }
  }
  return result;
}

// The JSON key for a data member or an enumerator: an explicit rename wins,
// then the rename_all of the enclosing structure or enumeration, then the C++
// identifier.
consteval std::string_view json_key_name(std::meta::info entity) {
  std::meta::info rename_type = annotation_of_template(entity, ^^rename_t);
  if (rename_type != std::meta::info{}) {
    return std::define_static_string(std::string_view{
        static_member_value<const char *>(rename_type, "key_data"),
        static_member_value<size_t>(rename_type, "key_size")});
  }
  std::meta::info rename_all_type = annotation_of_template(std::meta::parent_of(entity), ^^rename_all_t);
  if (rename_all_type != std::meta::info{}) {
    case_style style = static_member_value<case_style>(rename_all_type, "style");
    return std::define_static_string(apply_case_style(std::meta::identifier_of(entity), style));
  }
  return std::define_static_string(std::meta::identifier_of(entity));
}

// The keys accepted when deserializing a data member or an enumerator: the JSON
// key first, followed by the aliases (if any), in declaration order.
consteval std::vector<std::string_view> json_key_names(std::meta::info entity) {
  std::vector<std::string_view> names{json_key_name(entity)};
  for (std::meta::info ann : std::meta::annotations_of(entity)) {
    std::meta::info type = std::meta::type_of(ann);
    if (std::meta::has_template_arguments(type) && std::meta::template_of(type) == ^^alias_t) {
      const std::string_view *keys = static_member_value<const std::string_view *>(type, "keys_data");
      size_t count = static_member_value<size_t>(type, "keys_count");
      for (size_t i = 0; i < count; i++) { names.push_back(keys[i]); }
    }
  }
  return names;
}

// The structure type of a member annotated with flatten.
consteval std::meta::info flattened_type(std::meta::info mem) {
  if (std::meta::is_reference_type(std::meta::type_of(mem))) {
    // A reference member could refer back to the enclosing structure: the
    // flattening would never terminate.
    throw std::meta::exception(u8"simdjson::flatten requires a member that is not a reference", mem);
  }
  std::meta::info type = std::meta::remove_cvref(std::meta::type_of(mem));
  if (!std::meta::is_class_type(type)) {
    throw std::meta::exception(u8"simdjson::flatten requires a member of class type", mem);
  }
  return type;
}

// The single data member of a structure annotated with transparent: the only
// member that is not annotated with skip.
consteval std::meta::info transparent_member(std::meta::info type) {
  std::vector<std::meta::info> members;
  for (std::meta::info mem : std::meta::nonstatic_data_members_of(type, std::meta::access_context::unchecked())) {
    if (!has_annotation(mem, ^^skip_tag)) { members.push_back(mem); }
  }
  if (members.size() != 1) {
    throw std::meta::exception(u8"simdjson::transparent requires exactly one data member (not counting skipped members)", type);
  }
  return members[0];
}

} // namespace detail

// Returns the JSON key for a reflected data member (or enumerator).
template <auto dm>
consteval const char* get_json_key_name() {
  return detail::json_key_name(dm).data();
}

} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_ANNOTATIONS_H
