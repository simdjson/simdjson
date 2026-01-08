// C:\MyCode\Lemire\simdjson\fuzz\rvv_fuzz_minify.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Fuzzer Harness: minify()
// -----------------------------------------------------------------------------
//
// Purpose
// - LibFuzzer-compatible harness for the minify API, with best-effort forcing of
//   the RVV backend (when available).
// - Checks basic safety properties:
//   * No crashes on arbitrary input.
//   * On SUCCESS: output contains no JSON whitespace characters (space, tab,
//     LF, CR) and output length <= input length.
//   * On SUCCESS: result matches a simple scalar reference filter that removes
//     exactly the 4 JSON whitespace bytes (0x20, 0x09, 0x0A, 0x0D).
//
// Notes
// - This does not require the input to be valid JSON; simdjson::minify() is a
//   whitespace stripper and should be robust on any byte buffer.
// - The harness is safe to compile on non-RVV hosts; it will run using whatever
//   active implementation is available if RVV isn't compiled in.
//
// Build hints (clang + libFuzzer):
//   clang++ -fsanitize=fuzzer,address,undefined -O1 -g \
//     -Iinclude -Isingleheader -DSIMDJSON_ENABLE_RV64=ON \
//     fuzz/rvv_fuzz_minify.cpp -o rvv_fuzz_minify
//
// Run (QEMU example):
//   export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//   export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//   qemu-riscv64 ./rvv_fuzz_minify -runs=200000
//
// -----------------------------------------------------------------------------

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>

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

static inline bool is_json_whitespace(uint8_t c) {
  return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D;
}

static inline bool size_ok(size_t len) {
  // Keep fuzzing fast / bounded. Adjust if you want.
  return len <= (1u << 20);
}

// Reference: remove exactly 4 JSON whitespace bytes.
static inline void reference_minify(const uint8_t* in, size_t len, std::vector<uint8_t>& out) {
  out.clear();
  out.reserve(len);
  for (size_t i = 0; i < len; i++) {
    uint8_t c = in[i];
    if (!is_json_whitespace(c)) out.push_back(c);
  }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (data == nullptr) return 0;
  if (!size_ok(size)) return 0;

  force_rvv_if_present();

  // Destination buffer upper bound is input length (minify only removes bytes).
  std::vector<uint8_t> dst(size ? size : 1);
  size_t dst_len = 0;

  simdjson::error_code err = simdjson::minify(data, size, dst.data(), dst_len);

  if (err == simdjson::SUCCESS) {
    // 1) Basic invariants
    if (dst_len > size) __builtin_trap();

    // 2) No JSON whitespace remains
    for (size_t i = 0; i < dst_len; i++) {
      if (is_json_whitespace(dst[i])) __builtin_trap();
    }

    // 3) Must match scalar reference
    std::vector<uint8_t> ref;
    reference_minify(data, size, ref);

    if (ref.size() != dst_len) __builtin_trap();
    if (!std::equal(ref.begin(), ref.end(), dst.begin())) __builtin_trap();
  } else {
    // On error, we still require: no out-of-bounds writes etc. (sanitizers help).
    // Some implementations may set dst_len; don't assert on it.
  }

  return 0;
}
