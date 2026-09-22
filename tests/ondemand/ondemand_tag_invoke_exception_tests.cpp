#include "simdjson.h"
#include "test_ondemand.h"
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace simdjson;

#if !SIMDJSON_SUPPORTS_CONCEPTS

int main(void) {
  printf("This test is only relevant when SIMDJSON_SUPPORTS_CONCEPTS is true (C++20)\n");
  return EXIT_SUCCESS;
}
#else

struct throwing_type {
  int64_t x{};
};

struct nothrow_type {
  int64_t x{};
};

struct document_only_type {
  int64_t x{};
};

namespace simdjson {
template <typename simdjson_value>
error_code tag_invoke(deserialize_tag, simdjson_value &val, throwing_type &out) {
  int64_t x;
  if (auto error = val.get_int64().get(x)) {
    return error;
  }
#if SIMDJSON_EXCEPTIONS
  if (x < 0) {
    throw std::runtime_error("negative");
  }
#endif
  out.x = x;
  return SUCCESS;
}

template <typename simdjson_value>
error_code tag_invoke(deserialize_tag, simdjson_value &val, nothrow_type &out) noexcept {
  return val.get_int64().get(out.x);
}

// A throwing customization defined for document only.
error_code tag_invoke(deserialize_tag, ondemand::document &val, document_only_type &out) {
  return val.get_int64().get(out.x);
}
} // namespace simdjson

namespace tag_invoke_exception_tests {

// ---- compile-time checks ---------------------------------------------------

template <typename ValT, typename T>
constexpr bool get_is_noexcept = noexcept(std::declval<ValT &>().template get<T>());
template <typename ValT, typename T>
constexpr bool get_ref_is_noexcept = noexcept(std::declval<ValT &>().template get<T>(std::declval<T &>()));

// Built-in types never throw.
static_assert(get_is_noexcept<ondemand::value, int64_t>);
static_assert(get_is_noexcept<simdjson_result<ondemand::value>, int64_t>);
static_assert(get_is_noexcept<ondemand::document, int64_t>);
static_assert(get_is_noexcept<simdjson_result<ondemand::document>, int64_t>);

// A noexcept tag_invoke keeps the whole chain noexcept.
static_assert(nothrow_gettable<nothrow_type, ondemand::value>);
static_assert(get_is_noexcept<ondemand::value, nothrow_type>);
static_assert(get_ref_is_noexcept<ondemand::value, nothrow_type>);
static_assert(get_is_noexcept<simdjson_result<ondemand::value>, nothrow_type>);
static_assert(get_ref_is_noexcept<simdjson_result<ondemand::value>, nothrow_type>);
static_assert(get_is_noexcept<ondemand::document, nothrow_type>);
static_assert(get_is_noexcept<simdjson_result<ondemand::document>, nothrow_type>);
static_assert(get_is_noexcept<ondemand::document_reference, nothrow_type>);
static_assert(get_is_noexcept<simdjson_result<ondemand::document_reference>, nothrow_type>);
static_assert(get_is_noexcept<ondemand::value, std::optional<nothrow_type>>);
static_assert(get_is_noexcept<simdjson_result<ondemand::value>, std::optional<nothrow_type>>);
// unique_ptr / vector / map allocate, so their tag_invoke is noexcept(false)
// even when the nested type cannot throw (bad_alloc must be able to propagate).
static_assert(!get_is_noexcept<ondemand::value, std::unique_ptr<nothrow_type>>);

// A potentially-throwing tag_invoke makes every layer noexcept(false).
static_assert(!nothrow_gettable<throwing_type, ondemand::value>);
static_assert(!get_is_noexcept<ondemand::value, throwing_type>);
static_assert(!get_ref_is_noexcept<ondemand::value, throwing_type>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::value>, throwing_type>);
static_assert(!get_ref_is_noexcept<simdjson_result<ondemand::value>, throwing_type>);
static_assert(!get_is_noexcept<ondemand::document, throwing_type>);
static_assert(!get_ref_is_noexcept<ondemand::document, throwing_type>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::document>, throwing_type>);
static_assert(!get_ref_is_noexcept<simdjson_result<ondemand::document>, throwing_type>);
static_assert(!get_is_noexcept<ondemand::document_reference, throwing_type>);
static_assert(!get_ref_is_noexcept<ondemand::document_reference, throwing_type>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::document_reference>, throwing_type>);
static_assert(!get_ref_is_noexcept<simdjson_result<ondemand::document_reference>, throwing_type>);
static_assert(!get_is_noexcept<ondemand::value, std::optional<throwing_type>>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::value>, std::optional<throwing_type>>);
static_assert(!get_is_noexcept<ondemand::value, std::unique_ptr<throwing_type>>);
// document_reference::get() && forwards to document::get(), so it follows the
// document customization even when there is none for document_reference.
static_assert(!noexcept(std::declval<ondemand::document_reference &&>().template get<document_only_type>()));
static_assert(!noexcept(std::declval<ondemand::document &>().template get<document_only_type>()));
// Containers of a throwing element type.
static_assert(!get_is_noexcept<ondemand::value, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::value>, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<ondemand::array, std::vector<throwing_type>>);
static_assert(!get_ref_is_noexcept<ondemand::array, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::array>, std::vector<throwing_type>>);
static_assert(!get_ref_is_noexcept<simdjson_result<ondemand::array>, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<ondemand::document, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::document>, std::vector<throwing_type>>);
static_assert(!get_is_noexcept<ondemand::value, std::vector<nothrow_type>>);
static_assert(!get_is_noexcept<ondemand::value, std::map<std::string, throwing_type>>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::value>, std::map<std::string, throwing_type>>);
static_assert(!get_is_noexcept<ondemand::document, std::map<std::string, throwing_type>>);
static_assert(!get_is_noexcept<ondemand::value, std::map<std::string, nothrow_type>>);

#if SIMDJSON_STATIC_REFLECTION
// A reflected struct is deserialized by a noexcept(false) customization (one of
// its members may have a throwing tag_invoke), so get<T>() is noexcept(false)
// for every reflected struct T, and an exception thrown while deserializing a
// member propagates out of get<T>().
struct Holder {
  int64_t id{};
  throwing_type t{};
};
struct Outer {
  int64_t id{};
  Holder inner{};
};
static_assert(!get_is_noexcept<ondemand::value, Holder>);
static_assert(!get_ref_is_noexcept<ondemand::value, Holder>);
static_assert(!get_is_noexcept<simdjson_result<ondemand::value>, Holder>);
static_assert(!get_is_noexcept<ondemand::document, Holder>);
static_assert(!get_is_noexcept<ondemand::value, Outer>);
static_assert(!get_is_noexcept<ondemand::value, std::vector<Holder>>);
// Self-referential types must still compile: allocating customizations
// (containers, smart pointers, reflected structs) are unconditionally
// noexcept(false), so computing the exception specification of get<T>() never
// recurses into T's members.
struct Node {
  int64_t v{};
  std::unique_ptr<Node> next{};
};
struct Tree {
  int64_t v{};
  std::vector<Tree> children{};
};
static_assert(!get_is_noexcept<ondemand::value, Node>);
static_assert(!get_is_noexcept<ondemand::value, Tree>);
static_assert(!get_is_noexcept<ondemand::document, Node>);
static_assert(!get_is_noexcept<ondemand::document, Tree>);
// simdjson's own types never go through the reflected-struct customization.
static_assert(!custom_deserializable<ondemand::array, ondemand::value>);
static_assert(!custom_deserializable<ondemand::object, ondemand::value>);
static_assert(!custom_deserializable<ondemand::number, ondemand::value>);
static_assert(!custom_deserializable<ondemand::document, ondemand::document>);
#endif

// ---- runtime checks --------------------------------------------------------

#if SIMDJSON_EXCEPTIONS

// Runs f() and returns true if it threw the runtime_error from our tag_invoke.
template <typename F>
bool throws_negative(F &&f) {
  try {
    f();
  } catch (std::runtime_error &e) {
    return std::string_view(e.what()) == "negative";
  }
  return false;
}

bool throw_through_value() {
  TEST_START();
  auto json = R"({"a": -1})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::value v = doc["a"];
  ASSERT_TRUE(throws_negative([&] { (void)v.get<throwing_type>(); }));
  TEST_SUCCEED();
}

bool throw_through_result_value() {
  TEST_START();
  auto json = R"({"a": -1})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<throwing_type>(); }));
  doc.rewind();
  throwing_type t;
  ASSERT_TRUE(throws_negative([&] { auto e = doc["a"].get<throwing_type>(t); (void)e; }));
  doc.rewind();
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<std::optional<throwing_type>>(); }));
  TEST_SUCCEED();
}

bool throw_through_document() {
  TEST_START();
  auto json = R"(-1)"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc.get<throwing_type>(); }));
  doc.rewind();
  throwing_type t;
  ASSERT_TRUE(throws_negative([&] { auto e = doc.get<throwing_type>(t); (void)e; }));
  // Through the simdjson_result<document> wrapper.
  simdjson_result<ondemand::document> result = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)result.get<throwing_type>(); }));
  result = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { auto e = result.get<throwing_type>(t); (void)e; }));
  TEST_SUCCEED();
}

bool throw_through_document_reference() {
  TEST_START();
  auto json = R"(-1)"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::document_reference ref(doc);
  ASSERT_TRUE(throws_negative([&] { (void)ref.get<throwing_type>(); }));
  doc.rewind();
  throwing_type t;
  ASSERT_TRUE(throws_negative([&] { auto e = ref.get<throwing_type>(t); (void)e; }));
  // Through the simdjson_result<document_reference> wrapper, as produced by
  // iterate_many.
  auto stream_json = R"(-1 -1)"_padded;
  ondemand::document_stream stream = parser.iterate_many(stream_json);
  size_t count = 0;
  for (auto result : stream) {
    if (count == 0) {
      ASSERT_TRUE(throws_negative([&] { (void)result.get<throwing_type>(); }));
    } else {
      ASSERT_TRUE(throws_negative([&] { auto e = result.get<throwing_type>(t); (void)e; }));
    }
    count++;
  }
  ASSERT_EQUAL(count, 2);
  TEST_SUCCEED();
}

bool throw_through_container() {
  TEST_START();
  auto json = R"({"a": [1, 2, -1, 4]})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<std::vector<throwing_type>>(); }));
  doc.rewind();
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get_array().get<std::vector<throwing_type>>(); }));
  doc.rewind();
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<std::list<throwing_type>>(); }));
  doc.rewind();
  // Elements already stored are kept; the throwing element is not inserted.
  std::vector<throwing_type> out;
  ASSERT_TRUE(throws_negative([&] { auto e = doc["a"].get<std::vector<throwing_type>>(out); (void)e; }));
  ASSERT_EQUAL(out.size(), 2);
  ASSERT_EQUAL(out[0].x, 1);
  ASSERT_EQUAL(out[1].x, 2);
  TEST_SUCCEED();
}

bool throw_through_map() {
  TEST_START();
  auto json = R"({"a": {"x": 1, "y": -1, "z": 3}})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<std::map<std::string, throwing_type>>(); }));
  doc.rewind();
  std::map<std::string, throwing_type> out;
  ASSERT_TRUE(throws_negative([&] { auto e = doc["a"].get<std::map<std::string, throwing_type>>(out); (void)e; }));
  ASSERT_EQUAL(out.size(), 1);
  ASSERT_EQUAL(out["x"].x, 1);
  TEST_SUCCEED();
}

bool throw_through_unique_ptr() {
  TEST_START();
  auto json = R"({"a": -1})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc["a"].get<std::unique_ptr<throwing_type>>(); }));
  doc.rewind();
  std::unique_ptr<throwing_type> p;
  ASSERT_TRUE(throws_negative([&] { auto e = doc["a"].get<std::unique_ptr<throwing_type>>(p); (void)e; }));
  ASSERT_TRUE(p == nullptr);
  TEST_SUCCEED();
}

#if SIMDJSON_STATIC_REFLECTION
bool throw_through_reflected_struct() {
  TEST_START();
  auto json = R"({"id": 7, "t": -1})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ASSERT_TRUE(throws_negative([&] { (void)doc.get<Holder>(); }));
  doc.rewind();
  Holder h;
  ASSERT_TRUE(throws_negative([&] { auto e = doc.get<Holder>(h); (void)e; }));
  // Nested one level down, and through the simdjson_result<value> wrapper.
  auto nested = R"({"id": 1, "inner": {"id": 7, "t": -1}})"_padded;
  doc = parser.iterate(nested);
  ASSERT_TRUE(throws_negative([&] { (void)doc.get<Outer>(); }));
  doc.rewind();
  ASSERT_TRUE(throws_negative([&] { (void)doc["inner"].get<Holder>(); }));
  // Inside a container of reflected structs: earlier elements are kept.
  auto arr = R"({"a": [{"id": 1, "t": 1}, {"id": 2, "t": -1}, {"id": 3, "t": 3}]})"_padded;
  doc = parser.iterate(arr);
  std::vector<Holder> out;
  ASSERT_TRUE(throws_negative([&] { auto e = doc["a"].get<std::vector<Holder>>(out); (void)e; }));
  ASSERT_EQUAL(out.size(), 1);
  ASSERT_EQUAL(out[0].id, 1);
  // And valid input still deserializes.
  auto ok = R"({"id": 1, "inner": {"id": 7, "t": 5}})"_padded;
  doc = parser.iterate(ok);
  Outer o = doc.get<Outer>();
  ASSERT_EQUAL(o.id, 1);
  ASSERT_EQUAL(o.inner.id, 7);
  ASSERT_EQUAL(o.inner.t.x, 5);
  TEST_SUCCEED();
}

bool recursive_tree() {
  TEST_START();
  auto json = R"({"v": 1, "children": [{"v": 2, "children": []}]})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  Tree t = doc.get<Tree>();
  ASSERT_EQUAL(t.v, 1);
  ASSERT_EQUAL(t.children.size(), 1);
  ASSERT_EQUAL(t.children[0].v, 2);
  ASSERT_EQUAL(t.children[0].children.size(), 0);
  TEST_SUCCEED();
}
#endif

// On an error code, the container keeps exactly the elements that were
// deserialized successfully, whether or not the element type can throw.
bool error_code_through_container() {
  TEST_START();
  auto json = R"({"a": [1, 2, "x", 4]})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::vector<nothrow_type> out;
  ASSERT_ERROR(doc["a"].get<std::vector<nothrow_type>>(out), INCORRECT_TYPE);
  ASSERT_EQUAL(out.size(), 2);
  ASSERT_EQUAL(out[0].x, 1);
  ASSERT_EQUAL(out[1].x, 2);
  doc.rewind();
  std::vector<throwing_type> out2;
  ASSERT_ERROR(doc["a"].get<std::vector<throwing_type>>(out2), INCORRECT_TYPE);
  ASSERT_EQUAL(out2.size(), 2);
  TEST_SUCCEED();
}

bool no_throw_on_valid_input() {
  TEST_START();
  auto json = R"({"a": [1, 2, 3], "b": 4})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::vector<throwing_type> a = doc["a"].get<std::vector<throwing_type>>();
  ASSERT_EQUAL(a.size(), 3);
  ASSERT_EQUAL(a[2].x, 3);
  throwing_type b = doc["b"].get<throwing_type>();
  ASSERT_EQUAL(b.x, 4);
  TEST_SUCCEED();
}

bool run() {
  return throw_through_value() && throw_through_result_value() &&
         throw_through_document() && throw_through_document_reference() &&
         throw_through_container() && throw_through_map() &&
         throw_through_unique_ptr() &&
#if SIMDJSON_STATIC_REFLECTION
         throw_through_reflected_struct() && recursive_tree() &&
#endif
         error_code_through_container() &&
         no_throw_on_valid_input();
}
#else
bool run() { return true; }
#endif // SIMDJSON_EXCEPTIONS

} // namespace tag_invoke_exception_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, tag_invoke_exception_tests::run);
}
#endif // SIMDJSON_SUPPORTS_CONCEPTS
