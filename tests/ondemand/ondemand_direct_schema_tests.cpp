#include "simdjson.h"
#include "test_ondemand.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// C++20: direct_schema + simdjson::from. Skipped when concepts are off.
#if !SIMDJSON_SUPPORTS_CONCEPTS
int main(void) {
  printf("direct_schema tests require SIMDJSON_SUPPORTS_CONCEPTS\n");
  return 0;
}
#else

namespace schema_tests {

struct SchemaCar {
  std::string make{};
  std::string model{};
  int64_t year{};
  std::vector<double> tire_pressure{};
  bool active{};
};
struct SchemaGarage {
  SchemaCar car{};
  int64_t bays{};
};

simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, SchemaCar &car) {
  simdjson::ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) { return error; }
  for (auto field : obj) {
    simdjson::ondemand::raw_json_string key;
    error = field.key().get(key);
    if (error) { return error; }
    if (key == "make") { error = field.value().get_string(car.make); if (error) { return error; } }
    else if (key == "model") { error = field.value().get_string(car.model); if (error) { return error; } }
    else if (key == "year") { error = field.value().get(car.year); if (error) { return error; } }
    else if (key == "tire_pressure") { error = field.value().get(car.tire_pressure); if (error) { return error; } }
    else if (key == "active") { error = field.value().get(car.active); if (error) { return error; } }
  }
  return simdjson::SUCCESS;
}
simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, SchemaGarage &g) {
  simdjson::ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) { return error; }
  for (auto field : obj) {
    simdjson::ondemand::raw_json_string key;
    error = field.key().get(key);
    if (error) { return error; }
    if (key == "car") { error = field.value().get(g.car); if (error) { return error; } }
    else if (key == "bays") { error = field.value().get(g.bays); if (error) { return error; } }
  }
  return simdjson::SUCCESS;
}


struct SchemaNode {
  std::vector<SchemaNode> kids{};
  int64_t v{};
};
simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, SchemaNode &n) {
  simdjson::ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) { return error; }
  for (auto field : obj) {
    simdjson::ondemand::raw_json_string key;
    error = field.key().get(key);
    if (error) { return error; }
    if (key == "kids") {
      simdjson::ondemand::array arr;
      error = field.value().get_array().get(arr);
      if (error) { return error; }
      n.kids.clear();
      for (auto element : arr) {
        simdjson::ondemand::value child_value;
        error = element.get(child_value);
        if (error) { return error; }
        SchemaNode child;
        error = tag_invoke(simdjson::deserialize_tag{}, child_value, child);
        if (error) { return error; }
        n.kids.push_back(std::move(child));
      }
    } else if (key == "v") { error = field.value().get(n.v); if (error) { return error; } }
  }
  return simdjson::SUCCESS;
}
bool operator==(const SchemaNode &a, const SchemaNode &b) { return a.v == b.v && a.kids == b.kids; }

struct SchemaCounts {
  uint8_t small{};
  uint64_t big{};
  int32_t delta{};
};
simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, SchemaCounts &c) {
  simdjson::ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) { return error; }
  for (auto field : obj) {
    simdjson::ondemand::raw_json_string key;
    error = field.key().get(key);
    if (error) { return error; }
    if (key == "small") { error = field.value().get(c.small); if (error) { return error; } }
    else if (key == "big") { error = field.value().get(c.big); if (error) { return error; } }
    else if (key == "delta") { error = field.value().get(c.delta); if (error) { return error; } }
  }
  return simdjson::SUCCESS;
}
bool operator==(const SchemaCounts &a, const SchemaCounts &b) {
  return a.small == b.small && a.big == b.big && a.delta == b.delta;
}

bool operator==(const SchemaCar &a, const SchemaCar &b) {
  return a.make == b.make && a.model == b.model && a.year == b.year &&
         a.tire_pressure == b.tire_pressure && a.active == b.active;
}
bool operator==(const SchemaGarage &a, const SchemaGarage &b) { return a.car == b.car && a.bays == b.bays; }

} // namespace schema_tests

namespace simdjson {
template<> struct direct_schema<schema_tests::SchemaCar> {
  static constexpr bool enabled = true;
  static constexpr auto fields = std::tuple{
    direct_field<direct_key{"make"}, &schema_tests::SchemaCar::make>{},
    direct_field<direct_key{"model"}, &schema_tests::SchemaCar::model>{},
    direct_field<direct_key{"year"}, &schema_tests::SchemaCar::year>{},
    direct_field<direct_key{"tire_pressure"}, &schema_tests::SchemaCar::tire_pressure>{},
    direct_field<direct_key{"active"}, &schema_tests::SchemaCar::active>{},
  };
};
template<> struct direct_schema<schema_tests::SchemaGarage> {
  static constexpr bool enabled = true;
  static constexpr auto fields = std::tuple{
    direct_field<direct_key{"car"}, &schema_tests::SchemaGarage::car>{},
    direct_field<direct_key{"bays"}, &schema_tests::SchemaGarage::bays>{},
  };
};
template<> struct direct_schema<schema_tests::SchemaNode> {
  static constexpr bool enabled = true;
  static constexpr auto fields = std::tuple{
    direct_field<direct_key{"kids"}, &schema_tests::SchemaNode::kids>{},
    direct_field<direct_key{"v"}, &schema_tests::SchemaNode::v>{},
  };
};
template<> struct direct_schema<schema_tests::SchemaCounts> {
  static constexpr bool enabled = true;
  template <typename F> static void for_each_field(F &&f) {
    f(std::string_view("small"), &schema_tests::SchemaCounts::small);
    f(std::string_view("big"), &schema_tests::SchemaCounts::big);
    f(std::string_view("delta"), &schema_tests::SchemaCounts::delta);
  }
};
} // namespace simdjson

namespace schema_tests {

bool test_direct_schema() {
  TEST_START();
  {
    simdjson::padded_string j(std::string(
        R"({"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1,39.9,40.0,39.8],"active":true})"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("Toyota"));
    ASSERT_EQUAL(c.model, std::string("Camry"));
    ASSERT_EQUAL(c.year, int64_t(2018));
    ASSERT_EQUAL(c.tire_pressure.size(), 4);
    ASSERT_TRUE(c.active);
    ASSERT_EQUAL(c.tire_pressure[0], 40.1);
    ASSERT_EQUAL(c.tire_pressure[3], 39.8);
  }
  {
    simdjson::padded_string j(std::string(
        R"( { "active" : false , "note" : "x" , "model" : "Camry" , "year" : 2018 , "tire_pressure" : [ 40.1 , 39.9 , 40.0 , 39.8 ] , "make" : "Toyota" } )"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("Toyota"));
    ASSERT_EQUAL(c.year, int64_t(2018));
    ASSERT_TRUE(!c.active);
    ASSERT_EQUAL(c.tire_pressure.size(), 4);
  }
  {
    simdjson::padded_string j(std::string(
        R"({"make":"To\"yota","model":"Camry","year":2018,"tire_pressure":[1.0],"active":false})"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("To\"yota"));
  }
  {
    simdjson::padded_string j(std::string(
        R"({"make":"X","make":"Toyota","model":"Camry","year":2018,"tire_pressure":[],"active":false})"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("Toyota"));
  }
  {
    simdjson::padded_string j(std::string(
        R"({"make":"Toyota","model":"Camry","year":"2018","tire_pressure":[1.0],"active":false})"));
    ASSERT_ERROR(simdjson::from(j).result<SchemaCar>().error(), simdjson::INCORRECT_TYPE);
  }
  {
    simdjson::padded_string j(std::string(
        R"([{"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1],"active":true},{"make":"Honda","model":"Fit","year":2020,"tire_pressure":[32.0,32.0],"active":false}])"));
    std::vector<SchemaCar> v;
    ASSERT_SUCCESS(simdjson::from(j).get(v));
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[0].make, std::string("Toyota"));
    ASSERT_EQUAL(v[1].model, std::string("Fit"));
    ASSERT_EQUAL(v[1].year, int64_t(2020));
    ASSERT_EQUAL(v[1].tire_pressure.size(), 2);
  }
  {
    simdjson::padded_string j(std::string(
        R"({"car":{"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1],"active":true},"bays":3})"));
    SchemaGarage g;
    ASSERT_SUCCESS(simdjson::from(j).get(g));
    ASSERT_EQUAL(g.bays, int64_t(3));
    ASSERT_EQUAL(g.car.make, std::string("Toyota"));
    ASSERT_EQUAL(g.car.tire_pressure.size(), 1);
  }
  {
    simdjson::padded_string j(std::string(
        "{\"make\":\"T\xc5\x8dyota\",\"model\":\"Camry\",\"year\":2018,\"tire_pressure\":[1.0],\"active\":true}"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("T\xc5\x8dyota"));
    ASSERT_EQUAL(c.year, int64_t(2018));
  }
  {
    // Missing key keeps the default.
    simdjson::padded_string j(std::string(
        R"({"make":"Toyota","model":"Camry","tire_pressure":[1.0],"active":true})"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.year, int64_t(0));
    ASSERT_EQUAL(c.make, std::string("Toyota"));
  }
  TEST_SUCCEED();
}

bool test_direct_schema_string_scan() {
  TEST_START();
  // Keys out of order go through the general key reader and unknown values
  // are skipped. An escape, a control character or a non-ASCII byte at any
  // position, in a key or a skipped string, must give the On Demand result.
  const char *specials[] = {"\\n", "\\\"", "\\\\", "\\u00e9", "\\x", "\x01", "\xc3\xa9", "\xff"};
  for (size_t n = 1; n <= 24; n++) {
    for (size_t k = 0; k < n; k++) {
      for (const char *special : specials) {
        const std::string text = std::string(k, 'a') + special + std::string(n - k - 1, 'b');
        const std::string docs[] = {
            "{\"model\":\"Camry\",\"note\":\"" + text + "\",\"make\":\"Toyota\",\"year\":2018}",
            "{\"model\":\"Camry\",\"" + text + "\":1,\"make\":\"Toyota\",\"year\":2018}"};
        for (const std::string &json : docs) {
          simdjson::padded_string padded(json);
          simdjson::ondemand::parser parser;
          simdjson::ondemand::document doc;
          SchemaCar expected;
          auto expected_error = parser.iterate(padded).get(doc);
          if (!expected_error) { expected_error = doc.get<SchemaCar>().get(expected); }
          SchemaCar got;
          auto got_error = simdjson::from(padded).get(got);
          ASSERT_EQUAL(got_error, expected_error);
          if (!expected_error) {
            ASSERT_EQUAL(got.make, expected.make);
            ASSERT_EQUAL(got.model, expected.model);
            ASSERT_EQUAL(got.year, expected.year);
          }
        }
      }
    }
  }
  TEST_SUCCEED();
}

// from<T> must give the On Demand result, whichever path it takes.
template <typename T>
bool same_as_ondemand(const std::string &json) {
  simdjson::padded_string padded(json);
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  T expected{};
  auto expected_error = parser.iterate(padded).get(doc);
  if (!expected_error) { expected_error = doc.get<T>().get(expected); }
  T got{};
  auto got_error = simdjson::from(padded).get(got);
  if (got_error != expected_error) {
    std::cerr << "error mismatch on " << json << ": " << got_error << " vs " << expected_error << std::endl;
    return false;
  }
  if (!expected_error && !(got == expected)) {
    std::cerr << "value mismatch on " << json << std::endl;
    return false;
  }
  return true;
}

std::string nested_nodes(size_t depth, const std::string &sep, const std::string &extra) {
  std::string json;
  for (size_t i = 0; i < depth; i++) { json += "{\"kids\":" + sep + "["; }
  json += "{\"kids\":[]" + sep + ",\"v\":0" + extra + "}";
  for (size_t i = depth; i > 0; i--) { json += "]" + sep + ",\"v\":" + std::to_string(i) + extra + "}"; }
  return json;
}

bool test_direct_schema_ordered_fallback() {
  TEST_START();
  // Pretty-printed objects and keys out of schema order after a field
  // already parsed in order.
  const std::string car_docs[] = {
      "{\"make\": \"Toyota\", \"model\": \"Camry\", \"year\": 2018, \"tire_pressure\": [40.1, 39.9], \"active\": true}",
      "{\n  \"make\" : \"Toyota\" ,\n  \"model\" : \"Camry\"\n}",
      R"({"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[1.5],"active":true,"vin":"X1"})",
      R"({"make":"Toyota","vin":{"a":[1,{"b":null}]},"model":"Camry","year":2018})",
      R"({"make":"Toyota","model":"Camry","active":true,"year":2018})",
      R"({"make":"Toyota","model":"Camry","make":"Honda"})",
      R"({"make":"Toyota","model":"Camry",})",
      R"({"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[],"active":true,})",
      R"({"make":"Toyota","model":"Camry" "year":2018})",
      R"({"make":"Toyota","model":"Camry","year":"2018"})",
      R"({"make":"Toyota","model":"Camry","year":2018,"vin":"\u00e9"})",
      "{\"make\":\"T\xc3\xa9\",\"model\":\"Camry\",\"vin\":1}",
      R"({"make":"Toyota" , })",
      R"({"make":"Toyota"})",
      R"({})"};
  for (const std::string &json : car_docs) {
    ASSERT_TRUE(same_as_ondemand<SchemaCar>(json));
  }
  const std::string garage_docs[] = {
      R"({"car":{"make":"Toyota","model":"Camry","year":2018},"bays":3})",
      R"({"car":{"make":"Toyota","model":"Camry","year":2018},"note":"x","bays":3})",
      R"({"bays":3,"car":{"model":"Camry","make":"Toyota"}})",
      "{ \"car\" : { \"make\" : \"Toyota\" , \"year\" : 2018 } , \"bays\" : 3 }"};
  for (const std::string &json : garage_docs) {
    ASSERT_TRUE(same_as_ondemand<SchemaGarage>(json));
  }
  TEST_SUCCEED();
}

bool test_direct_schema_recursive() {
  TEST_START();
  // A key out of order at every level used to re-parse the nested values
  // from the first key, which doubled the work at each level.
  const std::string seps[] = {"", " "};
  const std::string extras[] = {"", ",\"z\":0"};
  for (const std::string &sep : seps) {
    for (const std::string &extra : extras) {
      for (size_t depth : {size_t(0), size_t(1), size_t(5), size_t(100)}) {
        const std::string json = nested_nodes(depth, sep, extra);
        ASSERT_TRUE(same_as_ondemand<SchemaNode>(json));
        SchemaNode node;
        ASSERT_SUCCESS(simdjson::from(simdjson::padded_string(json)).get(node));
        const SchemaNode *n = &node;
        for (size_t i = 1; i <= depth; i++) {
          ASSERT_EQUAL(n->v, int64_t(i));
          ASSERT_EQUAL(n->kids.size(), 1);
          n = &n->kids[0];
        }
        ASSERT_EQUAL(n->v, int64_t(0));
      }
    }
  }
  TEST_SUCCEED();
}

bool test_direct_schema_for_each_field() {
  TEST_START();
  const std::string docs[] = {
      R"({"small":7,"big":9223372036854775807,"delta":-5})",
      R"({ "small" : 7 , "big" : 18446744073709551615 , "delta" : -5 })",
      R"({"delta":-5,"small":7})",
      R"({"small":7,"x":[1,2],"big":1,"delta":2})",
      R"({"small":256})",
      R"({"small":-0})",
      R"({"big":-0})",
      R"({"delta":-0})",
      R"({"delta":2147483648})",
      R"({"small":7,})"};
  for (const std::string &json : docs) {
    ASSERT_TRUE(same_as_ondemand<SchemaCounts>(json));
  }
  TEST_SUCCEED();
}

bool test_direct_schema_reserve() {
  TEST_START();
  // The capacity guess comes from the first element; a tiny first element
  // must not reserve far more than the document can fill.
  std::string json = "[{}";
  for (size_t i = 0; i < 1000; i++) {
    json += R"(,{"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1,39.9,40.0,39.8],"active":true})";
  }
  json += "]";
  std::vector<SchemaCar> cars;
  ASSERT_SUCCESS(simdjson::from(simdjson::padded_string(json)).get(cars));
  ASSERT_EQUAL(cars.size(), 1001);
  ASSERT_TRUE(cars.capacity() <= 2 * json.size() / sizeof(SchemaCar) + 2);
  TEST_SUCCEED();
}

bool test_direct_schema_unicode() {
  TEST_START();
  // Escapes and non-ASCII strings are decoded by the direct parser.
  {
    simdjson::padded_string j(std::string(
        "{\"make\":\"T\xc5\x8dyota\\n\\u00e9\\ud83d\\ude00\\/\\\"\",\"model\":\"\\u0000x\",\"year\":2018}"));
    SchemaCar c;
    ASSERT_SUCCESS(simdjson::from(j).get(c));
    ASSERT_EQUAL(c.make, std::string("T\xc5\x8dyota\n\xc3\xa9\xf0\x9f\x98\x80/\""));
    ASSERT_EQUAL(c.model, std::string("\0x", 2));
    ASSERT_EQUAL(c.year, int64_t(2018));
  }
  const char *pieces[] = {
      "a", "\\\"", "\\\\", "\\/", "\\b", "\\f", "\\n", "\\r", "\\t", "\\u0041", "\\u00E9", "\\u20ac",
      "\\uffff", "\\ud83d\\ude00", "\\uD83D\\uDE00", "\\ud83d", "\\ude00", "\\ude00\\ud83d", "\\ud83d\\u0041",
      "\\ud83d\\n", "\\u12", "\\u12g4", "\\x", "\\", "\xc3\xa9", "\xe2\x82\xac", "\xf0\x9f\x98\x80",
      "\xef\xbf\xbf", "\xf4\x8f\xbf\xbf", "\xc0\x80", "\xc1\xbf", "\xe0\x80\x80", "\xed\xa0\x80",
      "\xf0\x80\x80\x80", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80", "\x80", "\xc3", "\xe2\x82", "\xff",
      "\x01", "\x7f"};
  std::string long_run;
  for (int i = 0; i < 30; i++) { long_run += "\xe4\xb8\xad"; }
  std::vector<std::string> all_pieces(std::begin(pieces), std::end(pieces));
  all_pieces.push_back(long_run);
  all_pieces.push_back(long_run + "\xe4\xb8");
  all_pieces.push_back("\xc3" + long_run);
  for (const std::string &first : all_pieces) {
    for (const std::string &second : all_pieces) {
      const std::string text = "x" + first + "y" + second + "z";
      const std::string docs[] = {
          "{\"make\":\"" + text + "\",\"model\":\"Camry\",\"year\":2018}",
          "{\"make\":\"Toyota\",\"note\":\"" + text + "\",\"year\":2018}",
          "{\"year\":2018,\"" + text + "\":1,\"make\":\"Toyota\"}"};
      for (const std::string &json : docs) {
        ASSERT_TRUE(same_as_ondemand<SchemaCar>(json));
      }
    }
  }
  // Escaped keys fall back; the result must not change.
  ASSERT_TRUE(same_as_ondemand<SchemaCar>(R"({"m\u0061ke":"Toyota","year":2018})"));
  ASSERT_TRUE(same_as_ondemand<SchemaCar>(R"({"make":"Toyota","\u0079ear":2018})"));
  TEST_SUCCEED();
}

bool run() {
  return test_direct_schema() && test_direct_schema_string_scan() &&
         test_direct_schema_ordered_fallback() && test_direct_schema_recursive() &&
         test_direct_schema_for_each_field() && test_direct_schema_reserve() &&
         test_direct_schema_unicode();
}

} // namespace schema_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, schema_tests::run);
}

#endif
