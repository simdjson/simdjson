#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>

namespace stl_types {
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION

bool basic_general_madness() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<std::unique_ptr<float>> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }
  if (*codes[5] != 2.2f || *codes[7] != 6.6f || *codes[9] != 10.0f) {
    return false;
  }

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      basic_general_madness() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace stl_types

int main(int argc, char *argv[]) {
  return test_main(argc, argv, stl_types::run);
}
