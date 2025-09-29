#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <optional>
#include <memory>

using namespace simdjson;

namespace builder_tests {

#if SIMDJSON_STATIC_REFLECTION
  // Custom type for testing tag_invoke with extract_into
  struct Price {
    double amount;
    std::string currency;

    // Custom deserializer that applies currency conversion
    friend error_code tag_invoke(deserialize_tag,
                                ondemand::value& val,
                                Price& price) noexcept {
      ondemand::object obj;
      auto error = val.get_object().get(obj);
      if (error) return error;

      // Get the raw amount
      error = obj["amount"].get(price.amount);
      if (error) return error;

      // Get the currency
      std::string_view currency_sv;
      error = obj["currency"].get(currency_sv);
      if (error) return error;
      price.currency = std::string(currency_sv);

      // Custom logic: Convert EUR to USD for consistency
      if (price.currency == "EUR") {
        price.amount = price.amount * 1.1;  // Simplified conversion
        price.currency = "USD";
      }

      return SUCCESS;
    }
  };

  struct Product {
    std::string name;
    Price price;  // Custom deserializable type
    int stock;
  };

  // Another custom type for testing
  struct Dimensions {
    double value;
    std::string unit;

    // Custom deserializer that converts to metric
    friend error_code tag_invoke(deserialize_tag,
                                ondemand::value& val,
                                Dimensions& dim) noexcept {
      ondemand::object obj;
      auto error = val.get_object().get(obj);
      if (error) return error;

      error = obj["value"].get(dim.value);
      if (error) return error;

      std::string_view unit_sv;
      error = obj["unit"].get(unit_sv);
      if (error) return error;
      dim.unit = std::string(unit_sv);

      // Convert inches to cm
      if (dim.unit == "inches") {
        dim.value = dim.value * 2.54;
        dim.unit = "cm";
      }

      return SUCCESS;
    }
  };

  struct Package {
    std::string id;
    Dimensions weight;
    Dimensions length;
  };
#endif

  bool test_primitive_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct PrimitiveTypes {
      bool bool_val;
      char char_val;
      int int_val;
      double double_val;
      float float_val;
    };

    PrimitiveTypes test{true, 'X', 42, 3.14159, 2.71f};

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));

    ASSERT_TRUE(json.find("\"bool_val\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"char_val\":\"X\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"int_val\":42") != std::string::npos);
    ASSERT_TRUE(json.find("\"double_val\":3.14159") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    PrimitiveTypes deserialized;
    ASSERT_SUCCESS(doc_result.get<PrimitiveTypes>().get(deserialized));

    ASSERT_EQUAL(deserialized.bool_val, test.bool_val);
    ASSERT_EQUAL(deserialized.char_val, test.char_val);
    ASSERT_EQUAL(deserialized.int_val, test.int_val);
    ASSERT_EQUAL(deserialized.double_val, test.double_val);
#endif
    TEST_SUCCEED();
  }

  bool test_string_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct StringTypes {
      std::string string_val;
      std::string_view string_view_val;
    };

    StringTypes test{"hello world", "test_view"};
    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));

    ASSERT_TRUE(json.find("\"string_val\":\"hello world\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_view_val\":\"test_view\"") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);
    StringTypes deserialized;
    ASSERT_SUCCESS(doc_result.get<StringTypes>().get(deserialized));

    ASSERT_EQUAL(deserialized.string_val, test.string_val);
#endif
    TEST_SUCCEED();
  }

  bool test_optional_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct OptionalTypes {
      std::optional<int> opt_int_with_value;
      std::optional<std::string> opt_string_with_value;
      std::optional<int> opt_int_null;
      std::optional<std::string> opt_string_null;
    };

    OptionalTypes test;
    test.opt_int_with_value = 42;
    test.opt_string_with_value = "optional_test";
    test.opt_int_null = std::nullopt;
    test.opt_string_null = std::nullopt;

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"opt_int_with_value\":42") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_string_with_value\":\"optional_test\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_int_null\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_string_null\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    OptionalTypes deserialized;
    ASSERT_SUCCESS(doc_result.get<OptionalTypes>().get(deserialized));

    ASSERT_TRUE(deserialized.opt_int_with_value.has_value());
    ASSERT_EQUAL(*deserialized.opt_int_with_value, 42);
    ASSERT_TRUE(deserialized.opt_string_with_value.has_value());
    ASSERT_EQUAL(*deserialized.opt_string_with_value, "optional_test");
    ASSERT_FALSE(deserialized.opt_int_null.has_value());
    ASSERT_FALSE(deserialized.opt_string_null.has_value());
#endif
    TEST_SUCCEED();
  }

  bool test_smart_pointer_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct SmartPointerTypes {
      std::unique_ptr<int> unique_int_with_value;
      std::shared_ptr<std::string> shared_string_with_value;
      std::unique_ptr<bool> unique_bool_with_value;
      std::unique_ptr<int> unique_int_null;
      std::shared_ptr<std::string> shared_string_null;
    };

    SmartPointerTypes test;
    test.unique_int_with_value = std::make_unique<int>(123);
    test.shared_string_with_value = std::make_shared<std::string>("shared_test");
    test.unique_bool_with_value = std::make_unique<bool>(true);
    test.unique_int_null = nullptr;
    test.shared_string_null = nullptr;

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"unique_int_with_value\":123") != std::string::npos);
    ASSERT_TRUE(json.find("\"shared_string_with_value\":\"shared_test\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_bool_with_value\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_int_null\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"shared_string_null\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));

    SmartPointerTypes deserialized;
    ASSERT_SUCCESS(doc_result.get<SmartPointerTypes>().get(deserialized));
    ASSERT_TRUE(deserialized.unique_int_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.unique_int_with_value, 123);
    ASSERT_TRUE(deserialized.shared_string_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.shared_string_with_value, "shared_test");
    ASSERT_TRUE(deserialized.unique_bool_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.unique_bool_with_value, true);
    ASSERT_TRUE(deserialized.unique_int_null == nullptr);
    ASSERT_TRUE(deserialized.shared_string_null == nullptr);
#endif
    TEST_SUCCEED();
  }

  bool test_container_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    // Test basic container types
    struct ContainerTypes {
      std::vector<int> int_vector;
      std::set<std::string> string_set;
      std::map<std::string, int> string_map;
    };

    ContainerTypes test;
    test.int_vector = {1, 2, 3, 4, 5};
    test.string_set = {"apple", "banana", "cherry"};
    test.string_map = {{"key1", 10}, {"key2", 20}, {"key3", 30}};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"int_vector\":[1,2,3,4,5]") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_set\":[") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_map\":{") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);
    ContainerTypes deserialized;
    ASSERT_SUCCESS(doc_result.get<ContainerTypes>().get(deserialized));
    ASSERT_EQUAL(deserialized.int_vector.size(), 5);
    ASSERT_EQUAL(deserialized.string_set.size(), 3);
    ASSERT_EQUAL(deserialized.string_map.size(), 3);
    ASSERT_EQUAL(deserialized.int_vector[0], 1);
    ASSERT_EQUAL(deserialized.int_vector[4], 5);

    // Test std::list with iterator-based serialization
    struct ListContainer {
      std::list<int> int_list;
      std::list<std::string> string_list;
    };

    ListContainer list_test;
    list_test.int_list = {10, 20, 30, 40, 50};
    list_test.string_list = {"first", "second", "third"};

    std::string list_result;
    ASSERT_SUCCESS( builder::to_json_string(list_test).get(list_result));

    // Check that list serialization produces correct JSON array format
    ASSERT_TRUE(list_result.find("\"int_list\":[10,20,30,40,50]") != std::string::npos);
    ASSERT_TRUE(list_result.find("\"string_list\":[\"first\",\"second\",\"third\"]") != std::string::npos);

    // Test list round-trip
    auto list_doc_result = parser.iterate(pad(list_result));
    ASSERT_SUCCESS(list_doc_result);

    ListContainer list_deserialized;
    ASSERT_SUCCESS(list_doc_result.get<ListContainer>().get(list_deserialized));
    ASSERT_EQUAL(list_deserialized.int_list.size(), 5);
    ASSERT_EQUAL(list_deserialized.string_list.size(), 3);

    // Check list values
    auto it = list_deserialized.int_list.begin();
    ASSERT_EQUAL(*it, 10);
    std::advance(it, 4);
    ASSERT_EQUAL(*it, 50);

    auto str_it = list_deserialized.string_list.begin();
    ASSERT_EQUAL(*str_it, "first");
    std::advance(str_it, 2);
    ASSERT_EQUAL(*str_it, "third");
#endif
    TEST_SUCCEED();
  }

  bool test_extract_into() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Car {
      std::string make;
      std::string model;
      int year;
      double price;
      std::optional<std::string> color;
    };

    ondemand::parser parser;

    // Test 1: Extract only specific fields
    {
      auto padded = R"({
        "make": "Toyota",
        "model": "Camry",
        "year": 2024,
        "price": 28999.99,
        "color": "Blue",
        "engine": "V6",
        "transmission": "Automatic"
      })"_padded;

      Car car{};
      ondemand::document doc;
      ASSERT_SUCCESS( parser.iterate(padded).get(doc) );

      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));

            // Extract only 'make' and 'model'
      ASSERT_SUCCESS(obj.extract_into<"make","model">(car));

      ASSERT_EQUAL(car.make, "Toyota");
      ASSERT_EQUAL(car.model, "Camry");
      ASSERT_EQUAL(car.year, 0);  // Not extracted
      ASSERT_EQUAL(car.price, 0);  // Not extracted

      ASSERT_SUCCESS(parser.iterate(padded).get(doc));

      // Extract only 'make' and 'model'
      ASSERT_SUCCESS(doc.extract_into<"make", "model">(car));

      ASSERT_EQUAL(car.make, "Toyota");
      ASSERT_EQUAL(car.model, "Camry");
      ASSERT_EQUAL(car.year, 0);  // Not extracted
      ASSERT_EQUAL(car.price, 0);  // Not extracted
    }

    // Test 2: Extract with optional field
    {
      auto padded = R"({
        "make": "Honda",
        "model": "Accord",
        "year": 2023,
        "price": 26999.99,
        "color": "Red"
      })"_padded;

      Car car{};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(padded).get(doc));

      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));

      // Extract including optional 'color'
      ASSERT_SUCCESS(obj.extract_into<"make", "model", "color">(car));

      ASSERT_EQUAL(car.make, "Honda");
      ASSERT_EQUAL(car.model, "Accord");
      ASSERT_TRUE(car.color.has_value());
      ASSERT_EQUAL(*car.color, "Red");
    }

    // Test 3: Extract with missing optional field
    {
      auto padded = R"({
        "make": "Ford",
        "model": "F-150",
        "year": 2024,
        "price": 35999.99
      })"_padded;

      Car car{};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(padded).get(doc));

      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));

      // Try to extract including optional 'color' which doesn't exist
      ASSERT_SUCCESS(obj.extract_into<"make", "model", "color">(car));

      ASSERT_EQUAL(car.make, "Ford");
      ASSERT_EQUAL(car.model, "F-150");
      ASSERT_FALSE(car.color.has_value());  // Should be empty
    }

    // Test 4: Extract with custom deserializable type using tag_invoke
    {
      // Test using the Price struct defined at namespace level

      auto padded = R"({
        "name": "Laptop",
        "price": {
          "amount": 1000,
          "currency": "EUR"
        },
        "stock": 15,
        "description": "High-end gaming laptop"
      })"_padded;

      Product product{};
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      ondemand::object obj;
      auto obj_result = doc.get_object().get(obj);
      ASSERT_SUCCESS(obj_result);

      // Extract including price field with custom deserializer
      ASSERT_SUCCESS(obj.extract_into<"name", "price">(product));

      ASSERT_EQUAL(product.name, "Laptop");
      // Verify custom deserializer was invoked: EUR should be converted to USD
      ASSERT_EQUAL(product.price.currency, "USD");  // Should be converted from EUR
      // Verify custom deserializer was invoked: amount should be 1100 (1000 * 1.1)
      ASSERT_EQUAL(product.price.amount, 1100);  // Should be exactly 1100 after conversion
      ASSERT_EQUAL(product.stock, 0);  // Not extracted
    }

    // Test 5: Extract with nested custom deserializable types
    {
      // Test using the Dimensions struct defined at namespace level

      auto padded = R"({
        "id": "PKG123",
        "weight": {
          "value": 10,
          "unit": "pounds"
        },
        "length": {
          "value": 12,
          "unit": "inches"
        },
        "fragile": true
      })"_padded;

      Package pkg{};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(padded).get(doc));

      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));

      // Extract only length (with custom deserializer), skip weight
      ASSERT_SUCCESS(obj.extract_into<"id", "length">(pkg));

      ASSERT_EQUAL(pkg.id, "PKG123");
      // Verify custom deserializer was invoked: inches should be converted to cm
      ASSERT_EQUAL(pkg.length.unit, "cm");  // Should be converted from inches
      // Verify exact conversion: 12 inches * 2.54 = 30.48 cm
      ASSERT_EQUAL(pkg.length.value, 30.48);  // Should be exactly 30.48 after conversion
      ASSERT_EQUAL(pkg.weight.unit, "");  // Weight not extracted
      ASSERT_EQUAL(pkg.weight.value, 0);  // Weight value should remain at default
    }
#endif
    TEST_SUCCEED();
  }


  bool test_extract_from() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    // Test 1: Extract specific fields from Car struct
    {
      struct Car {
        std::string make;
        std::string model;
        int year;
        double price;
        bool electric;
      };

      Car car{"Tesla", "Model 3", 2023, 42000.0, true};

      // Extract only make and model
      std::string json;
      ASSERT_SUCCESS((extract_from<"make", "model">(car).get(json)));

      // Parse back to verify correctness
      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      std::string_view make;
      std::string_view model;
      ASSERT_SUCCESS(doc["make"].get(make));
      ASSERT_SUCCESS(doc["model"].get(model));
      ASSERT_EQUAL(make, "Tesla");
      ASSERT_EQUAL(model, "Model 3");

      // Verify excluded fields are not present
      auto year_result = doc["year"];
      ASSERT_ERROR(year_result.error(), NO_SUCH_FIELD);
      auto price_result = doc["price"];
      ASSERT_ERROR(price_result.error(), NO_SUCH_FIELD);
      auto electric_result = doc["electric"];
      ASSERT_ERROR(electric_result.error(), NO_SUCH_FIELD);
    }

    // Test 2: Extract different field combination
    {
      struct Car {
        std::string make;
        std::string model;
        int year;
        double price;
        bool electric;
      };

      Car car{"Ford", "F-150", 2024, 55000.0, false};

      // Extract year and price
      std::string json;
      ASSERT_SUCCESS((extract_from<"year", "price">(car).get(json)));

      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      int64_t year;
      double price;
      ASSERT_SUCCESS(doc["year"].get(year));
      ASSERT_SUCCESS(doc["price"].get(price));
      ASSERT_EQUAL(year, 2024);
      ASSERT_EQUAL(price, 55000.0);

      // Verify excluded fields
      auto make_result = doc["make"];
      ASSERT_ERROR(make_result.error(), NO_SUCH_FIELD);
    }

    // Test 3: Extract from struct with optional fields
    {
      struct Person {
        std::string name;
        int age;
        std::optional<std::string> email;
        std::optional<std::string> phone;
      };

      Person person{"John Doe", 30, "john@example.com", std::nullopt};

      // Extract name and email
      std::string json;
      ASSERT_SUCCESS((extract_from<"name", "email">(person).get(json)));

      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      std::string_view name;
      std::string_view email;
      ASSERT_SUCCESS(doc["name"].get(name));
      ASSERT_SUCCESS(doc["email"].get(email));
      ASSERT_EQUAL(name, "John Doe");
      ASSERT_EQUAL(email, "john@example.com");
    }

    // Test 4: Extract with optional that has value
    {
      struct Person {
        std::string name;
        int age;
        std::optional<std::string> email;
        std::optional<std::string> phone;
      };

      Person person{"Jane Smith", 25, "jane@example.com", "555-1234"};

      // Extract name, age, and phone
      std::string json;
      ASSERT_SUCCESS((extract_from<"name", "age", "phone">(person).get(json)));

      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      std::string_view name;
      int64_t age;
      std::string_view phone;
      ASSERT_SUCCESS(doc["name"].get(name));
      ASSERT_SUCCESS(doc["age"].get(age));
      ASSERT_SUCCESS(doc["phone"].get(phone));
      ASSERT_EQUAL(name, "Jane Smith");
      ASSERT_EQUAL(age, 25);
      ASSERT_EQUAL(phone, "555-1234");

      // Email should not be present
      auto email_result = doc["email"];
      ASSERT_ERROR(email_result.error(), NO_SUCH_FIELD);
    }

    // Test 5: Round-trip test - serialize with extract_from, deserialize with extract_into
    {
      struct Product {
        std::string id;
        std::string name;
        double price;
        int stock;
      };

      Product original{"P123", "Widget", 19.99, 100};

      // Extract specific fields to JSON
      std::string json;
      ASSERT_SUCCESS((extract_from<"id", "name", "price">(original).get(json)));

      // Parse and extract back
      Product restored{"", "", 0.0, 0};
      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      auto extract_result = obj.extract_into<"id", "name", "price">(restored);
      ASSERT_SUCCESS(extract_result);

      // Verify fields match
      ASSERT_EQUAL(restored.id, original.id);
      ASSERT_EQUAL(restored.name, original.name);
      ASSERT_EQUAL(restored.price, original.price);
      ASSERT_EQUAL(restored.stock, 0);  // Stock should remain at default
    }

    // Test 6: Extract from nested structs
    {
      struct Address {
        std::string street;
        std::string city;
        std::string zip;
      };

      struct Company {
        std::string name;
        Address headquarters;
        int employees;
      };

      Company company{"TechCorp", {"123 Main St", "San Francisco", "94105"}, 500};

      // Extract name and employees only
      std::string json;
      ASSERT_SUCCESS((extract_from<"name", "employees">(company).get(json)));

      auto padded = pad(json);
      ondemand::parser parser;
      auto doc = parser.iterate(padded);
      ASSERT_SUCCESS(doc);

      std::string_view name;
      int64_t employees;
      ASSERT_SUCCESS(doc["name"].get(name));
      ASSERT_SUCCESS(doc["employees"].get(employees));
      ASSERT_EQUAL(name, "TechCorp");
      ASSERT_EQUAL(employees, 500);

      // headquarters should not be present
      auto hq_result = doc["headquarters"];
      ASSERT_ERROR(hq_result.error(), NO_SUCH_FIELD);
    }
#endif
    TEST_SUCCEED();
  }

  bool run() {
    return test_primitive_types() &&
           test_string_types() &&
           test_optional_types() &&
           test_smart_pointer_types() &&
           test_container_types() &&
           test_extract_into() &&
           test_extract_from();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}