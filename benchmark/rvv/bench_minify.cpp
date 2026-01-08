#include <benchmark/benchmark.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Benchmark: Minification (Whitespace Removal)
// -------------------------------------------------------------------------
// Purpose:
// Measure the throughput of whitespace removal.
//
// This is a critical stress test for:
// 1. The input vector load loop.
// 2. The classification kernel (identifying 0x20, 0x09, 0x0A, 0x0D).
// 3. The `vcompress` (or permute) logic used to pack non-whitespace bytes.
// -------------------------------------------------------------------------

class MinifyFixture : public benchmark::Fixture {
protected:
    std::string dense_json;
    std::string pretty_json;
    std::vector<char> buffer; // Destination buffer
    const simdjson::implementation* rvv_impl = nullptr;
    const size_t TARGET_SIZE = 1024 * 1024; // 1 MB

public:
    void SetUp(const benchmark::State& state) override {
        // 1. Locate RVV Backend
        for (auto impl : simdjson::get_available_implementations()) {
            if (impl->name() == "rvv") {
                rvv_impl = impl;
                break;
            }
        }
        if (!rvv_impl) {
            rvv_impl = simdjson::get_active_implementation();
            std::cerr << "[WARN] RVV backend not found. Using: " << rvv_impl->name() << std::endl;
        }

        // 2. Generate Data
        // A) Dense JSON (Little whitespace) - Tests copy overhead
        std::string dense_pattern = R"({"id":123,"data":[1,2,3,4,5],"valid":true},)";
        while (dense_json.size() < TARGET_SIZE) {
            dense_json += dense_pattern;
        }

        // B) Pretty JSON (Lots of whitespace) - Tests removal logic
        // This forces the "vcompress" / vector packing logic to work hard.
        std::string pretty_pattern = R"(
        {
            "id": 123,
            "data": [
                1,
                2,
                3,
                4,
                5
            ],
            "valid": true
        },
        )";
        while (pretty_json.size() < TARGET_SIZE) {
            pretty_json += pretty_pattern;
        }

        // 3. Prepare Output Buffer (Max size = Input size)
        buffer.resize(std::max(dense_json.size(), pretty_json.size()));
    }
};

// -------------------------------------------------------------------------
// Benchmark: Dense Input (Mostly Copy)
// -------------------------------------------------------------------------
BENCHMARK_DEFINE_F(MinifyFixture, BM_Minify_Dense)(benchmark::State& state) {
    simdjson::get_active_implementation() = rvv_impl;

    size_t out_len = 0;
    for (auto _ : state) {
        auto err = simdjson::minify(dense_json.data(), dense_json.size(), buffer.data(), out_len);
        if (err) {
            state.SkipWithError("Minify failed on dense data");
            break;
        }
        benchmark::DoNotOptimize(out_len);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(dense_json.size()));
    state.SetLabel(rvv_impl->name());
}

// -------------------------------------------------------------------------
// Benchmark: Pretty Input (Heavy Compression)
// -------------------------------------------------------------------------
BENCHMARK_DEFINE_F(MinifyFixture, BM_Minify_Pretty)(benchmark::State& state) {
    simdjson::get_active_implementation() = rvv_impl;

    size_t out_len = 0;
    for (auto _ : state) {
        auto err = simdjson::minify(pretty_json.data(), pretty_json.size(), buffer.data(), out_len);
        if (err) {
            state.SkipWithError("Minify failed on pretty data");
            break;
        }
        benchmark::DoNotOptimize(out_len);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(pretty_json.size()));
    state.SetLabel(rvv_impl->name());
}

// -------------------------------------------------------------------------
// Register
// -------------------------------------------------------------------------
BENCHMARK_REGISTER_F(MinifyFixture, BM_Minify_Dense)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

BENCHMARK_REGISTER_F(MinifyFixture, BM_Minify_Pretty)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

BENCHMARK_MAIN();
