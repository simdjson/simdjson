#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/stage1_find_marks_haswell.h"
#include "simdjson/stage1_find_marks_westmere.h"
TARGET_HASWELL
namespace simdjson {
template <>
int find_structural_bits<Architecture::HASWELL>(const uint8_t *buf, size_t len,
                                                ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::HASWELL, buf, len, pj,
                       simdjson::haswell::flatten_bits);
}
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len,
                                                 ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::WESTMERE, buf, len, pj,
                       simdjson::flatten_bits);
}
} // namespace simdjson
UNTARGET_REGION

#endif

#ifdef IS_ARM64
#include "simdjson/stage1_find_marks_arm64.h"
namespace simdjson {
template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len,
                                              ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::ARM64, buf, len, pj,
                       simdjson::flatten_bits);
}
} // namespace simdjson
#endif
