#include <string_view>
#include "simdjson.h"

int main() {
  constexpr simdjson::constevalutil::fixed_string json( R"({"key1": "value1", "key2": 42, "key3": {"nestedKey": "nestedValue"}, "key4": [1, 2, 3]})" );
  constexpr auto parsed = simdjson::compile_time::parse_json<json>();
  (void)parsed; // Suppress unused variable warning
}