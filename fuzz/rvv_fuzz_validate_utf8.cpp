// C:\MyCode\Lemire\simdjson\fuzz\rvv_fuzz_validate_utf8.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Fuzzer Harness: validate_utf8()
// -----------------------------------------------------------------------------
//
// Purpose
// - LibFuzzer-compatible harness for simdjson::validate_utf8().
// - Best-effort forces the RVV backend when it is available.
// - Cross-checks result against a simple reference validator when possible.
//
// Notes
// - simdjson::validate_utf8() is defined for arbitrary byte buffers (not just
//   null-terminated strings).
// - The reference validator below is a small, strict UTF-8 checker (RFC 3629),
//   intended for fuzz cross-checking. It rejects:
//     * Overlong sequences
//     * Surrogates (U+D800..U+DFFF)
//     * Code points > U+10FFFF
//     * Invalid continuation bytes / truncated sequences
//
// Build hints (clang + libFuzzer):
//   clang++ -fsanitize=fuzzer,address,undefined -O1 -g \
//     -Iinclude -Isingleheader -DSIMDJSON_ENABLE_RV64=ON \
//     fuzz/rvv_fuzz_validate_utf8.cpp -o rvv_fuzz_validate_utf8
//
// Run (QEMU example):
//   export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//   export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//   qemu-riscv64 ./rvv_fuzz_validate_utf8 -runs=200000
//
// -----------------------------------------------------------------------------

#include <cstdint>
#include <cstddef>

#include "simdjson.h"

namespace {

static inline void force_rvv_if_present() {
  for (auto impl : simdjson::get_available_implementations()) {
    if (impl && impl->name() == "rvv") {
      simdjson::get_active_implementation() = impl;
      break;
    }
  }
}

static inline bool size_ok(size_t len) {
  // Bound runtime; adjust if desired.
  return len <= (1u << 20);
}

// Strict UTF-8 validator (RFC 3629).
// Returns true iff the byte buffer is valid UTF-8.
static bool ref_validate_utf8(const uint8_t *s, size_t len) {
  size_t i = 0;
  while (i < len) {
    uint8_t c = s[i];

    // ASCII
    if (c <= 0x7F) {
      i++;
      continue;
    }

    // 2-byte: C2..DF 80..BF
    if (c >= 0xC2 && c <= 0xDF) {
      if (i + 1 >= len) return false;
      uint8_t c1 = s[i + 1];
      if ((c1 & 0xC0) != 0x80) return false;
      i += 2;
      continue;
    }

    // 3-byte:
    // E0 A0..BF 80..BF   (no overlong)
    // E1..EC 80..BF 80..BF
    // ED 80..9F 80..BF   (no surrogates)
    // EE..EF 80..BF 80..BF
    if (c >= 0xE0 && c <= 0xEF) {
      if (i + 2 >= len) return false;
      uint8_t c1 = s[i + 1];
      uint8_t c2 = s[i + 2];
      if ((c1 & 0xC0) != 0x80) return false;
      if ((c2 & 0xC0) != 0x80) return false;

      if (c == 0xE0) {
        if (c1 < 0xA0) return false; // overlong
      } else if (c == 0xED) {
        if (c1 > 0x9F) return false; // surrogate range
      }
      i += 3;
      continue;
    }

    // 4-byte:
    // F0 90..BF 80..BF 80..BF  (no overlong)
    // F1..F3 80..BF 80..BF 80..BF
    // F4 80..8F 80..BF 80..BF  (<= U+10FFFF)
    if (c >= 0xF0 && c <= 0xF4) {
      if (i + 3 >= len) return false;
      uint8_t c1 = s[i + 1];
      uint8_t c2 = s[i + 2];
      uint8_t c3 = s[i + 3];
      if ((c1 & 0xC0) != 0x80) return false;
      if ((c2 & 0xC0) != 0x80) return false;
      if ((c3 & 0xC0) != 0x80) return false;

      if (c == 0xF0) {
        if (c1 < 0x90) return false; // overlong
      } else if (c == 0xF4) {
        if (c1 > 0x8F) return false; // > U+10FFFF
      }
      i += 4;
      continue;
    }

    // Reject everything else:
    // - C0, C1 (overlong 2-byte starters)
    // - F5..FF (invalid)
    // - 80..BF (unexpected continuation)
    return false;
  }
  return true;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (data == nullptr) return 0;
  if (!size_ok(size)) return 0;

  force_rvv_if_present();

  bool got = simdjson::validate_utf8(reinterpret_cast<const char *>(data), size);
  bool ref = ref_validate_utf8(data, size);

  if (got != ref) {
    __builtin_trap();
  }

  return 0;
}
