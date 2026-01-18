#include <benchmark/benchmark.h>
#include <iostream>
#include <string>
#include <vector>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Benchmark (Proxy): Stage 1 Kernels via minify()
// -------------------------------------------------------------------------
// NOTE:
// The public simdjson API does not expose a stable “stage1-only” entry point.
// This benchmark uses simdjson::minify() as a proxy because it exercises the
// same hot kernels we care about for Stage 1:
//   - 64B block loads
//   - whitespace classification
//   - mask export + packing/stores
//
// IMPORTANT:
// simdjson::implementation::minify() requires dst allocated up to
// len + SIMDJSON_PADDING bytes. (doc/API contract)
// -------------------------------------------------------------------------

class Stage1Fixture : public benchmark::Fixture {
protected:
  simdjson::padded_string json;
  std::vector<char> out;

  const simdjson::implementation *rvv_impl{nullptr};
  const simdjson::implementation *saved_impl{nullptr};

public:
  void SetUp(const benchmark::State &) override {
    // Save current active implementation (global) so we can restore it.
    saved_impl = simdjson::get_active_implementation();

    // Prefer the canonical lookup API.
    rvv_impl = simdjson::get_available_implementations()["rvv"];

    if (!rvv_impl) {
      rvv_impl = saved_impl;
      std::cerr << "[WARN] RVV implementation not found. Using: "
                << rvv_impl->name() << std::endl;
    } else {
      // Ensure it's safe to run on this machine (avoid illegal instruction).
      if (!rvv_impl->supported_by_runtime_system()) {
        std::cerr << "[WARN] RVV implementation present but not supported by runtime. Using: "
                  << saved_impl->name() << std::endl;
        rvv_impl = saved_impl;
      }
    }

    // Set active implementation once (avoid per-iteration global mutation).
    simdjson::get_active_implementation() = rvv_impl;

    // Generate ~1MB of JSON deterministically (no I/O noise).
    const std::string repeated_obj =
        R"({"id":1234567890,"name":"simdjson-rvv-bench","active":true},)";
    std::string content = "[";
    const size_t target_size = 1024 * 1024;

    while (content.size() < target_size) {
      content += repeated_obj;
    }

    // Replace last comma with closing bracket.
    if (content.size() >= 2) {
      content.back() = ']';
    } else {
      content = "[]";
    }

    json = simdjson::padded_string(content);

    // Output buffer must be len + SIMDJSON_PADDING bytes.
    out.resize(json.size() + SIMDJSON_PADDING);
  }

  void TearDown(const benchmark::State &) override {
    if (saved_impl) {
      simdjson::get_active_implementation() = saved_impl;
    }
  }
};

BENCHMARK_DEFINE_F(Stage1Fixture, BM_Stage1_Proxy_Minify)(benchmark::State &state) {
  if (!rvv_impl) {
    state.SkipWithError("No valid implementation found");
    return;
  }

  for (auto _ : state) {
    size_t out_len = 0;
    auto err = simdjson::minify(json.data(), json.size(), out.data(), out_len);
    if (err) {
      state.SkipWithError("minify() failed");
      break;
    }
    benchmark::DoNotOptimize(out_len);
    benchmark::DoNotOptimize(out.data());
  }

  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(json.size()));
  state.SetLabel(rvv_impl->name());
}

BENCHMARK_REGISTER_F(Stage1Fixture, BM_Stage1_Proxy_Minify)
  ->Unit(benchmark::kMicrosecond)
  ->MinTime(1.0);

BENCHMARK_MAIN();
