#include "simdjson.h"
#include "test_ondemand.h"

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
} // namespace simdjson

namespace schema_tests {

bool test_direct_schema() {
  TEST_START();
  {
    simdjson::padded_string j(std::string(
        R"({"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1,39.9,40.0,39.8],"active":true})"));
    SchemaCar c = simdjson::from(j);
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
    SchemaCar c = simdjson::from(j);
    ASSERT_EQUAL(c.make, std::string("Toyota"));
    ASSERT_EQUAL(c.year, int64_t(2018));
    ASSERT_TRUE(!c.active);
    ASSERT_EQUAL(c.tire_pressure.size(), 4);
  }
  {
    simdjson::padded_string j(std::string(
        R"({"make":"To\"yota","model":"Camry","year":2018,"tire_pressure":[1.0],"active":false})"));
    SchemaCar c = simdjson::from(j);
    ASSERT_EQUAL(c.make, std::string("To\"yota"));
  }
  {
    simdjson::padded_string j(std::string(
        R"({"make":"X","make":"Toyota","model":"Camry","year":2018,"tire_pressure":[],"active":false})"));
    SchemaCar c = simdjson::from(j);
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
    std::vector<SchemaCar> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[0].make, std::string("Toyota"));
    ASSERT_EQUAL(v[1].model, std::string("Fit"));
    ASSERT_EQUAL(v[1].year, int64_t(2020));
    ASSERT_EQUAL(v[1].tire_pressure.size(), 2);
  }
  {
    simdjson::padded_string j(std::string(
        R"({"car":{"make":"Toyota","model":"Camry","year":2018,"tire_pressure":[40.1],"active":true},"bays":3})"));
    SchemaGarage g = simdjson::from(j);
    ASSERT_EQUAL(g.bays, int64_t(3));
    ASSERT_EQUAL(g.car.make, std::string("Toyota"));
    ASSERT_EQUAL(g.car.tire_pressure.size(), 1);
  }
  {
    simdjson::padded_string j(std::string(
        "{\"make\":\"T\xc5\x8dyota\",\"model\":\"Camry\",\"year\":2018,\"tire_pressure\":[1.0],\"active\":true}"));
    SchemaCar c = simdjson::from(j);
    ASSERT_EQUAL(c.make, std::string("T\xc5\x8dyota"));
    ASSERT_EQUAL(c.year, int64_t(2018));
  }
  {
    // Missing key keeps the default.
    simdjson::padded_string j(std::string(
        R"({"make":"Toyota","model":"Camry","tire_pressure":[1.0],"active":true})"));
    SchemaCar c = simdjson::from(j);
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

bool run() {
  return test_direct_schema() && test_direct_schema_string_scan();
}

} // namespace schema_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, schema_tests::run);
}

#endif
