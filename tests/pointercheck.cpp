#include <iostream>

#include "simdjson/jsonparser.h"
#include "simdjson/parsedjson.h"

int main() {
  // {"/~01abc": [0, {"\\\" 0": ["value0", "value1"]}]}"
  std::string json =
      "{\"/~01abc\": [0, {\"\\\\\\\" 0\": [\"value0\", \"value1\"]}]}";
  simdjson::ParsedJson pj;
  assert(pj.allocate_capacity(json.length()));
  simdjson::json_parse(json.c_str(), json.length(), pj);
  assert(pj.is_valid());
  simdjson::ParsedJson::Iterator it(pj);

  // valid JSON String Representation pointer
  std::string pointer1("/~1~001abc/1/\\\\\\\" 0/0");
  assert(it.move_to(pointer1.c_str(), pointer1.length()));
  assert(it.is_string());
  assert(it.get_string() == std::string("value0"));

  // valid URI Fragment Identifier Representation pointer
  std::string pointer2("#/~1~001abc/1/%x5C%x22%x200/1");
  assert(it.move_to(pointer2.c_str(), pointer2.length()));
  assert(it.is_string());
  assert(it.get_string() == std::string("value1"));

  // invalid pointer with leading 0 in index
  std::string pointer3("#/~1~001abc/01");
  assert(!it.move_to(pointer3.c_str(), pointer3.length())); // failed
  assert(it.is_string());                           // has probably not moved
  assert(it.get_string() == std::string("value1")); // has not move

  // "the (nonexistent) member after the last array element"
  std::string pointer4("/~1~001abc/-");
  assert(it.move_to(pointer4.c_str(), pointer4.length()));
  assert(it.get_type() == ']');
}
