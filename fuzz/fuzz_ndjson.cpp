#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "FuzzUtils.h"
#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data, Size);
  const auto batch_size = static_cast<size_t>(fd.getInt<0,1000>());
  const auto json = simdjson::padded_string{fd.remainder_as_stringview()};
  simdjson::dom::parser parser;
  simdjson::dom::document_stream docs;
  if(parser.parse_many(json,batch_size).get(docs)) { return 0; }
  size_t bool_count1 = 0;
  size_t total_count1 = 0;
  for (auto doc : docs) {
    total_count1++;
    bool_count1 += doc.is_bool();
  }
  // Restart, if we made it this far, the document *must* be accessible.
  if(parser.parse_many(json,batch_size).get(docs)) { return EXIT_FAILURE; }
  size_t bool_count2 = 0;
  size_t total_count2 = 0;
  for (auto doc : docs) {
    total_count2++;
    bool_count2 += doc.is_bool();
  }
  // They should agree!!!
  if((total_count2 != total_count1) || (bool_count2 != bool_count1)) { return EXIT_FAILURE; }
  return 0;
}
