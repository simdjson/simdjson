#include "simdjson/jsonparser.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  if (Size < 1) {
    return 0;
  }

  auto begin = (const char *)Data;
  auto end = begin + Size;

  std::string str(begin, end);

  // taken from the example on README on the front page.
  simdjson::ParsedJson pj = simdjson::build_parsed_json(str); // do the parsing
  if (!pj.is_valid()) {
    // something went wrong
    // std::cout << pj.get_error_message() << std::endl;
  }

  return 0;
}
