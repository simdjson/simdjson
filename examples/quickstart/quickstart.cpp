#include <iostream>
#include "simdjson.h"

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
  std::cout << parsed_json["firstName"] << std::endl;

  std::cout << parsed_json.at_path_new("$.*")[0] << std::endl;
}
