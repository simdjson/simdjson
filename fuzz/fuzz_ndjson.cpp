#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "FuzzUtils.h"
#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data, Size);
  const auto batch_size=static_cast<size_t>(fd.getInt<0,1000>());
  const auto json=simdjson::padded_string{fd.remainder_as_stringview()};
  simdjson::dom::parser parser;
#if SIMDJSON_EXCEPTIONS
  try {
#endif
    simdjson::dom::document_stream docs;
    if(parser.parse_many(json,batch_size).get(docs)) {
      return 0;
    }

  size_t bool_count=0;
  for (auto doc : docs) {
    bool_count+=doc.is_bool();
  }
#if SIMDJSON_EXCEPTIONS
  } catch(...) {
  }
#endif
  return 0;
}
