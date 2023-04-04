#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace object_tests {
  using namespace std;
  using simdjson::ondemand::json_type;

  bool issue1979() {
    TEST_START();
    auto json = R"({
  "@avito-core/toggles:6.1.18": {
    "add_model_review_from": true
  }
 })"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object object;
    ASSERT_SUCCESS(doc.get_object().get(object));
    ASSERT_SUCCESS(object["@avito-core/toggles:6.1.18"].get_object().error());
    TEST_SUCCEED();
  }

  bool issue1977() {
    TEST_START();
    auto json = R"({"1": 2} foo })"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object object;
    ASSERT_SUCCESS(doc.get_object().get(object));
    for (auto values : object) {
      ASSERT_SUCCESS(values);
    }
    ASSERT_FALSE(doc.at_end());
    TEST_SUCCEED();
  }

  bool issue1745() {
    TEST_START();
    auto json = R"({
  "keys": [
    {
      "kid": "bbd2ac7c4c5eb8adc8eeffbc8f5a2dd6cf7545e4",
      "e": "AQAB",
      "alg": "RS256",
      "use": "sig",
      "n": "y930dtGTeMG52IPsKmMuEpPHLaxuYQlduZd6BqFVjc2-UFZR8fNqtnYzAjbXWJD_Tqxgdlj_MW4vogvX4sHwVpZONvdyeGoIyDQtis6iuGQhQamV85F_JbrEUnEw3QCO87Liz5UXG6BK2HRyPhDfMex1_tO0ROmySLFdCTS17D0wah71Ibpi0gI8LUi6kzVRjYDIC1oE-iK3Y9s88Bi4ZGYJxXAbnNwbwVkGOKCXja9k0jjBGRxZD-4KDuf493lFOOEGSLDA2Qp9rDqrURP12XYgvf_zJx_kSDipnr0gL6Vz2n3H4-XN4tA45zuzRkHoE7-XexPq-tv7kQ8pSjY2uQ",
      "kty": "RSA"
    },
    {
      "kty": "RSA",
      "n": "zMHxWuxztMKXdBhv3rImlUvW_yp6nO03cVXPyA0Vyq0-M7LfOJJIF-OdNoRGdsFPHVKCFoo6qGhR8rBCmMxA4fM-Ubk5qKuUqCN9eP3yZJq8Cw9tUrt_qh7uW-qfMr0upcyeSHhC_zW1lTGs5sowDorKN_jQ1Sfh9hfBxfc8T7dQAAgEqqMcE3u-2J701jyhJz0pvurCfziiB3buY6SGREhBQwNwpnQjt_lE2U4km8FS0woPzt0ccE3zsGL2qM-LWZbOm9aXquSnqNJLt3tGVvShnev-GiJ1XfQ3EWm0f4w0TX9fTOkxstl0vo_vW_FjGQ0D1pXSjqb7n-hAdXwc9w",
      "alg": "RS256",
      "kid": "85828c59284a69b54b27483e487c3bd46cd2a2b3",
      "e": "AQAB",
      "use": "sig"
    }
  ]
}

  })"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    for (auto items_instance : doc["keys"]) {
      ondemand::object items;
      ASSERT_SUCCESS(items_instance.get_object().get(items));
      std::string_view kty_value;
      ASSERT_SUCCESS(items["kty"].get_string().get(kty_value));
      std::string_view use_value;
      ASSERT_SUCCESS(items["use"].get_string().get(use_value));
      if (kty_value != "RSA" && use_value != "sig") { continue; }
      std::string_view str;
      ASSERT_SUCCESS(items["alg"].get_string().get(str));
      ASSERT_EQUAL(str, "RS256");
      ASSERT_EQUAL(use_value, "sig");
      ASSERT_EQUAL(kty_value, "RSA");
      ASSERT_SUCCESS(items["n"].get_string().get(str));
      ASSERT_EQUAL(str.size(), 342);
      ASSERT_SUCCESS(items["e"].get_string().get(str));
      ASSERT_EQUAL(str, "AQAB");
    }
    TEST_SUCCEED();
  }

  bool issue1723() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({
    "contents":
    {
      "bids":[
         {"offset":"1", "price":"1.1", "size":"4.1"}
      ],
      "asks":[
        {"offset":"1111", "price":"1.100", "size":"114.1"}
      ]

    }
    })"_padded;
    simdjson::ondemand::document doc;
    auto error = parser.iterate(docdata).get(doc);
    if(error) { return false; }
    ondemand::object content;
    error = doc.get_object().find_field("contents").get(content);
    if(error) { return false; }
    ondemand::array bids;
    error = content["bids"].get_array().get(bids);
    if(error) { return false; }
    for (auto aux : bids) {
      uint64_t id;
      error = aux["offset"].get_uint64_in_string().get(id);
      if(error) { return false; }
      double price;
      error = aux["price"].get_double_in_string().get(price);
      if(error) { return false; }
      double size;
      error = aux["size"].get_double_in_string().get(size);
      if(error) { return false; }
      std::cout << id << " " << price << " " << size << std::endl;
    }
    ondemand::array asks;
    error = content["asks"].get_array().get(asks);
    if(error) { return false; }
    for (auto aux : asks) {
      uint64_t id;
      error = aux["offset"].get_uint64_in_string().get(id);
      if(error) { return false; }
      double price;
      error = aux["price"].get_double_in_string().get(price);
      if(error) { return false; }
      double size;
      error = aux["size"].get_double_in_string().get(size);
      if(error) { return false; }
      std::cout << id << " " << price << " " << size << std::endl;
    }
    return true;
  }

  // In this test, no non-trivial object in an array have a missing key
  bool no_missing_keys() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"([{"a":"a"},{}])"_padded;
    simdjson::ondemand::document doc;
    auto error = parser.iterate(docdata).get(doc);
    if(error != simdjson::SUCCESS) { return false; }
    simdjson::ondemand::array a;
    error = doc.get_array().get(a);
    if(error != simdjson::SUCCESS) { return false; }
    size_t counter{0};
    for(auto elem : a) {
      error = elem.find_field_unordered("a").error();
      if(counter == 0) {
        ASSERT_EQUAL( error, simdjson::SUCCESS);
      } else {
        ASSERT_EQUAL( error, simdjson::NO_SUCH_FIELD);
      }
      counter++;
    }
    return true;
  }

  bool missing_key_continue() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({"a":0, "b":1, "c":2})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    int64_t num;
    ASSERT_SUCCESS(doc["a"].get(num));
    ASSERT_EQUAL(num, 0);
    ASSERT_SUCCESS(doc["b"].get(num));
    ASSERT_EQUAL(num, 1);
    ASSERT_SUCCESS(doc["c"].get(num));
    ASSERT_EQUAL(num, 2);
    // Start again, but omit a key
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc["a"].get(num));
    ASSERT_EQUAL(num, 0);
    ASSERT_SUCCESS(doc["c"].get(num));
    ASSERT_EQUAL(num, 2);
    // Start again, but request a missing key
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    simdjson::ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    assert_error(obj["d"].get(num), NO_SUCH_FIELD);
    ASSERT_SUCCESS(obj["c"].get(num));
    ASSERT_EQUAL(num, 2);
    // Start again, but request a missing key first
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_object().get(obj));
    assert_error(obj["d"].get(num), NO_SUCH_FIELD);
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    // Start again, but request a missing key twice
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_object().get(obj));
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    assert_error(obj["d"].get(num), NO_SUCH_FIELD);
    assert_error(obj["z"].get(num), NO_SUCH_FIELD);
    // Because we do a full circle, you can query the same
    // key twice!!!
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    ASSERT_SUCCESS(obj["b"].get(num));
    ASSERT_EQUAL(num, 1);
    ASSERT_SUCCESS(obj["b"].get(num));
    ASSERT_EQUAL(num, 1);
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    assert_error(obj["d"].get(num), NO_SUCH_FIELD);
    ASSERT_SUCCESS(obj["a"].get(num));
    ASSERT_EQUAL(num, 0);
    TEST_SUCCEED();
  }

  bool missing_keys() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"([{"a":"a"},{}])"_padded;
    simdjson::ondemand::document doc;
    auto error = parser.iterate(docdata).get(doc);
    if(error != simdjson::SUCCESS) { return false; }
    simdjson::ondemand::array a;
    error = doc.get_array().get(a);
    if(error != simdjson::SUCCESS) { return false; }
    for(auto elem : a) {
      error = elem.find_field_unordered("keynotfound").error();
      if(error != simdjson::NO_SUCH_FIELD) {
        std::cout << error << std::endl;
        return false;
      }
    }
    return true;
  }

  bool missing_keys_for_empty_top_level_object() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata = "{}"_padded;
    simdjson::ondemand::document doc;
    auto error = parser.iterate(docdata).get(doc);
    if(error != simdjson::SUCCESS) { return false; }
    error = doc.find_field_unordered("keynotfound").error();
    if(error != simdjson::NO_SUCH_FIELD) {
      std::cout << error << std::endl;
      return false;
    }
    return true;
  }

#if SIMDJSON_EXCEPTIONS

  bool issue1965() {
    TEST_START();
    std::string str = "{\"query\":\"ah\"}";
    std::unique_ptr<char[]> buffer(new char[str.size() + simdjson::SIMDJSON_PADDING]);
    memcpy(buffer.get(), str.data(), str.size());
    simdjson::padded_string_view view(buffer.get(), str.size(), str.size() + simdjson::SIMDJSON_PADDING);
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(view);
    simdjson::ondemand::object root = doc.get_object();
    simdjson::ondemand::value query = root.find_field("query");
    simdjson::ondemand::raw_json_string raw = query.get_raw_json_string();
    std::unique_ptr<uint8_t[]> dst_buffer(new uint8_t[3 + simdjson::SIMDJSON_PADDING]);
    uint8_t * dst = dst_buffer.get();
    std::string_view fieldstring = parser.unescape(raw, dst);
    std::cout << fieldstring << std::endl;
    TEST_SUCCEED();
  }

  bool issue1745_with_exceptions() {
    TEST_START();
    auto json = R"({
  "keys": [
    {
      "kid": "bbd2ac7c4c5eb8adc8eeffbc8f5a2dd6cf7545e4",
      "e": "AQAB",
      "alg": "RS256",
      "use": "sig",
      "n": "y930dtGTeMG52IPsKmMuEpPHLaxuYQlduZd6BqFVjc2-UFZR8fNqtnYzAjbXWJD_Tqxgdlj_MW4vogvX4sHwVpZONvdyeGoIyDQtis6iuGQhQamV85F_JbrEUnEw3QCO87Liz5UXG6BK2HRyPhDfMex1_tO0ROmySLFdCTS17D0wah71Ibpi0gI8LUi6kzVRjYDIC1oE-iK3Y9s88Bi4ZGYJxXAbnNwbwVkGOKCXja9k0jjBGRxZD-4KDuf493lFOOEGSLDA2Qp9rDqrURP12XYgvf_zJx_kSDipnr0gL6Vz2n3H4-XN4tA45zuzRkHoE7-XexPq-tv7kQ8pSjY2uQ",
      "kty": "RSA"
    },
    {
      "kty": "RSA",
      "n": "zMHxWuxztMKXdBhv3rImlUvW_yp6nO03cVXPyA0Vyq0-M7LfOJJIF-OdNoRGdsFPHVKCFoo6qGhR8rBCmMxA4fM-Ubk5qKuUqCN9eP3yZJq8Cw9tUrt_qh7uW-qfMr0upcyeSHhC_zW1lTGs5sowDorKN_jQ1Sfh9hfBxfc8T7dQAAgEqqMcE3u-2J701jyhJz0pvurCfziiB3buY6SGREhBQwNwpnQjt_lE2U4km8FS0woPzt0ccE3zsGL2qM-LWZbOm9aXquSnqNJLt3tGVvShnev-GiJ1XfQ3EWm0f4w0TX9fTOkxstl0vo_vW_FjGQ0D1pXSjqb7n-hAdXwc9w",
      "alg": "RS256",
      "kid": "85828c59284a69b54b27483e487c3bd46cd2a2b3",
      "e": "AQAB",
      "use": "sig"
    }
  ]
}

  })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    for (auto items : doc["keys"]) {
      std::string_view kty_value = items["kty"].get_string();
      std::string_view use_value = items["use"].get_string();
      if (kty_value != "RSA" && use_value != "sig") { continue; }
      std::string_view str = items["alg"].get_string();
      if(str != "RS256") { return false; }
      if(use_value != "sig") { return false; }
      if(kty_value != "RSA") { return false; }
      str = items["n"].get_string();
      if(str.size() != 342) { return false; }
      str = items["e"].get_string();
      if(str != "AQAB") { return false; }
    }
    TEST_SUCCEED();
  }

  bool issue1723_except() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({
    "contents":
    {
      "bids":[
         {"offset":"1", "price":"1.1", "size":"4.1"}
      ],
      "asks":[
        {"offset":"1111", "price":"1.100", "size":"114.1"}
      ]

    }
    })"_padded;
    simdjson::ondemand::document doc = parser.iterate(docdata);
    ondemand::object content = doc.get_object().find_field("contents");
    ondemand::array bids = content["bids"].get_array();
    std::cout << "bids:" << std::endl;
    for (auto aux : bids) {
      uint64_t id = aux["offset"].get_uint64_in_string();
      double price = aux["price"].get_double_in_string();
      double size = aux["size"].get_double_in_string();
      std::cout << id << " " << price << " " << size << std::endl;
    }
    ondemand::array asks = content["asks"].get_array();
    std::cout << "asks:" << std::endl;
    for (auto aux : asks) {
      uint64_t id = aux["offset"].get_uint64_in_string();
      double price = aux["price"].get_double_in_string();
      double size = aux["size"].get_double_in_string();
      std::cout << id << " " << price << " " << size << std::endl;
    }
    return true;
  }
  // used in issue_1521
  // difficult to use as a lambda because it is recursive.
  void broken_descend(ondemand::object node) {
    if(auto type = node.find_field_unordered("type"); type.error() == SUCCESS && type == "child") {
      auto n = node.find_field_unordered("name");
      if(n.error() == simdjson::SUCCESS) {
          std::cout << std::string_view(n) << std::endl;
      }
    } else {
     for (ondemand::object child_node : node["nodes"]) { broken_descend(child_node); }
    }
  }

  bool broken_issue_1521() {
    TEST_START();
    ondemand::parser parser;
    padded_string json = R"({"type":"root","nodes":[{"type":"child","nodes":[]},{"type":"child","name":"child-name","nodes":[]}]})"_padded;
    ondemand::document file_tree = parser.iterate(json);
    try {
      broken_descend(file_tree);
    } catch(simdjson::simdjson_error& e) {
      std::cout << "The document is valid JSON: " << json << std::endl;
      TEST_FAIL(e.error());
    }
    TEST_SUCCEED();
  }

  bool fixed_broken_issue_1521() {
    TEST_START();
    ondemand::parser parser;
    // We omit the ',"nodes":[]'
    padded_string json = R"({"type":"root","nodes":[{"type":"child"},{"type":"child","name":"child-name","nodes":[]}]})"_padded;
    ondemand::document file_tree = parser.iterate(json);
    try {
      broken_descend(file_tree);
    } catch(simdjson::simdjson_error& e) {
      std::cout << "The document is valid JSON: " << json << std::endl;
      TEST_FAIL(e.error());
    }
    TEST_SUCCEED();
  }

  // used in issue_1521
  // difficult to use as a lambda because it is recursive.
  void descend(ondemand::object node) {
    auto n = node.find_field_unordered("name");
    if(auto type = node.find_field_unordered("type"); type.error() == SUCCESS && type == "child") {
      if(n.error() == simdjson::SUCCESS) {
          std::cout << std::string_view(n) << std::endl;
      }
    } else {
     for (ondemand::object child_node : node["nodes"]) { descend(child_node); }
    }
  }

  bool issue_1521() {
    TEST_START();
    ondemand::parser parser;
    padded_string json = R"({"type":"root","nodes":[{"type":"child","nodes":[]},{"type":"child","name":"child-name","nodes":[]}]})"_padded;
    ondemand::document file_tree = parser.iterate(json);
    try {
      descend(file_tree);
    } catch(simdjson::simdjson_error& e) {
      std::cout << "The document is valid JSON: " << json << std::endl;
      TEST_FAIL(e.error());
    }
    TEST_SUCCEED();
  }
#endif

  bool iterate_object() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    const uint64_t expected_value[] = { 1, 2, 3 };
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        ASSERT_SUCCESS( field.key().error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    SUBTEST("ondemand::object-document-rewind", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        ASSERT_SUCCESS( field.error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      doc_result.rewind();
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      i = 0;
      for (auto field : object) {
        ASSERT_SUCCESS( field.error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    SUBTEST("ondemand::object-rewind", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        ASSERT_SUCCESS( field.error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      object.reset();
      i = 0;
      for (auto field : object) {
        ASSERT_SUCCESS( field.error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      size_t i = 0;
      for (auto field : object_result) {
        ASSERT_SUCCESS( field.error() );
        ASSERT_EQUAL( field.key().value_unsafe(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_object_partial_children() {
    TEST_START();
    auto json = R"(
      {
        "scalar_ignore":       0,
        "empty_array_ignore":  [],
        "empty_object_ignore": {},
        "object_break":        { "x": 3, "y": 33 },
        "object_break_unused": { "x": 4, "y": 44 },
        "object_index":        { "x": 5, "y": 55 },
        "object_index_unused": { "x": 6, "y": 66 },
        "array_break":         [ 7, 77, 777 ],
        "array_break_unused":  [ 8, 88, 888 ],
        "quadruple_nested_break": { "a": [ { "b": [ 9, 99 ], "c": 999 }, 9999 ], "d": 99999 },
        "actual_value": 10
      }
    )"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        ondemand::raw_json_string key;
        ASSERT_SUCCESS( field.key().get(key) );

        switch (i) {
          case 0: {
            ASSERT_EQUAL(key, "scalar_ignore");
            std::cout << "  - After ignoring empty scalar ..." << std::endl;
            break;
          }
          case 1: {
            ASSERT_EQUAL(key, "empty_array_ignore");
            std::cout << "  - After ignoring empty array ..." << std::endl;
            break;
          }
          case 2: {
            ASSERT_EQUAL(key, "empty_object_ignore");
            std::cout << "  - After ignoring empty object ..." << std::endl;
            break;
          }
          // Break after using first value in child object
          case 3: {
            ASSERT_EQUAL(key, "object_break");

            for (auto child_field : field.value().get_object()) {
              ASSERT_SUCCESS( child_field.error() );
              ASSERT_EQUAL(child_field.key().value_unsafe(), "x");
              uint64_t x;
              ASSERT_SUCCESS( child_field.value().get(x) );
              ASSERT_EQUAL(x, 3);
              break; // Break after the first value
            }
            std::cout << "  - After using first value in child object ..." << std::endl;
            break;
          }

          // Break without using first value in child object
          case 4: {
            ASSERT_EQUAL(key, "object_break_unused");

            for (auto child_field : field.value().get_object()) {
              ASSERT_SUCCESS(child_field.error());
              ASSERT_EQUAL(child_field.key().value_unsafe(), "x");
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object
          case 5: {
            ASSERT_EQUAL(key, "object_index");

            uint64_t x;
            ASSERT_SUCCESS( field.value()["x"].get(x) );
            ASSERT_EQUAL( x, 5 );
            std::cout << "  - After looking up one field in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object, but don't use it
          case 6: {
            ASSERT_EQUAL(key, "object_index_unused");

            ASSERT_SUCCESS( field.value()["x"] );
            std::cout << "  - After looking up (but not using) one field in child object ..." << std::endl;
            break;
          }

          // Break after first value in child array
          case 7: {
            ASSERT_EQUAL(key, "array_break");
            for (auto child_value : field.value()) {
              uint64_t x;
              ASSERT_SUCCESS( child_value.get(x) );
              ASSERT_EQUAL( x, 7 );
              break;
            }
            std::cout << "  - After using first value in child array ..." << std::endl;
            break;
          }

          // Break without using first value in child array
          case 8: {
            ASSERT_EQUAL(key, "array_break_unused");
            for (auto child_value : field.value()) {
              ASSERT_SUCCESS(child_value);
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child array ..." << std::endl;
            break;
          }

          // Break out of multiple child loops
          case 9: {
            ASSERT_EQUAL(key, "quadruple_nested_break");
            for (auto child1 : field.value().get_object()) {
              for (auto child2 : child1.value().get_array()) {
                for (auto child3 : child2.get_object()) {
                  for (auto child4 : child3.value().get_array()) {
                    uint64_t x;
                    ASSERT_SUCCESS( child4.get(x) );
                    ASSERT_EQUAL( x, 9 );
                    break;
                  }
                  break;
                }
                break;
              }
              break;
            }
            std::cout << "  - After breaking out of quadruply-nested arrays and objects ..." << std::endl;
            break;
          }

          // Test the actual value
          case 10: {
            ASSERT_EQUAL(key, "actual_value");
            uint64_t actual_value;
            ASSERT_SUCCESS( field.value().get(actual_value) );
            ASSERT_EQUAL( actual_value, 10 );
            break;
          }
        }

        i++;
      }
      ASSERT_EQUAL( i, 11 ); // Make sure we found all the keys we expected
      return true;
    }));
    return true;
  }

  bool iterate_empty_object() {
    TEST_START();
    auto json = R"({})"_padded;

    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      for (simdjson_unused auto field : object) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      for (simdjson_unused auto field : object_result) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    TEST_SUCCEED();
  }

  bool value_search_unescaped_key() {
    TEST_START();
    auto json = R"({"k\u0065y": 1})"_padded;
    SUBTEST("ondemand::unescapedkey", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      bool got_key = false;
      ASSERT_SUCCESS( doc_result.get(object) );
      for (auto field : object) {
        std::string_view keyv;
        ASSERT_SUCCESS( field.unescaped_key().get(keyv) );
        if(keyv == "key") {
          int64_t value;
          ASSERT_SUCCESS( field.value().get(value) );
          ASSERT_EQUAL( value, 1);
          got_key = true;
        }
      }
      return got_key;
    }));
    SUBTEST("ondemand::rawkey", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      bool got_key = false;
      for (auto field : object) {
        ondemand::raw_json_string keyv;
        ASSERT_SUCCESS( field.key().get(keyv) );
        if(keyv == R"(k\u0065y)") {
          int64_t value;
          ASSERT_SUCCESS( field.value().get(value) );
          ASSERT_EQUAL( value, 1);
          got_key = true;
        }
      }
      return got_key;
    }));
    TEST_SUCCEED();
  }

  bool issue_1480() {
    TEST_START();
    auto json = R"({ "name"  : "something", "version": "0.13.2", "version_major": 0})"_padded;

    SUBTEST("ondemand::issue_1480::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      ASSERT_EQUAL( object.find_field_unordered("version_major").get_uint64().value_unsafe(), 0 );
      ASSERT_EQUAL( object.find_field_unordered("version").get_string().value_unsafe(),  "0.13.2");
      return true;
    }));
    SUBTEST("ondemand::issue_1480::big-key", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      std::vector<char> large_buf(512,' ');
      std::string_view lb{large_buf.data(), large_buf.size()};
      ASSERT_ERROR( object.find_field(lb), NO_SUCH_FIELD );
      return true;
    }));

#if SIMDJSON_EXCEPTIONS
    SUBTEST("ondemand::issue_1480::object-iteration", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t counter_version{0};
      size_t counter_version_major{0};
      for(auto field : object)  {
        std::string_view keyv = field.unescaped_key();
        if(keyv == "version") { counter_version++; }
        if(keyv == "version_major") { counter_version_major++; }
      }
      ASSERT_EQUAL( counter_version, 1 );
      ASSERT_EQUAL( counter_version_major, 1 );
      return true;
    }));
    SUBTEST("ondemand::issue_1480::object-iteration-string-view", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t counter_version{0};
      size_t counter_version_major{0};
      const char * bufv = "version";
      const char * bufvm = "version_major";

      std::string_view v{bufv};
      std::string_view vm{bufvm};

      for(auto field : object)  {
        std::string_view keyv = field.unescaped_key();
        if(keyv == v) { counter_version++; }
        if(keyv == vm) { counter_version_major++; }
      }
      ASSERT_EQUAL( counter_version, 1 );
      ASSERT_EQUAL( counter_version_major, 1 );
      return true;
    }));
    SUBTEST("ondemand::issue_1480::original", test_ondemand_doc(json, [&](auto doc_result) {
      size_t counter{0};
      for (auto field : doc_result.get_object()) {
        auto key = field.key().value();
        if (key == "version") { counter++; }
      }
      ASSERT_EQUAL( counter, 1 );
      return true;
    }));
    SUBTEST("ondemand::issue_1480::object-operator", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      ASSERT_EQUAL( std::string_view(object["version"]), "0.13.2" );
      ASSERT_EQUAL( uint64_t(object["version_major"]), 0 );
      return true;
    }));
#endif
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS

  bool iterate_object_exception() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    const uint64_t expected_value[] = { 1, 2, 3 };
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      size_t i = 0;
      for (ondemand::field field : doc_result.get_object()) {
        ASSERT_EQUAL( field.key(), expected_key[i] );
        ASSERT_EQUAL( uint64_t(field.value()), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_object_exception() {
    TEST_START();
    auto json = R"({})"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::field field : doc_result.get_object()) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    SUBTEST("ondemand::object-document-rewind", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      doc_result.rewind();
      i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      return true;
    }));
    SUBTEST("ondemand::object-rewind", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      object.reset();
      i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      return true;
    }));
    SUBTEST("ondemand::object-rewind", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_RESULT( doc_result.type(), json_type::object );
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      object.reset();
      i = 0;
      for (auto field : object) {
        (void)field;
        i++;
      }
      ASSERT_EQUAL( i, 0 );
      return true;
    }));

    TEST_SUCCEED();
  }

#endif // SIMDJSON_EXCEPTIONS

  bool empty(simdjson::ondemand::object obj) {
    bool is_empty;
    ASSERT_SUCCESS(obj.is_empty().get(is_empty));
    ASSERT_TRUE(is_empty);
    return true;
  }
  bool value_to_object(simdjson::ondemand::value val) {
    ondemand::json_type t;
    ASSERT_SUCCESS(val.type().get(t));
    ASSERT_EQUAL(t, ondemand::json_type::object);
    simdjson::ondemand::object obj;
    ASSERT_SUCCESS(val.get_object().get(obj));
    if(empty(obj) != true) { return false; }
    return true;
  }
  bool empty_rewind_convoluted() {
    TEST_START();
    const auto json = R"( {} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value val;
    ASSERT_SUCCESS(doc.get_value().get(val));
    if(!value_to_object(val)) { return false; }
    TEST_SUCCEED();
  }
#if SIMDJSON_EXCEPTIONS
  bool value_to_object_except(simdjson::ondemand::value val) {
    ondemand::json_type t = val.type();
    ASSERT_EQUAL(t, ondemand::json_type::object);
    if(empty(simdjson::ondemand::object(val)) != true) { return false; }
    return true;
  }
  bool empty_rewind_convoluted_with_exceptions() {
    TEST_START();
    const auto json = R"( {} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value val;
    ASSERT_SUCCESS(doc.get_value().get(val));
    if(value_to_object_except(val) != true) { return false; }
    TEST_SUCCEED();
  }
#endif

  bool iterate_empty_object_count() {
    TEST_START();
    auto json = R"( {} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    size_t count;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    ASSERT_SUCCESS(obj.count_fields().get(count));
    ASSERT_EQUAL(count, 0);
    TEST_SUCCEED();
  }

  bool issue1876a() {
    TEST_START();
    auto json = R"( {} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    size_t count;
    ASSERT_SUCCESS(obj.count_fields().get(count));
    ASSERT_EQUAL(count, 0);
    for (auto field : obj) {
      std::string_view key;
      ASSERT_SUCCESS(field.unescaped_key().get(key));
      double value;
      ASSERT_SUCCESS(field.value().get_double().get(value));
      std::cout << key << " " << value << std::endl;
    }
    TEST_SUCCEED();
  }

  bool issue1876() {
    TEST_START();
    auto json = R"( {} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    size_t count;
    ASSERT_SUCCESS(doc.count_fields().get(count));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));

    ASSERT_EQUAL(count, 0);
    for (auto field : obj) {
      std::string_view key;
      ASSERT_SUCCESS(field.unescaped_key().get(key));
      double value;
      ASSERT_SUCCESS(field.value().get_double().get(value));
      std::cout << key << " " << value << std::endl;
    }
    TEST_SUCCEED();
  }

  bool issue1742() {
    TEST_START();
    auto json = R"( {
  "code": 0,
  "method": "subscribe",
  "result": {
    "instrument_name": "DAI_USDC",
    "subscription": "trade.DAI_USDC",
    "channel": "trade",
    "data": [
      {
        "dataTime": 1635315736764,
        "d": 1928673075051012900,
        "s": "SELL",
        "p": 1.0004,
        "q": 0.01,
        "t": 1635315736763,
        "i": "DAI_USDC"
      }
    ]
  }
} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    simdjson::ondemand::array data;
    ASSERT_SUCCESS(doc["result"]["data"].get_array().get(data));
    for (auto d : data) {
      simdjson::ondemand::object obj;
      ASSERT_SUCCESS(d.get_object().get(obj));
      size_t count;
      ASSERT_SUCCESS(obj.count_fields().get(count));
      ASSERT_EQUAL(count, 7);
    }
    TEST_SUCCEED();
  }


  bool issue1742_value() {
    TEST_START();
    auto json = R"( {
  "code": 0,
  "method": "subscribe",
  "result": {
    "instrument_name": "DAI_USDC",
    "subscription": "trade.DAI_USDC",
    "channel": "trade",
    "data": [
      {
        "dataTime": 1635315736764,
        "d": 1928673075051012900,
        "s": "SELL",
        "p": 1.0004,
        "q": 0.01,
        "t": 1635315736763,
        "i": "DAI_USDC"
      }
    ]
  }
} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    simdjson::ondemand::array data;
    ASSERT_SUCCESS(doc["result"]["data"].get_array().get(data));
    for (auto d : data) {
      size_t count;
      ASSERT_SUCCESS(d.count_fields().get(count));
      ASSERT_EQUAL(count, 7);
    }
    TEST_SUCCEED();
  }

  bool iterate_basic_object_count() {
    TEST_START();
    auto json = R"( {"a":-55, "b":3.23, "c":100000000000000000000, "d":true, "e":null} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    size_t count;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    ASSERT_SUCCESS(obj.count_fields().get(count));
    ASSERT_EQUAL(count, 5);
    TEST_SUCCEED();
  }

  bool iterate_complex_object_count() {
    TEST_START();
    auto json = R"( {
                      "first": {"a":1, "b":[1,2,3], "c":{}},
                      "second":[true, false, null, {"1":[4,5,6], "2":3.14}]
                    } )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj1;
    size_t count{0};
    ASSERT_SUCCESS(doc.get_object().get(obj1));
    ASSERT_SUCCESS(obj1.count_fields().get(count));
    ASSERT_EQUAL(count, 2);
    count = 0;
    ondemand::object obj2;
    ASSERT_SUCCESS(doc.find_field("first").get_object().get(obj2));
    ASSERT_SUCCESS(obj2.count_fields().get(count));
    ASSERT_EQUAL(count, 3);
    count = 0;
    ondemand::object obj3;
    ASSERT_SUCCESS(obj2.find_field("c").get_object().get(obj3));
    ASSERT_SUCCESS(obj3.count_fields().get(count));
    ASSERT_EQUAL(count, 0);
    count = 0;
    ondemand::object obj4;
    ASSERT_SUCCESS(doc.at_pointer("/second/3").get_object().get(obj4));
    ASSERT_SUCCESS(obj4.count_fields().get(count));
    ASSERT_EQUAL(count, 2);
    TEST_SUCCEED();
  }

  bool iterate_bad_object_count() {
    TEST_START();
    padded_string bad_jsons[3] = {R"( {"a":5 "b":3} )"_padded, R"( {"a":5, 3} )"_padded, R"( {"a":5, "b": } )"_padded};
    std::string names[3] = {"missing_comma", "missing_key", "missing_value"};
    size_t count{0};

    for (auto name : names) {
      SUBTEST("ondemand::" + name, test_ondemand_doc(bad_jsons[count++], [&](auto doc_result) {
        ondemand::object object;
        ASSERT_RESULT( doc_result.type(), json_type::object );
        ASSERT_SUCCESS( doc_result.get(object) );
        ASSERT_ERROR(object.count_fields(), TAPE_ERROR);
        return true;
      }));
    }
    ASSERT_EQUAL(count, 3);
    TEST_SUCCEED();
  }

  bool iterate_doc_object_count() {
    TEST_START();
    auto empty = R"( {} )"_padded;
    SUBTEST("ondemand::empty_doc_object", test_ondemand_doc(empty, [&](auto doc_result) {
        size_t count;
        ASSERT_RESULT( doc_result.type(), json_type::object );
        ASSERT_SUCCESS( doc_result.count_fields().get(count) );
        ASSERT_EQUAL( count, 0 );
        return true;
    }));
    auto basic = R"( {"a":-1.234, "b":false, "c":null, "d":[1000.1,-2000.2,3000.3], "e":{"a":true, "b":false}} )"_padded;
    SUBTEST("ondemand::basic_doc_object", test_ondemand_doc(basic, [&](auto doc_result) {
        size_t count;
        ASSERT_RESULT( doc_result.type(), json_type::object );
        ASSERT_SUCCESS( doc_result.count_fields().get(count) );
        ASSERT_EQUAL( count, 5 );
        return true;
    }));
    TEST_SUCCEED();
  }


  bool issue1974a() {
    TEST_START();
    padded_string bad_json = R"({"key":111)"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(bad_json).get(doc));
    ondemand::object object;
    ASSERT_ERROR(doc.get_object().get(object), INCOMPLETE_ARRAY_OR_OBJECT);
    TEST_SUCCEED();
  }

  bool issue1974b() {
    TEST_START();
    padded_string bad_json = R"({"key":111)"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(bad_json).get(doc));
    ondemand::value val;
    ASSERT_ERROR(doc.get_value().get(val), INCOMPLETE_ARRAY_OR_OBJECT);
    TEST_SUCCEED();
  }

  bool iterate_bad_doc_object_count() {
    TEST_START();
    padded_string bad_jsons[4] = {R"( {"a":5 "b":3} )"_padded, R"( {"a":5, 3} )"_padded, R"( {"a":5, "b": } )"_padded, R"( {"a":5, "b":3 )"_padded};
    std::string names[4] = {"missing_comma", "missing_key", "missing_value", "missing_bracket"};
    simdjson::error_code errors[4] = {TAPE_ERROR, TAPE_ERROR, TAPE_ERROR, INCOMPLETE_ARRAY_OR_OBJECT};
    size_t count{0};

    for (auto name : names) {
      SUBTEST("ondemand::" + name, test_ondemand_doc(bad_jsons[count], [&](auto doc_result) {
        ASSERT_RESULT(doc_result.type(), json_type::object );
        ASSERT_ERROR(doc_result.count_fields(), errors[count]);
        return true;
      }));
      count++;
    }
    ASSERT_EQUAL(count, 4);
    TEST_SUCCEED();
  }


  bool test_strager() {
    // Find exactly one of a/myint or b/myint, if both or none are present,
    // return false.
    auto process = [](ondemand::object root) -> std::pair<bool,uint64_t> {
      ///////////////////////////////////////////////////////////
      // The core idea with On-Demand is that you should visit everything only
      // once.
      //
      // Thus, with On-Demand, you must consume a field once you have accessed it.
      // Thus if you do root["a"], you must full consume the corresponding
      // value (if present) before you move to another field (e.g., root["a"]).
      //
      // If, somehow, you only want the value corresponding to "a" if some
      // other key is not present, then you must still parse/consume the value
      // corresponding to "a", and later throw the result away.
      ///////////////////////////////////////////////////////////
      uint64_t myint;
      // Notice in the line below that I use chaining... I do not
      // first convert root["a"] to an object and then seek
      // "myint" into. Chaining works in this instance because
      // I only need to access the object 'root["a"]' for a single
      // key, if I needed to do more work on root["a"], then I should
      // create an object.
      if(root["a"]["myint"].get(myint) == SUCCESS) {
        // We want myint to be discarded if "b" is also present.
        if(! root["b"].error() ) {
          // Oops, the other key is also present!!! We must throw
          // away 'myint' and return false.
          // Notice how I do not parse the value corresponding to
          // '"b"', as it is unneeded.
          return std::make_pair(false, 0);
        }
        // All is good,
        return std::make_pair(true,myint);
      }
      // Now we know that "a" is not present, so seek "b".
      if(root["b"]["myint"].get(myint) == SUCCESS) {
        return std::make_pair(true, myint);
      }
      // None are present.
      return std::make_pair(false, 0);
    };

    TEST_START();

    {
      ondemand::parser parser;
      padded_string json = R"({ "a": { "myint": 42 } })"_padded;
      std::cout << " - Subtest 'a' - JSON: " << (json) << " ..." << std::endl;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      std::pair<bool,uint64_t> ret = process(obj);
      ASSERT_TRUE(ret.first);
      ASSERT_EQUAL(ret.second, 42);
    }

    {
      ondemand::parser parser;
      padded_string json = R"({ "b": { "myint": 42 } })"_padded;
      std::cout << " - Subtest 'b' - JSON: " << (json) << " ..." << std::endl;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      std::pair<bool,uint64_t> ret = process(obj);
      ASSERT_TRUE(ret.first);
      ASSERT_EQUAL(ret.second, 42);
    }


    {
      ondemand::parser parser;
      padded_string json = R"({ "b": { "myint": 42 }, "a": { "myint": 42 } })"_padded;
      std::cout << " - Subtest 'b&a' - JSON: " << (json) << " ..." << std::endl;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      std::pair<bool,uint64_t> ret = process(obj);
      ASSERT_FALSE(ret.first);
      ASSERT_EQUAL(ret.second, 0);
    }

    {
      ondemand::parser parser;
      padded_string json = R"({ "z": { "myint": 42 }, "y": { "myint": 42 } })"_padded;
      std::cout << " - Subtest 'none' - JSON: " << (json) << " ..." << std::endl;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      std::pair<bool,uint64_t> ret = process(obj);
      ASSERT_FALSE(ret.first);
      ASSERT_EQUAL(ret.second, 0);
    }

    TEST_SUCCEED();
  }

  bool run() {
    return issue1979() &&
           issue1977() &&
#if SIMDJSON_EXCEPTIONS
           issue1965() &&
#endif
           issue1974a() &&
           issue1974b() &&
           issue1876a() &&
           issue1876() &&
           test_strager() &&
           issue1745() &&
           issue1742() &&
           issue1742_value() &&
           issue1723() &&
           value_search_unescaped_key() &&
           missing_key_continue() &&
           no_missing_keys() &&
           missing_keys() &&
           missing_keys_for_empty_top_level_object() &&
#if SIMDJSON_EXCEPTIONS
           issue1745_with_exceptions() &&
           issue1723_except() &&
           fixed_broken_issue_1521() &&
           issue_1521() &&
           broken_issue_1521() &&
#endif
           iterate_object() &&
           iterate_empty_object() &&
           iterate_object_partial_children() &&
           issue_1480() &&
           empty_rewind_convoluted() &&
#if SIMDJSON_EXCEPTIONS
           iterate_object_exception() &&
           empty_rewind_convoluted_with_exceptions() &&
#endif // SIMDJSON_EXCEPTIONS
           iterate_empty_object_count() &&
           iterate_basic_object_count() &&
           iterate_complex_object_count() &&
           iterate_bad_object_count() &&
           iterate_bad_doc_object_count() &&
           true;
  }

} // namespace object_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, object_tests::run);
}
