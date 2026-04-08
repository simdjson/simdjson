#include "simdjson.h"
#include "simdjson/padded_string_view.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <string_view>

#if SIMDJSON_CPLUSPLUS17

bool test_padded_input_from_string_view() {
  std::string_view json = "[1,2,3]";
  simdjson::padded_input input(json);
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_from_string_view (iterate error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  size_t count = 0;
  for (auto val : doc) {
    int64_t v;
    error = val.get_int64().get(v);
    if (error) {
      printf("FAILED: test_padded_input_from_string_view (get_int64 error)\n");
      return false;
    }
    count++;
  }
  if (count != 3) {
    printf("FAILED: test_padded_input_from_string_view (expected 3 elements, got %zu)\n", count);
    return false;
  }
  printf("OK: test_padded_input_from_string_view\n");
  return true;
}

bool test_padded_input_from_cstring() {
  const char *json = R"({"key": "value"})";
  size_t len = std::strlen(json);
  simdjson::padded_input input(json, len);
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_from_cstring (iterate error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  std::string_view val;
  error = doc["key"].get_string().get(val);
  if (error) {
    printf("FAILED: test_padded_input_from_cstring (get_string error)\n");
    return false;
  }
  if (val != "value") {
    printf("FAILED: test_padded_input_from_cstring (expected 'value', got '%.*s')\n",
           (int)val.size(), val.data());
    return false;
  }
  printf("OK: test_padded_input_from_cstring\n");
  return true;
}

bool test_padded_input_is_view() {
  // A short string at a "safe" position should typically be a view (no copy).
  // We can't guarantee this in all environments, but we can at least test
  // that is_view() returns a valid boolean and parsing works either way.
  std::string_view json = "[1]";
  simdjson::padded_input input(json);
  bool view = input.is_view();
  printf("  is_view() = %s\n", view ? "true" : "false");
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_is_view (iterate error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  printf("OK: test_padded_input_is_view\n");
  return true;
}

bool test_padded_input_nested_json() {
  std::string_view json = R"({"a":{"b":[1,2,{"c":true}]}})";
  simdjson::padded_input input(json);
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_nested_json (iterate error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  bool c_val;
  error = doc["a"]["b"].at(2)["c"].get_bool().get(c_val);
  if (error) {
    printf("FAILED: test_padded_input_nested_json (get_bool error)\n");
    return false;
  }
  if (!c_val) {
    printf("FAILED: test_padded_input_nested_json (expected true)\n");
    return false;
  }
  printf("OK: test_padded_input_nested_json\n");
  return true;
}

bool test_padded_input_dom() {
  std::string_view json = "[1,2,3]";
  simdjson::padded_input input(json);
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_dom (parse error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  printf("OK: test_padded_input_dom\n");
  return true;
}

bool test_padded_input_dom_object() {
  const char *json = R"({"name":"simdjson","version":42})";
  size_t len = std::strlen(json);
  simdjson::padded_input input(json, len);
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_dom_object (parse error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  std::string_view name;
  error = doc["name"].get(name);
  if (error || name != "simdjson") {
    printf("FAILED: test_padded_input_dom_object (name mismatch)\n");
    return false;
  }
  printf("OK: test_padded_input_dom_object\n");
  return true;
}

bool test_padded_input_from_std_string() {
  std::string json = R"({"key": "value"})";
  // Reserve extra space to test capacity handling
  json.reserve(json.size() + 100);
  simdjson::padded_input input(json);
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(input).get(doc);
  if (error) {
    printf("FAILED: test_padded_input_from_std_string (iterate error: %s)\n",
           simdjson::error_message(error));
    return false;
  }
  std::string_view key;
  error = doc["key"].get(key);
  if (error || key != "value") {
    printf("FAILED: test_padded_input_from_std_string (key/value mismatch)\n");
    return false;
  }
  printf("OK: test_padded_input_from_std_string\n");
  return true;
}

int main() {
  bool ok = true;
  ok = test_padded_input_from_string_view() && ok;
  ok = test_padded_input_from_cstring() && ok;
  ok = test_padded_input_from_std_string() && ok;
  ok = test_padded_input_is_view() && ok;
  ok = test_padded_input_nested_json() && ok;
  ok = test_padded_input_dom() && ok;
  ok = test_padded_input_dom_object() && ok;
  if (ok) {
    printf("\nAll padded_input tests passed.\n");
    return EXIT_SUCCESS;
  } else {
    printf("\nSome padded_input tests FAILED.\n");
    return EXIT_FAILURE;
  }
}

#else // !SIMDJSON_CPLUSPLUS17

int main() {
  printf("padded_input requires C++17 or better. Skipping.\n");
  return EXIT_SUCCESS;
}

#endif // SIMDJSON_CPLUSPLUS17
