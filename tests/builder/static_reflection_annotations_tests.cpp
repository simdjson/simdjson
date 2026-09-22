#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <optional>
#include <vector>

using namespace simdjson;

#if SIMDJSON_STATIC_REFLECTION

struct RenamedFields {
  [[= simdjson::rename<"first_name">]] std::string firstName = "";
  [[= simdjson::rename<"last_name">]]  std::string lastName = "";
  int age = 0;
};

struct SkippedField {
  std::string name = "";
  [[= simdjson::skip]] int internalCache = 0;
};

struct MixedAnnotations {
  [[= simdjson::rename<"user_name">]] std::string userName = "";
  [[= simdjson::skip]] int sessionToken = 0;
  int age = 0;
};

struct AliasedFields {
  [[= simdjson::alias<"userName", "login">]] std::string user_name = "";
  int age = 0;
};

struct OneWaySkips {
  std::string name = "";
  [[= simdjson::skip_serializing]] std::string password = "";
  [[= simdjson::skip_deserializing]] int computed = 7;
};

inline constexpr auto is_zero = [](int v) { return v == 0; };

struct ConditionalSkips {
  [[= simdjson::skip_serializing_if<simdjson::is_none>]] std::optional<int> maybe{};
  [[= simdjson::skip_serializing_if<simdjson::is_empty>]] std::vector<int> values{};
  [[= simdjson::skip_serializing_if<simdjson::is_empty>]] std::string note = "";
  [[= simdjson::skip_serializing_if<is_zero>]] int count = 0;
  int always = 0;
};

struct Settings {
  std::string host = "";
  [[= simdjson::default_value]] int port = 8080;
  [[= simdjson::default_value]] std::vector<std::string> tags{"default"};
};

struct [[= simdjson::default_value]] AllDefaults {
  std::string host = "localhost";
  int port = 80;
  bool secure = false;
};

int make_timeout() { return 30; }

struct DefaultFrom {
  std::string name = "";
  [[= simdjson::default_from<make_timeout>]] int timeout = 0;
  [[= simdjson::default_from<[] { return std::string("guest"); }>]] std::string role{};
};

// Serializes a bool as "yes"/"no".
struct yes_no {
  static void serialize(simdjson::builder::string_builder &b, const bool &v) {
    b.append_raw(v ? "\"yes\"" : "\"no\"");
  }
  static simdjson::error_code deserialize(simdjson::ondemand::value &v, bool &out) {
    std::string_view s;
    SIMDJSON_TRY(v.get_string().get(s));
    if (s == "yes") { out = true; return simdjson::SUCCESS; }
    if (s == "no") { out = false; return simdjson::SUCCESS; }
    return simdjson::INCORRECT_TYPE;
  }
};

// Only customizes serialization: an int written as a string.
struct int_as_string {
  static void serialize(auto &b, const int &v) {
    std::string s = std::to_string(v);
    b.escape_and_append_with_quotes(s);
  }
};

struct WithAdapters {
  [[= simdjson::with<yes_no>]] bool enabled = false;
  [[= simdjson::with<int_as_string>]] int id = 0;
};

struct [[= simdjson::rename_all<simdjson::case_style::camel_case>]] CamelCase {
  std::string first_name = "";
  int user_id = 0;
  [[= simdjson::rename<"KEEP">]] int explicit_name = 0;
};

struct [[= simdjson::rename_all<simdjson::case_style::snake_case>]] SnakeCase {
  std::string firstName = "";
  int HTTPStatus = 0;
};

struct [[= simdjson::rename_all<simdjson::case_style::screaming_kebab_case>]] ScreamingKebab {
  int max_retry_count = 0;
};

struct [[= simdjson::rename_all<simdjson::case_style::pascal_case>]] PascalCase {
  int user_id = 0;
};

struct [[= simdjson::deny_unknown_fields]] Strict {
  std::string name = "";
  [[= simdjson::alias<"years">]] int age = 0;
  [[= simdjson::default_value]] int level = 1;
  [[= simdjson::skip]] int secret = 0;
};

struct [[= simdjson::transparent]] UserId {
  int64_t value = 0;
};

struct [[= simdjson::transparent]] Tags {
  std::vector<std::string> items{};
};

struct Account {
  UserId id{};
  Tags tags{};
  std::vector<UserId> friends{};
};

enum class [[= simdjson::rename_all<simdjson::case_style::screaming_snake_case>]] Status {
  not_started,
  inProgress,
  done [[= simdjson::rename<"finished">, = simdjson::alias<"complete", "DONE">]]
};

struct Task {
  Status status = Status::not_started;
};

struct Pagination {
  int limit = 0;
  int offset = 0;
};

struct [[= simdjson::rename_all<simdjson::case_style::camel_case>]] RequestMeta {
  std::string request_id = "";
  [[= simdjson::default_value]] int retry_count = 0;
};

struct Trace {
  [[= simdjson::flatten]] RequestMeta meta{};
  [[= simdjson::alias<"span">]] std::string span_id = "";
};

struct UserPage {
  int64_t id = 0;
  [[= simdjson::flatten]] Pagination page{};
  [[= simdjson::flatten]] Trace trace{};
};

struct [[= simdjson::deny_unknown_fields]] StrictPage {
  int id = 0;
  [[= simdjson::flatten]] Pagination page{};
};

// A key longer than 63 characters does not fit a key_selector, which forces the
// ordered per-member fallback even in the default build.
struct LongKeys {
  [[= simdjson::rename<"a_very_long_key_name_that_exceeds_the_key_selector_limit_of_63_chars">]] int value = 0;
  [[= simdjson::alias<"alt">]] int other = 0;
  [[= simdjson::default_from<make_timeout>]] int timeout = 0;
};

#endif // SIMDJSON_STATIC_REFLECTION

#if SIMDJSON_STATIC_REFLECTION
template <typename T>
simdjson::error_code parse_as(const std::string &json, T &out) {
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  simdjson::padded_string padded(json);
  SIMDJSON_TRY(parser.iterate(padded).get(doc));
  return doc.get<T>().get(out);
}
#endif // SIMDJSON_STATIC_REFLECTION

namespace annotation_tests {

bool rename_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  RenamedFields r{"Alice", "Smith", 30};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(r).get(out));
  ASSERT_EQUAL(out, "{\"first_name\":\"Alice\",\"last_name\":\"Smith\",\"age\":30}");
#endif
  TEST_SUCCEED();
}

bool rename_deserialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string json = R"({"first_name":"Bob","last_name":"Jones","age":25})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  RenamedFields r;
  ASSERT_SUCCESS(doc.get<RenamedFields>().get(r));
  ASSERT_EQUAL(r.firstName, "Bob");
  ASSERT_EQUAL(r.lastName, "Jones");
  ASSERT_EQUAL(r.age, 25);
#endif
  TEST_SUCCEED();
}

bool rename_roundtrip_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  RenamedFields original{"Carol", "White", 40};
  std::string json;
  ASSERT_SUCCESS(simdjson::to_json(original).get(json));
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  RenamedFields result;
  ASSERT_SUCCESS(doc.get<RenamedFields>().get(result));
  ASSERT_EQUAL(result.firstName, original.firstName);
  ASSERT_EQUAL(result.lastName, original.lastName);
  ASSERT_EQUAL(result.age, original.age);
#endif
  TEST_SUCCEED();
}

bool skip_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SkippedField s{"Alice", 999};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(s).get(out));
  ASSERT_EQUAL(out, "{\"name\":\"Alice\"}");
#endif
  TEST_SUCCEED();
}

bool skip_deserialize_ignores_field_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  // The key "internalCache" is present in JSON but the field is annotated skip -
  // it should be ignored and the field should keep its default value.
  std::string json = R"({"name":"Carol","internalCache":999})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  SkippedField s;
  ASSERT_SUCCESS(doc.get<SkippedField>().get(s));
  ASSERT_EQUAL(s.name, "Carol");
  ASSERT_EQUAL(s.internalCache, 0);
#endif
  TEST_SUCCEED();
}

bool mixed_annotations_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  MixedAnnotations m{"dave", 12345, 28};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(m).get(out));
  // sessionToken must not appear; userName must appear as "user_name"
  ASSERT_EQUAL(out, "{\"user_name\":\"dave\",\"age\":28}");
#endif
  TEST_SUCCEED();
}

bool mixed_annotations_deserialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string json = R"({"user_name":"eve","age":35})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  MixedAnnotations m;
  ASSERT_SUCCESS(doc.get<MixedAnnotations>().get(m));
  ASSERT_EQUAL(m.userName, "eve");
  ASSERT_EQUAL(m.age, 35);
  ASSERT_EQUAL(m.sessionToken, 0);
#endif
  TEST_SUCCEED();
}

bool alias_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  AliasedFields a;
  ASSERT_SUCCESS(parse_as(R"({"user_name":"ann","age":1})", a));
  ASSERT_EQUAL(a.user_name, "ann");
  ASSERT_SUCCESS(parse_as(R"({"userName":"bob","age":2})", a));
  ASSERT_EQUAL(a.user_name, "bob");
  ASSERT_SUCCESS(parse_as(R"({"age":3,"login":"cat"})", a));
  ASSERT_EQUAL(a.user_name, "cat");
  ASSERT_EQUAL(a.age, 3);
  // When several names of a member are present, one of them is used.
  ASSERT_SUCCESS(parse_as(R"({"login":"first","user_name":"second","age":4})", a));
  ASSERT_TRUE(a.user_name == "first" || a.user_name == "second");
  // Missing (under every name) is still an error.
  ASSERT_ERROR(parse_as(R"({"age":5})", a), simdjson::NO_SUCH_FIELD);
  // Serialization uses the regular key.
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(AliasedFields{"dan", 6}).get(out));
  ASSERT_EQUAL(out, R"({"user_name":"dan","age":6})");
#endif
  TEST_SUCCEED();
}

bool one_way_skip_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  OneWaySkips s{"eve", "hunter2", 42};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(s).get(out));
  ASSERT_EQUAL(out, R"({"name":"eve","computed":42})");
  OneWaySkips r;
  ASSERT_SUCCESS(parse_as(R"({"name":"fay","password":"pw","computed":99})", r));
  ASSERT_EQUAL(r.name, "fay");
  ASSERT_EQUAL(r.password, "pw");
  ASSERT_EQUAL(r.computed, 7); // not deserialized
  // A skip_deserializing member is not required.
  ASSERT_SUCCESS(parse_as(R"({"name":"gus","password":"pw"})", r));
#endif
  TEST_SUCCEED();
}

bool skip_serializing_if_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string out;
  ConditionalSkips empty;
  ASSERT_SUCCESS(simdjson::to_json(empty).get(out));
  ASSERT_EQUAL(out, R"({"always":0})");
  ConditionalSkips full;
  full.maybe = 1;
  full.values = {2, 3};
  full.note = "hi";
  full.count = 4;
  full.always = 5;
  ASSERT_SUCCESS(simdjson::to_json(full).get(out));
  ASSERT_EQUAL(out, R"({"maybe":1,"values":[2,3],"note":"hi","count":4,"always":5})");
  // The first serialized key may come after a skipped one: no stray comma.
  ConditionalSkips partial;
  partial.count = 9;
  ASSERT_SUCCESS(simdjson::to_json(partial).get(out));
  ASSERT_EQUAL(out, R"({"count":9,"always":0})");
#endif
  TEST_SUCCEED();
}

bool default_value_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  Settings s;
  ASSERT_SUCCESS(parse_as(R"({"host":"example.com"})", s));
  ASSERT_EQUAL(s.host, "example.com");
  ASSERT_EQUAL(s.port, 8080);
  ASSERT_EQUAL(s.tags.size(), 1);
  ASSERT_EQUAL(s.tags[0], "default");
  ASSERT_SUCCESS(parse_as(R"({"port":1,"host":"h","tags":[]})", s));
  ASSERT_EQUAL(s.port, 1);
  ASSERT_EQUAL(s.tags.size(), 0);
  // Members without default_value remain required.
  ASSERT_ERROR(parse_as(R"({"port":1})", s), simdjson::NO_SUCH_FIELD);
  // A present but invalid value is still an error.
  ASSERT_ERROR(parse_as(R"({"host":"h","port":"x"})", s), simdjson::INCORRECT_TYPE);
#endif
  TEST_SUCCEED();
}

bool struct_default_value_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  AllDefaults d;
  ASSERT_SUCCESS(parse_as(R"({})", d));
  ASSERT_EQUAL(d.host, "localhost");
  ASSERT_EQUAL(d.port, 80);
  ASSERT_EQUAL(d.secure, false);
  ASSERT_SUCCESS(parse_as(R"({"secure":true})", d));
  ASSERT_EQUAL(d.host, "localhost");
  ASSERT_EQUAL(d.secure, true);
#endif
  TEST_SUCCEED();
}

bool default_from_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  DefaultFrom d;
  ASSERT_SUCCESS(parse_as(R"({"name":"x"})", d));
  ASSERT_EQUAL(d.timeout, 30);
  ASSERT_EQUAL(d.role, "guest");
  ASSERT_SUCCESS(parse_as(R"({"name":"x","timeout":5,"role":"admin"})", d));
  ASSERT_EQUAL(d.timeout, 5);
  ASSERT_EQUAL(d.role, "admin");
#endif
  TEST_SUCCEED();
}

bool with_adapter_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(WithAdapters{true, 12}).get(out));
  ASSERT_EQUAL(out, R"({"enabled":"yes","id":"12"})");
  WithAdapters w;
  // int_as_string has no deserialize: the default (a JSON number) applies.
  ASSERT_SUCCESS(parse_as(R"({"enabled":"no","id":3})", w));
  ASSERT_EQUAL(w.enabled, false);
  ASSERT_EQUAL(w.id, 3);
  ASSERT_SUCCESS(parse_as(R"({"enabled":"yes","id":4})", w));
  ASSERT_EQUAL(w.enabled, true);
  ASSERT_ERROR(parse_as(R"({"enabled":true,"id":4})", w), simdjson::INCORRECT_TYPE);
  ASSERT_ERROR(parse_as(R"({"enabled":"maybe","id":4})", w), simdjson::INCORRECT_TYPE);
#endif
  TEST_SUCCEED();
}

bool rename_all_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(CamelCase{"Ann", 7, 8}).get(out));
  ASSERT_EQUAL(out, R"({"firstName":"Ann","userId":7,"KEEP":8})");
  CamelCase c;
  ASSERT_SUCCESS(parse_as(out, c));
  ASSERT_EQUAL(c.first_name, "Ann");
  ASSERT_EQUAL(c.user_id, 7);
  ASSERT_EQUAL(c.explicit_name, 8);
  ASSERT_ERROR(parse_as(R"({"first_name":"Ann","userId":7,"KEEP":8})", c), simdjson::NO_SUCH_FIELD);

  ASSERT_SUCCESS(simdjson::to_json(SnakeCase{"Bo", 404}).get(out));
  ASSERT_EQUAL(out, R"({"first_name":"Bo","http_status":404})");
  SnakeCase sc;
  ASSERT_SUCCESS(parse_as(out, sc));
  ASSERT_EQUAL(sc.HTTPStatus, 404);

  ASSERT_SUCCESS(simdjson::to_json(ScreamingKebab{3}).get(out));
  ASSERT_EQUAL(out, R"({"MAX-RETRY-COUNT":3})");
  ASSERT_SUCCESS(simdjson::to_json(PascalCase{4}).get(out));
  ASSERT_EQUAL(out, R"({"UserId":4})");
#endif
  TEST_SUCCEED();
}

bool deny_unknown_fields_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  Strict s;
  ASSERT_SUCCESS(parse_as(R"({"name":"a","age":1})", s));
  ASSERT_EQUAL(s.name, "a");
  ASSERT_EQUAL(s.age, 1);
  ASSERT_EQUAL(s.level, 1);
  ASSERT_SUCCESS(parse_as(R"({"years":2,"level":3,"name":"b"})", s));
  ASSERT_EQUAL(s.age, 2);
  ASSERT_EQUAL(s.level, 3);
  ASSERT_ERROR(parse_as(R"({"name":"a","age":1,"extra":true})", s), simdjson::UNKNOWN_FIELD);
  // The key of a skipped member is unknown.
  ASSERT_ERROR(parse_as(R"({"name":"a","age":1,"secret":1})", s), simdjson::UNKNOWN_FIELD);
  ASSERT_ERROR(parse_as(R"({"name":"a"})", s), simdjson::NO_SUCH_FIELD);
  ASSERT_ERROR(parse_as(R"({"name":"a","age":"x"})", s), simdjson::INCORRECT_TYPE);
  ASSERT_ERROR(parse_as(R"([1])", s), simdjson::INCORRECT_TYPE);
  // Escaped keys are compared after unescaping.
  ASSERT_SUCCESS(parse_as(R"({"name":"c","age":4})", s));
  ASSERT_EQUAL(s.name, "c");
  std::string message = simdjson::error_message(simdjson::UNKNOWN_FIELD);
  ASSERT_TRUE(message.find("UNKNOWN_FIELD") != std::string::npos);
#endif
  TEST_SUCCEED();
}

bool transparent_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  Account a;
  a.id.value = 42;
  a.tags.items = {"x", "y"};
  a.friends = {UserId{1}, UserId{2}};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(a).get(out));
  ASSERT_EQUAL(out, R"({"id":42,"tags":["x","y"],"friends":[1,2]})");
  Account b;
  ASSERT_SUCCESS(parse_as(out, b));
  ASSERT_EQUAL(b.id.value, 42);
  ASSERT_EQUAL(b.tags.items.size(), 2);
  ASSERT_EQUAL(b.friends.size(), 2);
  ASSERT_EQUAL(b.friends[1].value, 2);
  UserId top;
  ASSERT_SUCCESS(parse_as("17", top));
  ASSERT_EQUAL(top.value, 17);
  ASSERT_SUCCESS(simdjson::to_json(top).get(out));
  ASSERT_EQUAL(out, "17");
#endif
  TEST_SUCCEED();
}

bool enum_annotations_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(Task{Status::not_started}).get(out));
  ASSERT_EQUAL(out, R"({"status":"NOT_STARTED"})");
  ASSERT_SUCCESS(simdjson::to_json(Task{Status::inProgress}).get(out));
  ASSERT_EQUAL(out, R"({"status":"IN_PROGRESS"})");
  ASSERT_SUCCESS(simdjson::to_json(Task{Status::done}).get(out));
  ASSERT_EQUAL(out, R"({"status":"finished"})");
  Task t;
  ASSERT_SUCCESS(parse_as(R"({"status":"IN_PROGRESS"})", t));
  ASSERT_TRUE(t.status == Status::inProgress);
  ASSERT_SUCCESS(parse_as(R"({"status":"finished"})", t));
  ASSERT_TRUE(t.status == Status::done);
  ASSERT_SUCCESS(parse_as(R"({"status":"complete"})", t));
  ASSERT_TRUE(t.status == Status::done);
  ASSERT_SUCCESS(parse_as(R"({"status":"DONE"})", t));
  ASSERT_TRUE(t.status == Status::done);
  ASSERT_ERROR(parse_as(R"({"status":"inProgress"})", t), simdjson::INCORRECT_TYPE);
#endif
  TEST_SUCCEED();
}

bool long_keys_fallback_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  LongKeys k;
  ASSERT_SUCCESS(parse_as(R"({"alt":2,"a_very_long_key_name_that_exceeds_the_key_selector_limit_of_63_chars":1})", k));
  ASSERT_EQUAL(k.value, 1);
  ASSERT_EQUAL(k.other, 2);
  ASSERT_EQUAL(k.timeout, 30);
  ASSERT_SUCCESS(parse_as(R"({"other":3,"a_very_long_key_name_that_exceeds_the_key_selector_limit_of_63_chars":1,"timeout":4})", k));
  ASSERT_EQUAL(k.other, 3);
  ASSERT_EQUAL(k.timeout, 4);
  ASSERT_ERROR(parse_as(R"({"other":3})", k), simdjson::NO_SUCH_FIELD);
#endif
  TEST_SUCCEED();
}

#if SIMDJSON_STATIC_REFLECTION
UserPage make_user_page() {
  UserPage p{};
  p.id = 1;
  p.page = {10, 20};
  p.trace.meta.request_id = "r1";
  p.trace.meta.retry_count = 2;
  p.trace.span_id = "s1";
  return p;
}
#endif

bool flatten_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(make_user_page()).get(out));
  ASSERT_EQUAL(out, R"({"id":1,"limit":10,"offset":20,"requestId":"r1","retryCount":2,"span_id":"s1"})");
#endif
  TEST_SUCCEED();
}

bool flatten_roundtrip_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  // The output of flatten_serialize_test.
  UserPage q{};
  ASSERT_SUCCESS(parse_as(R"({"id":1,"limit":10,"offset":20,"requestId":"r1","retryCount":2,"span_id":"s1"})", q));
  ASSERT_EQUAL(q.id, 1);
  ASSERT_EQUAL(q.page.limit, 10);
  ASSERT_EQUAL(q.page.offset, 20);
  ASSERT_EQUAL(q.trace.meta.request_id, "r1");
  ASSERT_EQUAL(q.trace.meta.retry_count, 2);
  ASSERT_EQUAL(q.trace.span_id, "s1");
#endif
  TEST_SUCCEED();
}

bool flatten_deserialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  UserPage q{};
  // Any order; the default_value of a flattened member applies; aliases apply.
  ASSERT_SUCCESS(parse_as(R"({"span":"s2","offset":5,"requestId":"r2","id":3,"limit":4})", q));
  ASSERT_EQUAL(q.id, 3);
  ASSERT_EQUAL(q.page.limit, 4);
  ASSERT_EQUAL(q.page.offset, 5);
  ASSERT_EQUAL(q.trace.meta.retry_count, 0);
  ASSERT_EQUAL(q.trace.span_id, "s2");
  // A missing required member of a flattened structure is an error.
  ASSERT_ERROR(parse_as(R"({"id":3,"offset":5,"requestId":"r","span_id":"s"})", q), simdjson::NO_SUCH_FIELD);
  // The flattened structure is not expected as a nested object.
  ASSERT_ERROR(parse_as(R"({"id":3,"page":{"limit":1,"offset":2},"requestId":"r","span_id":"s"})", q), simdjson::NO_SUCH_FIELD);
#endif
  TEST_SUCCEED();
}

bool flatten_deny_unknown_fields_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  StrictPage sp;
  ASSERT_SUCCESS(parse_as(R"({"offset":2,"id":1,"limit":3})", sp));
  ASSERT_EQUAL(sp.page.limit, 3);
  ASSERT_ERROR(parse_as(R"({"offset":2,"id":1,"limit":3,"page":{}})", sp), simdjson::UNKNOWN_FIELD);
#endif
  TEST_SUCCEED();
}

bool default_replaces_container_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  Settings s;
  ASSERT_SUCCESS(parse_as(R"({"host":"h","tags":["a","b"]})", s));
  ASSERT_EQUAL(s.tags.size(), 2);
  ASSERT_EQUAL(s.tags[0], "a");
#endif
  TEST_SUCCEED();
}

bool run_all() {
  return rename_serialize_test()
      && rename_deserialize_test()
      && rename_roundtrip_test()
      && skip_serialize_test()
      && skip_deserialize_ignores_field_test()
      && mixed_annotations_serialize_test()
      && mixed_annotations_deserialize_test()
      && alias_test()
      && one_way_skip_test()
      && skip_serializing_if_test()
      && default_value_test()
      && struct_default_value_test()
      && default_from_test()
      && with_adapter_test()
      && rename_all_test()
      && deny_unknown_fields_test()
      && transparent_test()
      && enum_annotations_test()
      && long_keys_fallback_test()
      && flatten_serialize_test()
      && flatten_roundtrip_test()
      && flatten_deserialize_test()
      && flatten_deny_unknown_fields_test()
      && default_replaces_container_test();
}

} // namespace annotation_tests

int main() {
  return annotation_tests::run_all() ? EXIT_SUCCESS : EXIT_FAILURE;
}
