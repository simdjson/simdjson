// C:\MyCode\Lemire\simdjson\benchmarks\rvv_utf8_bench.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Benchmark: UTF-8 Validation
// -----------------------------------------------------------------------------
//
// Purpose
// - Measure validate_utf8 throughput for the RVV backend.
// - Exercise boundary/tail handling and different byte distributions.
// - Designed to run on real RVV hardware or under QEMU user-mode.
//
// Notes
// - Uses the public simdjson::validate_utf8 API, which dispatches to the active backend.
// - Attempts to activate RVV programmatically; also supports:
//     export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//
// Build / Run (QEMU example)
//   export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//   export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//   qemu-riscv64 ./build/rvv-clang-rv64gcv/benchmark/rvv_utf8_bench
//
// -----------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "simdjson.h"

namespace {

using clock_type = std::chrono::steady_clock;

static inline void clobber_memory() {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : : : "memory");
#endif
}

static bool force_rvv_backend() {
  for (auto impl : simdjson::get_available_implementations()) {
    if (impl && impl->name() == "rvv") {
      simdjson::get_active_implementation() = impl;
      return true;
    }
  }
  return false;
}

static double seconds_since(clock_type::time_point start, clock_type::time_point end) {
  using dsec = std::chrono::duration<double>;
  return std::chrono::duration_cast<dsec>(end - start).count();
}

struct BenchCase {
  std::string name;
  std::string data;
  bool expect_valid; // Used to sanity-check; benchmark focuses on speed.
};

// ---------------------------- Data generators --------------------------------

// ASCII-only valid UTF-8
static std::string make_ascii(size_t n, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> pick(32, 126);
  std::string out;
  out.resize(n);
  for (size_t i = 0; i < n; i++) out[i] = char(pick(rng));
  return out;
}

// Mix of valid UTF-8 sequences (ASCII + 2/3/4-byte), no invalid bytes.
static std::string make_valid_mixed_utf8(size_t n, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> which(0, 99);

  std::string out;
  out.reserve(n);

  while (out.size() < n) {
    int r = which(rng);
    if (r < 70) {
      // ASCII
      char c = char(32 + (rng() % 95));
      out.push_back(c);
    } else if (r < 85) {
      // 2-byte: U+00A0..U+07FF
      // Encode a codepoint in that range.
      uint32_t cp = 0x00A0u + (rng() % (0x07FFu - 0x00A0u + 1u));
      uint8_t b0 = uint8_t(0xC0u | (cp >> 6));
      uint8_t b1 = uint8_t(0x80u | (cp & 0x3Fu));
      if (out.size() + 2 <= n) {
        out.push_back(char(b0));
        out.push_back(char(b1));
      } else {
        // Fill remaining with ASCII.
        out.push_back('a');
      }
    } else if (r < 95) {
      // 3-byte: U+0800..U+D7FF and U+E000..U+FFFF (exclude surrogates)
      uint32_t cp;
      if (rng() & 1) {
        cp = 0x0800u + (rng() % (0xD7FFu - 0x0800u + 1u));
      } else {
        cp = 0xE000u + (rng() % (0xFFFFu - 0xE000u + 1u));
      }
      uint8_t b0 = uint8_t(0xE0u | (cp >> 12));
      uint8_t b1 = uint8_t(0x80u | ((cp >> 6) & 0x3Fu));
      uint8_t b2 = uint8_t(0x80u | (cp & 0x3Fu));
      if (out.size() + 3 <= n) {
        out.push_back(char(b0));
        out.push_back(char(b1));
        out.push_back(char(b2));
      } else {
        out.push_back('b');
      }
    } else {
      // 4-byte: U+10000..U+10FFFF
      uint32_t cp = 0x10000u + (rng() % (0x10FFFFu - 0x10000u + 1u));
      uint8_t b0 = uint8_t(0xF0u | (cp >> 18));
      uint8_t b1 = uint8_t(0x80u | ((cp >> 12) & 0x3Fu));
      uint8_t b2 = uint8_t(0x80u | ((cp >> 6) & 0x3Fu));
      uint8_t b3 = uint8_t(0x80u | (cp & 0x3Fu));
      if (out.size() + 4 <= n) {
        out.push_back(char(b0));
        out.push_back(char(b1));
        out.push_back(char(b2));
        out.push_back(char(b3));
      } else {
        out.push_back('c');
      }
    }
  }

  return out;
}

// Random bytes, often invalid UTF-8.
static std::string make_random_bytes(size_t n, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> pick(0, 255);
  std::string out;
  out.resize(n);
  for (size_t i = 0; i < n; i++) out[i] = char(pick(rng));
  return out;
}

// Mostly-valid UTF-8 with a controlled invalid byte injected near the end,
// to stress tail/boundary handling.
static std::string make_valid_with_tail_error(size_t n, uint32_t seed) {
  std::string out = make_valid_mixed_utf8(n, seed);
  if (!out.empty()) {
    // 0xFF is never valid in UTF-8.
    out.back() = char(0xFF);
  }
  return out;
}

static size_t pick_iters_for_size(size_t bytes) {
  // Keep total work reasonable under QEMU.
  if (bytes <= 4 * 1024) return 25000;
  if (bytes <= 64 * 1024) return 4000;
  if (bytes <= 1024 * 1024) return 350;
  return 90;
}

static void run_utf8_bench(const BenchCase &bc, size_t iters, size_t warmup_iters) {
  const size_t n = bc.data.size();

  // Warmup
  bool warm = false;
  for (size_t i = 0; i < warmup_iters; i++) {
    warm ^= simdjson::validate_utf8(bc.data.data(), n);
    clobber_memory();
  }

  auto t0 = clock_type::now();
  bool acc = warm;
  for (size_t i = 0; i < iters; i++) {
    // Accumulate to prevent the compiler from optimizing away the call.
    acc ^= simdjson::validate_utf8(bc.data.data(), n);
  }
  auto t1 = clock_type::now();

  const double sec = seconds_since(t0, t1);
  const double total_bytes = double(n) * double(iters);
  const double mb = total_bytes / (1024.0 * 1024.0);
  const double mbps = (sec > 0) ? (mb / sec) : 0.0;

  // Sanity check (single run)
  bool once = simdjson::validate_utf8(bc.data.data(), n);
  if (once != bc.expect_valid) {
    std::cerr << "  [WARN] Case '" << bc.name << "' sanity mismatch: expected "
              << (bc.expect_valid ? "valid" : "invalid") << " got "
              << (once ? "valid" : "invalid") << "\n";
  }

  std::cout << "  " << bc.name
            << " | size=" << n
            << " | iters=" << iters
            << " | " << mbps << " MB/s"
            << " | last=" << (once ? "valid" : "invalid")
            << " | acc=" << int(acc) // keep it observable
            << "\n";
}

} // namespace

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  std::cout << "==================================================\n";
  std::cout << "   RVV UTF-8 Validation Benchmark\n";
  std::cout << "==================================================\n";

  if (!force_rvv_backend()) {
    std::cerr << "[WARN] Could not activate 'rvv' backend programmatically.\n";
    std::cerr << "       Ensure build includes RVV and run with:\n";
    std::cerr << "       export SIMDJSON_FORCE_IMPLEMENTATION=rvv\n";
  }

  const auto *impl = simdjson::get_active_implementation();
  std::cout << "Active Backend: " << (impl ? impl->name() : "(null)") << "\n";
  if (impl) std::cout << "Description:    " << impl->description() << "\n";
  std::cout << "--------------------------------------------------\n";

  const std::vector<size_t> sizes = {
    256, 1024, 4096, 64 * 1024, 256 * 1024, 1024 * 1024
  };

  for (size_t sz : sizes) {
    std::cout << "\n=== Buffer size: " << sz << " bytes ===\n";

    std::vector<BenchCase> cases;
    cases.push_back({"ascii_valid", make_ascii(sz, 1001), true});
    cases.push_back({"mixed_utf8_valid", make_valid_mixed_utf8(sz, 2002), true});
    cases.push_back({"mixed_utf8_tail_error", make_valid_with_tail_error(sz, 3003), false});
    cases.push_back({"random_bytes_mostly_invalid", make_random_bytes(sz, 4004), false});

    for (const auto &bc : cases) {
      const size_t iters = pick_iters_for_size(bc.data.size());
      run_utf8_bench(bc, iters, std::min<size_t>(25, iters / 10));
    }
  }

  std::cout << "\nDone.\n";
  return 0;
}
