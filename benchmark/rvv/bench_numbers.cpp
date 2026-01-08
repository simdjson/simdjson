#include <benchmark/benchmark.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Benchmark: Number Parsing
// -------------------------------------------------------------------------
// Purpose:
// Measure the throughput of parsing arrays of numbers.
//
// Scenarios:
// 1. Integers: Tests the signed/unsigned 64-bit parsing logic.
// 2. Doubles: Tests the floating-point parsing (SWAR or Vectorized).
// -------------------------------------------------------------------------

class NumbersFixture : public benchmark::Fixture {
protected:
    simdjson::padded_string ints_json;
    simdjson::padded_string doubles_json;
    const simdjson::implementation* rvv_impl = nullptr;
    
    // We target a large enough buffer to ensure we are measuring steady-state
    // parsing speed, not just startup overhead.
    const size_t NUM_ELEMENTS = 100000; 

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

        // 2. Generate Integer Data
        // Format: [1234567890, 1234567890, ...]
        std::stringstream ss_int;
        ss_int << "[";
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            ss_int << "1234567890"; 
            if (i < NUM_ELEMENTS - 1) ss_int << ",";
        }
        ss_int << "]";
        ints_json = simdjson::padded_string(ss_int.str());

        // 3. Generate Double Data
        // Format: [1.234567890123E+10, ...] (Scientific notation stresses the parser)
        std::stringstream ss_dbl;
        ss_dbl << "[";
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            ss_dbl << std::setprecision(10) << 1.234567890123e10; 
            if (i < NUM_ELEMENTS - 1) ss_dbl << ",";
        }
        ss_dbl << "]";
        doubles_json = simdjson::padded_string(ss_dbl.str());
    }
};

// -------------------------------------------------------------------------
// Benchmark: Integers
// -------------------------------------------------------------------------
BENCHMARK_DEFINE_F(NumbersFixture, BM_Parse_Integers)(benchmark::State& state) {
    simdjson::get_active_implementation() = rvv_impl;
    simdjson::dom::parser parser;

    for (auto _ : state) {
        simdjson::dom::element doc;
        auto error = parser.parse(ints_json).get(doc);
        if (error) {
            state.SkipWithError("Parse failed on Integers");
            break;
        }
        // Touch the data to ensure lazy evaluation (if any) completes
        // (Though DOM parsing usually materializes numbers immediately in Tape)
        benchmark::DoNotOptimize(doc);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(ints_json.size()));
    state.SetLabel(rvv_impl->name());
}

// -------------------------------------------------------------------------
// Benchmark: Doubles
// -------------------------------------------------------------------------
BENCHMARK_DEFINE_F(NumbersFixture, BM_Parse_Doubles)(benchmark::State& state) {
    simdjson::get_active_implementation() = rvv_impl;
    simdjson::dom::parser parser;

    for (auto _ : state) {
        simdjson::dom::element doc;
        auto error = parser.parse(doubles_json).get(doc);
        if (error) {
            state.SkipWithError("Parse failed on Doubles");
            break;
        }
        benchmark::DoNotOptimize(doc);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(doubles_json.size()));
    state.SetLabel(rvv_impl->name());
}

// -------------------------------------------------------------------------
// Register
// -------------------------------------------------------------------------
BENCHMARK_REGISTER_F(NumbersFixture, BM_Parse_Integers)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

BENCHMARK_REGISTER_F(NumbersFixture, BM_Parse_Doubles)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

BENCHMARK_MAIN();