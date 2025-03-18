#include <iostream>
#include "simdjson.h"
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
        "number": "0123-4567-8888"
      },
      {
        "type"  : "home",
        "number": "0123-4567-8910"
      }
    ]
  })"_padded;


  // simdjson::dom::element tweets = parser.load("twitter.json");
  // std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

  simdjson::dom::element parsed_json = parser.parse(json_string);
  // std::cout << parsed_json["firstName"] << std::endl;

  auto result = parsed_json.at_path_new("$.*");
  std::vector<simdjson::dom::element> values = result.value();

  std::string string_result;

  string_result = "[";
  for (int i = 0; i < values.size(); ++i) {
    simdjson::internal::string_builder<> sb;
    sb.append(values[i]);
    string_result = string_result += sb.str();
    string_result += ",\n";
  }
  string_result += "]";

  std::cout << string_result << std::endl;

  // trying to figure out how printing arrays are handled
  // auto result = parsed_json["phoneNumbers"];
  // std::cout << result << std::endl;
}
