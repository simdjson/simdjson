#include <benchmark/benchmark.h>
#include <iostream>
#include <vector>
#include <string>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Benchmark: Stage 1 (Structural Indexing)
// -------------------------------------------------------------------------
// Purpose:
// Measure the throughput of the raw structural indexing phase.
// This isolates the vector kernels (Whitespace + Op identification) from
// the Stage 2 DOM construction overhead.
// -------------------------------------------------------------------------

// We rely on the public interface where possible, but we specifically
// want to target the RVV implementation.

class Stage1Fixture : public benchmark::Fixture {
protected:
    simdjson::padded_string json;
    const simdjson::implementation* rvv_impl = nullptr;
    std::unique_ptr<uint32_t[]> structural_indexes;
    size_t capacity = 0;

public:
    void SetUp(const benchmark::State& state) override {
        // 1. Locate RVV Backend
        auto& impls = simdjson::get_available_implementations();
        for (auto impl : impls) {
            if (impl->name() == "rvv") {
                rvv_impl = impl;
                break;
            }
        }

        if (!rvv_impl) {
            // Fallback for non-RVV environments (e.g. standard x86 dev)
            // allows benchmark to compile/run even if it just tests fallback.
            rvv_impl = simdjson::get_active_implementation();
            std::cerr << "[WARN] RVV backend not found. Using: " << rvv_impl->name() << std::endl;
        }

        // 2. Load Corpus (Synthetic or File)
        // We use a repeatable synthetic large JSON for stable micro-benchmarking
        // to avoid file I/O noise.
        std::string repeated_obj = R"({"id": 1234567890, "name": "simdjson-rvv-bench", "active": true},)";
        std::string content = "[";
        // Generate ~1MB of JSON
        size_t target_size = 1024 * 1024;
        while (content.size() < target_size) {
            content += repeated_obj;
        }
        content.back() = ']'; // Replace last comma
        json = simdjson::padded_string(content);

        // 3. Pre-allocate Output Buffer
        // Structural indexes needs to be large enough (worst case: every byte is structural)
        // Round up to block size for safety
        capacity = json.size() + 64;
        structural_indexes.reset(new uint32_t[capacity]);
    }

    void TearDown(const benchmark::State& state) override {
        structural_indexes.reset();
    }
};

// -------------------------------------------------------------------------
// Benchmark Function
// -------------------------------------------------------------------------
BENCHMARK_DEFINE_F(Stage1Fixture, BM_Stage1)(benchmark::State& state) {
    if (!rvv_impl) {
        state.SkipWithError("No valid implementation found");
        return;
    }

    // Warmup (to populate instruction cache and branch predictors)
    // Note: create_dom normally calls stage1 internally, but we want to call
    // the internal Stage 1 if exposed.
    // Since simdjson's public API doesn't expose `stage1()` directly on the
    // implementation pointer easily without internal headers, we simulate the
    // load by parsing but ignoring the DOM, OR we rely on Minify which is
    // purely structural for many parts, OR we access the internal definition
    // via friend if this file was part of the library.

    // However, for this standalone benchmark, we will benchmark the *Minify* // function as a proxy for Stage 1 throughput, as Minify relies entirely
    // on the identification of whitespace and structural characters, which
    // uses the exact same vector kernels.

    // Alternative: Use the Parser which runs Stage 1 + Stage 2.
    // To isolate Stage 1, we can't easily do it without private headers.
    // Let's stick to a full parse for "Stage 1 + Stage 2" baseline
    // unless we include "src/simdjson.cpp".

    // BETTER APPROACH FOR EXPERT USER:
    // We can assume we are linking against the library where we can just run
    // a parse. But to make this specific to "Stage 1", let's benchmark
    // `simdjson::minify` which is predominantly the classification kernel.

    std::string minified_buffer;
    minified_buffer.resize(json.size());
    size_t len = 0;

    for (auto _ : state) {
        // Enforce the specific implementation for this run
        simdjson::get_active_implementation() = rvv_impl;

        // Execute Minify (Proxy for Stage 1 Kernel Speed)
        // This exercises: Load -> Vector Classification -> Store
        auto err = simdjson::minify(json.data(), json.size(), minified_buffer.data(), len);

        if (err) {
            state.SkipWithError("Minify failed");
            break;
        }

        // Prevent optimization
        benchmark::DoNotOptimize(len);
        benchmark::DoNotOptimize(minified_buffer.data());
    }

    // Metrics
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(json.size()));
    state.SetLabel(rvv_impl->name());
}

// -------------------------------------------------------------------------
// Register
// -------------------------------------------------------------------------
BENCHMARK_REGISTER_F(Stage1Fixture, BM_Stage1)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(1.0); // Run for at least 1 second

BENCHMARK_MAIN();
