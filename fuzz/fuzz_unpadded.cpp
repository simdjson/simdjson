#include "simdjson.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  // Keep the allocation exact so AddressSanitizer places its redzone directly
  // after the logical input. Allocate one byte for the empty-input case so the
  // parser never receives a null pointer.
  const size_t allocation_size = Size == 0 ? 1 : Size;
  std::unique_ptr<uint8_t[]> exact(new (std::nothrow) uint8_t[allocation_size]);
  if (!exact) { return 0; }
  if (Size != 0) { std::memcpy(exact.get(), Data, Size); }

  simdjson::padded_string padded(reinterpret_cast<const char *>(Data), Size);
  if (!padded.data()) { return 0; }

  const simdjson::implementation *saved = simdjson::get_active_implementation();
  for (const simdjson::implementation *implementation :
       simdjson::get_available_implementations()) {
    if (!implementation->supported_by_runtime_system()) { continue; }
    simdjson::get_active_implementation() = implementation;
    simdjson::dom::parser unpadded_parser;
    const simdjson::error_code unpadded_error =
      unpadded_parser.parse_unpadded(exact.get(), Size).error();
    simdjson::dom::parser padded_parser;
    const simdjson::error_code padded_error = padded_parser.parse(padded).error();
    if (unpadded_error != padded_error) { std::abort(); }
  }
  simdjson::get_active_implementation() = saved;
  return 0;
}
