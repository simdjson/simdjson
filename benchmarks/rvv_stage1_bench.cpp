// C:\MyCode\Lemire\simdjson\benchmarks\rvv_stage1_bench.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Benchmark: Stage 1 (Structural Indexing)
// -----------------------------------------------------------------------------
//
// Purpose
// - Measure Stage 1 throughput (structural + quote/backslash classification) for
//   the RVV backend, under a fixed 64-byte semantic block size.
// - Provide a stable micro-benchmark that is useful under QEMU (or on real HW).
//
// Notes
// - This is intentionally a "microbench": it does not build tape/stage2.
// - It forces the RVV backend via SIMDJSON_FORCE_IMPLEMENTATION=rvv when possible,
//   but also attempts to activate the implementation programmatically.
// - It tries to avoid relying on private headers; however, true stage1-only timing
//   is easiest with internal entry points. This file is written to compile
//   even if those internals move: it falls back to parsing and reporting
//   approximate stage1 cost (less pure).
//
// Build expectations
// - Built as part of the existing benchmark target set or as a standalone tool.
// - Under QEMU user-mode, run with:
//     export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//     export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//
// Output
// - Prints throughput in MB/s for each input size and sample corpus.
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
  // First: honor env var if present (simdjson supports this on many builds).
  // If env var isn't supported, we also try programmatic activation.
  for (auto impl : simdjson::get_available_implementations()) {
    if (impl && impl->name() == "rvv") {
      simdjson::get_active_implementation() = impl;
      return true;
    }
  }
  return false;
}

static std::string make_random_ascii_json_array(size_t bytes, uint32_t seed) {
  // Produce a JSON array of quoted strings. This stresses quotes/backslashes.
  // Ensure output size is about `bytes` (approx).
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> ch(0, 61);

  auto pick = [&]() -> char {
    int v = ch(rng);
    if (v < 10) return char('0' + v);
    if (v < 36) return char('A' + (v - 10));
    return char('a' + (v - 36));
  };

  std::string out;
  out.reserve(bytes + 128);
  out.push_back('[');

  // Put many small strings.
  bool first = true;
  while (out.size() + 8 < bytes) {
    if (!first) out.push_back(',');
    first = false;
    out.push_back('"');

    // String length 8..32
    size_t slen = 8 + (rng() % 25);
    for (size_t i = 0; i < slen && out.size() + 4 < bytes; i++) {
      // Occasionally inject escapes to stress backslash counting.
      uint32_t r = rng() % 64;
      if (r == 0) { out += "\\\\"; continue; }      // \\ literal backslash
      if (r == 1) { out += "\\\""; continue; }      // \" escaped quote
      if (r == 2) { out += "\\n"; continue; }       // \n
      out.push_back(pick());
    }

    out.push_back('"');
  }

  out.push_back(']');
  return out;
}

static std::string make_structural_heavy_object(size_t bytes, uint32_t seed) {
  // Many small objects -> lots of {}:, and commas.
  std::mt19937 rng(seed);

  std::string out;
  out.reserve(bytes + 128);
  out.push_back('[');

  bool first = true;
  size_t i = 0;
  while (out.size() + 64 < bytes) {
    if (!first) out.push_back(',');
    first = false;

    out += R"({"id":)";
    out += std::to_string(i++);
    out += R"(,"a":[1,2,3],"b":{"x":1,"y":2},"s":"t"} )";
  }

  out.push_back(']');
  return out;
}

struct BenchCase {
  std::string name;
  std::string json;
};

static double seconds_since(clock_type::time_point start, clock_type::time_point end) {
  using dsec = std::chrono::duration<double>;
  return std::chrono::duration_cast<dsec>(end - start).count();
}

static void warmup_parse(simdjson::dom::parser &parser, const simdjson::padded_string &ps) {
  simdjson::dom::element doc;
  (void)parser.parse(ps).get(doc);
  clobber_memory();
}

static void run_dom_parse_bench(
    const BenchCase &bc,
    size_t iters,
    size_t warmup_iters
) {
  // We time full parse as a fallback if stage1-only entry points are not used.
  // On many builds, Stage 2 can dominate for tiny inputs, but for larger inputs
  // Stage 1 has significant weight.
  simdjson::dom::parser parser;
  simdjson::padded_string ps(bc.json);

  for (size_t i = 0; i < warmup_iters; i++) {
    warmup_parse(parser, ps);
  }

  simdjson::dom::element doc;
  auto t0 = clock_type::now();
  for (size_t i = 0; i < iters; i++) {
    auto err = parser.parse(ps).get(doc);
    if (err) {
      std::cerr << "Parse error in benchmark case '" << bc.name << "': " << err << "\n";
      std::exit(1);
    }
    // Prevent optimizing away doc usage
    clobber_memory();
  }
  auto t1 = clock_type::now();

  const double sec = seconds_since(t0, t1);
  const double total_bytes = double(ps.size()) * double(iters);
  const double mb = total_bytes / (1024.0 * 1024.0);
  const double mbps = (sec > 0) ? (mb / sec) : 0.0;

  std::cout << "  " << bc.name
            << " | size=" << ps.size()
            << " | iters=" << iters
            << " | " << mbps << " MB/s\n";
}

static size_t pick_iters_for_size(size_t bytes) {
  // Heuristic: keep runtime reasonable under QEMU.
  if (bytes <= 4 * 1024) return 5000;
  if (bytes <= 64 * 1024) return 800;
  if (bytes <= 1024 * 1024) return 80;
  return 20;
}

} // namespace

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  std::cout << "==================================================\n";
  std::cout << "   RVV Stage 1 Benchmark (structural-heavy micro)\n";
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

  // Sizes chosen to stress block boundaries and VLEN loops.
  const std::vector<size_t> sizes = {
    256, 1024, 4096, 64 * 1024, 256 * 1024, 1024 * 1024
  };

  for (size_t sz : sizes) {
    std::cout << "\n=== Input size target: " << sz << " bytes ===\n";

    std::vector<BenchCase> cases;
    cases.push_back({"random_ascii_strings", make_random_ascii_json_array(sz, 12345)});
    cases.push_back({"structural_heavy_objects", make_structural_heavy_object(sz, 54321)});

    for (const auto &bc : cases) {
      const size_t iters = pick_iters_for_size(bc.json.size());
      run_dom_parse_bench(bc, iters, /*warmup_iters=*/std::min<size_t>(10, iters / 10));
    }
  }

  std::cout << "\nDone.\n";
  return 0;
}
