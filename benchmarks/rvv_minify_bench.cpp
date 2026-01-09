// C:\MyCode\Lemire\simdjson\benchmarks\rvv_minify_bench.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Benchmark: Minify
// -----------------------------------------------------------------------------
//
// Purpose
// - Measure minify throughput for the RVV backend (whitespace removal).
// - Exercise tail handling and different whitespace densities.
// - Designed to run on real RVV hardware or under QEMU user-mode.
//
// Notes
// - Uses the public simdjson::minify API, which dispatches to the active backend.
// - Attempts to activate the RVV backend programmatically; also supports the
//   environment variable SIMDJSON_FORCE_IMPLEMENTATION=rvv.
// - Outputs MB/s for each test case / input size.
//
// Build / Run (QEMU example)
//   export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//   export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//   qemu-riscv64 ./build/rvv-clang-rv64gcv/benchmark/rvv_minify_bench
//
// -----------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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
  std::string input;  // raw JSON-ish bytes (minify doesn't validate JSON)
};

// Generates a JSON object with controllable whitespace density.
// density in [0, 1]: 0 -> minified already, 1 -> lots of whitespace.
static std::string make_whitespace_heavy_json(size_t target_bytes, double density, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> ws_pick(0, 3);
  auto ws_char = [&]() -> char {
    switch (ws_pick(rng)) {
      case 0: return ' ';
      case 1: return '\t';
      case 2: return '\n';
      default: return '\r';
    }
  };

  auto maybe_ws = [&](std::string &s, size_t max_n) {
    if (density <= 0.0) return;
    // Insert 0..max_n whitespace chars with probability density.
    if ((rng() % 10000) < uint32_t(density * 10000.0)) {
      size_t n = size_t(rng() % (max_n + 1));
      for (size_t i = 0; i < n; i++) s.push_back(ws_char());
    }
  };

  std::string out;
  out.reserve(target_bytes + 128);

  out.push_back('{');
  maybe_ws(out, 3);

  size_t i = 0;
  bool first = true;
  while (out.size() + 64 < target_bytes) {
    if (!first) {
      out.push_back(',');
      maybe_ws(out, 4);
    }
    first = false;

    // "k123": [ 1, 2, 3, {"x": 1} ]
    out.push_back('"');
    out.push_back('k');
    out += std::to_string(i++);
    out.push_back('"');
    maybe_ws(out, 2);
    out.push_back(':');
    maybe_ws(out, 5);

    out.push_back('[');
    maybe_ws(out, 3);
    out += "1";
    maybe_ws(out, 2);
    out.push_back(',');
    maybe_ws(out, 4);
    out += "2";
    maybe_ws(out, 2);
    out.push_back(',');
    maybe_ws(out, 4);
    out += "3";
    maybe_ws(out, 2);
    out.push_back(',');
    maybe_ws(out, 4);

    out.push_back('{');
    maybe_ws(out, 2);
    out += R"("x")";
    maybe_ws(out, 1);
    out.push_back(':');
    maybe_ws(out, 3);
    out += "1";
    maybe_ws(out, 1);
    out.push_back('}');
    maybe_ws(out, 2);

    out.push_back(']');
    maybe_ws(out, 4);
  }

  maybe_ws(out, 3);
  out.push_back('}');
  return out;
}

// Generates already-minified JSON (density ~ 0) but still large.
static std::string make_minified_json(size_t target_bytes, uint32_t seed) {
  return make_whitespace_heavy_json(target_bytes, /*density=*/0.0, seed);
}

// Generates "mostly whitespace" content with occasional tokens.
// Not necessarily valid JSON, but minify is specified to only remove the 4 ws bytes.
static std::string make_mostly_whitespace(size_t target_bytes, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> ws_pick(0, 3);
  auto ws_char = [&]() -> char {
    switch (ws_pick(rng)) {
      case 0: return ' ';
      case 1: return '\t';
      case 2: return '\n';
      default: return '\r';
    }
  };

  std::string out;
  out.reserve(target_bytes);

  // Pattern: whitespace runs punctuated by 'X', '{', '}', '"', digits...
  const char tokens[] = {'X', '{', '}', '[', ']', ':', ',', '"', '0', '1', '2', '3'};
  std::uniform_int_distribution<int> tok_pick(0, int(sizeof(tokens) - 1));

  while (out.size() < target_bytes) {
    size_t ws_len = 1 + (rng() % 64);
    for (size_t i = 0; i < ws_len && out.size() < target_bytes; i++) out.push_back(ws_char());
    if (out.size() < target_bytes) out.push_back(tokens[tok_pick(rng)]);
  }

  return out;
}

static size_t pick_iters_for_size(size_t bytes) {
  // Keep total work reasonable under QEMU.
  if (bytes <= 4 * 1024) return 15000;
  if (bytes <= 64 * 1024) return 2500;
  if (bytes <= 1024 * 1024) return 250;
  return 60;
}

static void run_minify_bench(const BenchCase &bc, size_t iters, size_t warmup_iters) {
  const size_t n = bc.input.size();

  // Destination buffer upper bound is input size (minify only removes chars).
  std::vector<uint8_t> dst(n);
  size_t dst_len = 0;

  // Warmup
  for (size_t i = 0; i < warmup_iters; i++) {
    dst_len = dst.size();
    auto err = simdjson::minify(reinterpret_cast<const uint8_t *>(bc.input.data()), n, dst.data(), dst_len);
    if (err) {
      std::cerr << "Warmup minify error in case '" << bc.name << "': " << err << "\n";
      std::exit(1);
    }
    clobber_memory();
  }

  auto t0 = clock_type::now();
  for (size_t i = 0; i < iters; i++) {
    dst_len = dst.size();
    auto err = simdjson::minify(reinterpret_cast<const uint8_t *>(bc.input.data()), n, dst.data(), dst_len);
    if (err) {
      std::cerr << "Minify error in case '" << bc.name << "': " << err << "\n";
      std::exit(1);
    }
    // Use dst_len to prevent optimization.
    if (dst_len == 0 && n != 0) clobber_memory();
  }
  auto t1 = clock_type::now();

  const double sec = seconds_since(t0, t1);
  const double total_bytes = double(n) * double(iters);
  const double mb = total_bytes / (1024.0 * 1024.0);
  const double mbps = (sec > 0) ? (mb / sec) : 0.0;

  std::cout << "  " << bc.name
            << " | size=" << n
            << " | iters=" << iters
            << " | " << mbps << " MB/s"
            << " | last_out=" << dst_len << "\n";
}

} // namespace

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  std::cout << "==================================================\n";
  std::cout << "   RVV Minify Benchmark\n";
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
    std::cout << "\n=== Input size target: " << sz << " bytes ===\n";

    std::vector<BenchCase> cases;
    cases.push_back({"minified_json", make_minified_json(sz, 1001)});
    cases.push_back({"whitespace_medium", make_whitespace_heavy_json(sz, 0.35, 2002)});
    cases.push_back({"whitespace_heavy", make_whitespace_heavy_json(sz, 0.75, 3003)});
    cases.push_back({"mostly_whitespace", make_mostly_whitespace(sz, 4004)});

    for (const auto &bc : cases) {
      const size_t iters = pick_iters_for_size(bc.input.size());
      run_minify_bench(bc, iters, std::min<size_t>(20, iters / 10));
    }
  }

  std::cout << "\nDone.\n";
  return 0;
}
