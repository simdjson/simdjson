#include "simdjson/portability.h"


#ifdef IS_X86_64

#include "simdjson/stage1_find_marks_haswell.h"
#include "simdjson/stage1_find_marks_westmere.h"
TARGET_HASWELL
namespace simdjson {
template<>
int find_structural_bits<architecture::haswell>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::haswell, buf, len, pj);
}

}
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
template<>
int find_structural_bits<architecture::westmere>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::westmere, buf, len, pj);
}

}
UNTARGET_REGION

#endif


#ifdef IS_ARM64
#include "simdjson/stage1_find_marks_arm64.h"
namespace simdjson {

template<>
inline int find_structural_bits<architecture::arm64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::arm64, buf, len, pj);
}


}
#endif
