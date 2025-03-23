#include "simdjson.h"
#include <iostream>
#include <vector>

int main(void) {
  simdjson::dom::parser parser;

  simdjson::padded_string json_string = R"(
  {
    "firstName": "John",
    "lastName" : "doe",
    "age"      : 26,
    "address"  : {
      "streetAddress": "naist street",
      "city"         : "Nara",
      "postalCode"   : "630-0192"
    },
    "phoneNumbers": [
      {
        "type"  : "iPhone",
        "numbers": [
          "0123-4567-8888",
          "0123-4567-8788",
          "0123-4567-8887"
        ]
      },
      {
        "type"  : "home",
        "numbers": [
          "0123-4567-8910",
          "0123-4267-8910",
          "0103-4567-8910"
        ]
      }
    ]
  })"_padded;

  // simdjson::dom::element tweets = parser.load("twitter.json");
  // std::cout << tweets["search_metadata"]["count"] << " results." <<
  // std::endl;

  simdjson::dom::element parsed_json = parser.parse(json_string);
  // std::cout << parsed_json["firstName"] << std::endl;

  // selects all the top level elements
  // auto result = parsed_json.at_path_new("$.*");

  // // selects all address properties - $.address.*
  // auto result = parsed_json.at_path_new("$.address.*");

  // selects all address properties - $["address"].*
  // auto result = parsed_json.at_path_new("$["address"].*");

  // selects nested properties - $.*.streetAddress
  auto result = parsed_json.at_path_new("$.*.streetAddress");

  std::vector<simdjson::dom::element> values = result.value();

  std::string string_result;

  string_result = "[";
  for (int i = 0; i < values.size(); ++i) {
    simdjson::internal::string_builder<> sb;
    sb.append(values[i]);
    string_result = string_result +=
        std::string(i == 0 ? "" : ",") + "\n\t" + std::string(sb.str());
  }
  string_result += "\n]";

  std::cout << string_result << std::endl;

  // trying to figure out how printing arrays are handled
  // auto result = parsed_json["phoneNumbers"];
  // std::cout << result << std::endl;
}
