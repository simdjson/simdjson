#include <string>
#include <type_traits>
#include <utility>
#if defined(__has_include)
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif
#endif

#include "simdjson.h"

#if SIMDJSON_CPLUSPLUS17

struct dom_stream_default_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::dom::parser &>().parse_many(
      std::forward<Input>(input)));
};

struct dom_stream_format_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::dom::parser &>().parse_many(
      std::forward<Input>(input), size_t{32},
      simdjson::stream_format::whitespace_delimited));
};

struct dom_stream_receiver_call {
  template<typename Parser>
  auto operator()(Parser &&parser) const -> decltype(
    std::forward<Parser>(parser).parse_many(
      std::declval<const simdjson::padded_string &>()));
};

struct dom_stream_format_receiver_call {
  template<typename Parser>
  auto operator()(Parser &&parser) const -> decltype(
    std::forward<Parser>(parser).parse_many(
      std::declval<const simdjson::padded_string &>(), size_t{32},
      simdjson::stream_format::whitespace_delimited));
};

struct dom_load_stream_receiver_call {
  template<typename Parser>
  auto operator()(Parser &&parser) const -> decltype(
    std::forward<Parser>(parser).load_many(std::string_view{}));
};

struct ondemand_document_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate(
      std::forward<Input>(input)));
};

struct ondemand_document_capacity_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate(
      std::forward<Input>(input), size_t{128}));
};

struct ondemand_raw_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate_raw(
      std::forward<Input>(input)));
};

struct ondemand_stream_default_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate_many(
      std::forward<Input>(input)));
};

struct ondemand_stream_format_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate_many(
      std::forward<Input>(input), size_t{32},
      simdjson::stream_format::whitespace_delimited));
};

struct ondemand_stream_receiver_call {
  template<typename Parser>
  auto operator()(Parser &&parser) const -> decltype(
    std::forward<Parser>(parser).iterate_many(
      std::declval<const simdjson::padded_string &>()));
};

struct ondemand_stream_format_receiver_call {
  template<typename Parser>
  auto operator()(Parser &&parser) const -> decltype(
    std::forward<Parser>(parser).iterate_many(
      std::declval<const simdjson::padded_string &>(), size_t{32},
      simdjson::stream_format::whitespace_delimited));
};

struct stream_begin_call {
  template<typename Stream>
  auto operator()(Stream &&stream) const -> decltype(
    std::forward<Stream>(stream).begin());
};

struct stream_end_call {
  template<typename Stream>
  auto operator()(Stream &&stream) const -> decltype(
    std::forward<Stream>(stream).end());
};

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
struct ondemand_stream_deprecated_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::declval<simdjson::ondemand::parser &>().iterate_many(
      std::forward<Input>(input), size_t{32}, false));
};
#endif

struct padded_input_view_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    static_cast<simdjson::padded_string_view>(
      std::forward<Input>(input)));
};

#if SIMDJSON_HAS_PADDED_MEMORY_MAP
struct memory_map_view_call {
  template<typename Input>
  auto operator()(Input &&input) const -> decltype(
    std::forward<Input>(input).view());
};
#endif

using padded_result = simdjson::simdjson_result<simdjson::padded_string>;

#define SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(call_type)                    \
  static_assert(!std::is_invocable<call_type, std::string>::value,           \
                "temporary std::string must be rejected");                  \
  static_assert(!std::is_invocable<call_type, const std::string>::value,     \
                "const temporary std::string must be rejected");            \
  static_assert(!std::is_invocable<call_type, simdjson::padded_string>::value, \
                "temporary padded_string must be rejected");                \
  static_assert(!std::is_invocable<call_type, const simdjson::padded_string>::value, \
                "const temporary padded_string must be rejected");          \
  static_assert(!std::is_invocable<call_type, padded_result>::value,         \
                "temporary padded_string result must be rejected");         \
  static_assert(!std::is_invocable<call_type, simdjson::padded_input>::value, \
                "temporary padded_input must be rejected")

SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(dom_stream_default_call);
SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(dom_stream_format_call);
SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(ondemand_stream_default_call);
SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(ondemand_stream_format_call);
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES(ondemand_stream_deprecated_call);
#endif

#define SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(call_type, parser_type)      \
  static_assert(std::is_invocable<call_type, parser_type &>::value,          \
                "lvalue parser must remain supported");                     \
  static_assert(!std::is_invocable<call_type, parser_type>::value,           \
                "temporary parser must be rejected")

SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(dom_stream_receiver_call,
                                         simdjson::dom::parser);
SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(dom_stream_format_receiver_call,
                                         simdjson::dom::parser);
SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(dom_load_stream_receiver_call,
                                         simdjson::dom::parser);
SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(ondemand_stream_receiver_call,
                                         simdjson::ondemand::parser);
SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER(ondemand_stream_format_receiver_call,
                                         simdjson::ondemand::parser);

#define SIMDJSON_ASSERT_REJECTS_TEMPORARY_STREAM(stream_type)                \
  static_assert(std::is_invocable<stream_begin_call, stream_type &>::value,  \
                "lvalue stream begin must remain supported");               \
  static_assert(!std::is_invocable<stream_begin_call, stream_type>::value,   \
                "temporary stream begin must be rejected");                 \
  static_assert(std::is_invocable<stream_end_call, stream_type &>::value,    \
                "lvalue stream end must remain supported");                 \
  static_assert(!std::is_invocable<stream_end_call, stream_type>::value,     \
                "temporary stream end must be rejected")

SIMDJSON_ASSERT_REJECTS_TEMPORARY_STREAM(simdjson::dom::document_stream);
SIMDJSON_ASSERT_REJECTS_TEMPORARY_STREAM(simdjson::ondemand::document_stream);
#if SIMDJSON_EXCEPTIONS || !defined(SIMDJSON_DISABLE_DEPRECATED_API)
SIMDJSON_ASSERT_REJECTS_TEMPORARY_STREAM(
  simdjson::simdjson_result<simdjson::dom::document_stream>);
#endif

static_assert(!std::is_invocable<ondemand_document_call, std::string>::value,
              "temporary std::string must be rejected");
static_assert(!std::is_invocable<ondemand_document_call, const std::string>::value,
              "const temporary std::string must be rejected");
static_assert(!std::is_invocable<ondemand_document_call, simdjson::padded_string>::value,
              "temporary padded_string must be rejected");
static_assert(!std::is_invocable<ondemand_document_call, padded_result>::value,
              "temporary padded_string result must be rejected");
static_assert(!std::is_invocable<ondemand_document_call, simdjson::padded_input>::value,
              "temporary padded_input must be rejected");
static_assert(!std::is_invocable<ondemand_document_capacity_call, std::string>::value,
              "temporary std::string with explicit capacity must be rejected");
static_assert(!std::is_invocable<ondemand_document_capacity_call,
                                 simdjson::padded_string>::value,
              "temporary padded_string with explicit capacity must be rejected");
static_assert(!std::is_invocable<ondemand_document_capacity_call,
                                 const simdjson::padded_string>::value,
              "const temporary padded_string with explicit capacity must be rejected");
static_assert(!std::is_invocable<ondemand_raw_call, simdjson::padded_string>::value,
              "temporary padded_string must be rejected by iterate_raw");
static_assert(!std::is_invocable<ondemand_raw_call, simdjson::padded_input>::value,
              "temporary padded_input must be rejected by iterate_raw");

static_assert(std::is_invocable<dom_stream_default_call, const std::string &>::value,
              "lvalue DOM stream input must remain supported");
static_assert(std::is_invocable<ondemand_document_call, const std::string &>::value,
              "lvalue On-Demand input must remain supported");
static_assert(std::is_invocable<ondemand_stream_default_call, const simdjson::padded_string &>::value,
              "lvalue On-Demand stream input must remain supported");
static_assert(std::is_invocable<padded_input_view_call, const simdjson::padded_input &>::value,
              "lvalue padded_input conversion must remain supported");
static_assert(!std::is_invocable<padded_input_view_call, simdjson::padded_input>::value,
              "temporary padded_input conversion must be rejected");
static_assert(std::is_constructible<simdjson::padded_string_view, const std::string &>::value,
              "lvalue string view construction must remain supported");
static_assert(!std::is_constructible<simdjson::padded_string_view, std::string>::value,
              "temporary string view construction must be rejected");
static_assert(!std::is_constructible<simdjson::padded_string_view,
                                     std::string, size_t>::value,
              "temporary string plus capacity must be rejected");
#if defined(__cpp_lib_memory_resource)
static_assert(!std::is_invocable<ondemand_document_capacity_call,
                                 std::pmr::string>::value,
              "temporary allocator-specialized string must be rejected");
static_assert(!std::is_constructible<simdjson::padded_string_view,
                                     std::pmr::string, size_t>::value,
              "temporary allocator-specialized string view must be rejected");
#endif

#if SIMDJSON_HAS_PADDED_MEMORY_MAP
static_assert(std::is_invocable<memory_map_view_call,
                                const simdjson::padded_memory_map &>::value,
              "lvalue memory-map view must remain supported");
static_assert(!std::is_invocable<memory_map_view_call,
                                 simdjson::padded_memory_map>::value,
              "temporary memory-map view must be rejected");
#endif

#undef SIMDJSON_ASSERT_REJECTS_STREAM_RVALUES
#undef SIMDJSON_ASSERT_REJECTS_TEMPORARY_PARSER
#undef SIMDJSON_ASSERT_REJECTS_TEMPORARY_STREAM

#endif // SIMDJSON_CPLUSPLUS17

int main() { return 0; }
