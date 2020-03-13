#include <iostream>

#include "simdjson.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                       \
{    if (!(x)) {                                                        \
        char buf[4096];                                                 \
        snprintf (buf, 4096, "Failure in \"%s\", line %d\n",            \
                 __FILE__, __LINE__);                                   \
        abort ();                                                       \
    }                                                                   \
}
#endif

int main() {
  // {"/~01abc": [0, {"\\\" 0": ["value0", "value1"]}]}"
  std::string json =
      "{\"/~01abc\": [0, {\"\\\\\\\" 0\": [\"value0\", \"value1\"]}]}";
  simdjson::ParsedJson pj;
  simdjson::json_parse(json.c_str(), json.length(), pj);
  ASSERT(pj.is_valid());
  simdjson::ParsedJson::Iterator it(pj);

  // valid JSON String Representation pointer
  std::string pointer1("/~1~001abc/1/\\\\\\\" 0/0");
  ASSERT(it.move_to(pointer1.c_str(), pointer1.length()));
  ASSERT(it.is_string());
  ASSERT(it.get_string() == std::string("value0"));

  // valid URI Fragment Identifier Representation pointer
  std::string pointer2("#/~1~001abc/1/%x5C%x22%x200/1");
  ASSERT(it.move_to(pointer2.c_str(), pointer2.length()));
  ASSERT(it.is_string());
  ASSERT(it.get_string() == std::string("value1"));

  // invalid pointer with leading 0 in index
  std::string pointer3("#/~1~001abc/01");
  ASSERT(!it.move_to(pointer3.c_str(), pointer3.length())); // failed
  ASSERT(it.is_string());                           // has probably not moved
  ASSERT(it.get_string() == std::string("value1")); // has not move

  // "the (nonexistent) member after the last array element"
  std::string pointer4("/~1~001abc/-");
  ASSERT(it.move_to(pointer4.c_str(), pointer4.length()));
  ASSERT(it.get_type() == ']');
}
