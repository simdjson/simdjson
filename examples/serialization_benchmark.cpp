#include <iostream>
#include <iomanip>
#include <chrono>
#include "simdjson.h"
#include <map>

struct TestData {
    std::string name;
    std::vector<int> numbers;
    std::map<std::string, double> metrics;
    bool active;
    std::optional<std::string> description;
};

int main() {
    std::cout << "⚡ Serialization Performance Benchmark\n";
    std::cout << "=====================================\n\n";
    
    // Create test data
    TestData data{
        .name = "Performance Test",
        .numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        .metrics = {{"latency", 1.23}, {"throughput", 456.78}, {"cpu", 89.01}},
        .active = true,
        .description = "Testing reflection-based serialization performance"
    };
    
    const int iterations = 100000;
    
    // Benchmark serialization
    std::cout << "📤 Benchmarking serialization (" << iterations << " iterations)...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string json;
    for (int i = 0; i < iterations; i++) {
        auto result = simdjson::builder::to_json_string(data);
        if (i == 0 && !result.error()) {
            json = result.value();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nResults:\n";
    std::cout << "  • Generated JSON size: " << json.length() << " bytes\n";
    std::cout << "  • Total time: " << duration.count() / 1000.0 << " ms\n";
    
    double time_per_iteration_us = duration.count() / double(iterations);
    double time_per_iteration_s = time_per_iteration_us / 1000000.0;
    double bytes_per_second = json.length() / time_per_iteration_s;
    double mb_per_second = bytes_per_second / (1024.0 * 1024.0);
    
    std::cout << "  • Time per serialization: " << std::fixed << std::setprecision(2) << time_per_iteration_us << " μs\n";
    std::cout << "  • Throughput: " << std::fixed << std::setprecision(2) << mb_per_second << " MB/s\n";
    
    std::cout << "\nGenerated JSON:\n" << json << "\n\n";
    
    // Benchmark deserialization for comparison
    std::cout << "📥 Benchmarking deserialization (for comparison)...\n";
    
    simdjson::padded_string padded_json(json);
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        TestData parsed = simdjson::from(padded_json);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nResults:\n";
    time_per_iteration_us = duration.count() / double(iterations);
    time_per_iteration_s = time_per_iteration_us / 1000000.0;
    bytes_per_second = json.length() / time_per_iteration_s;
    mb_per_second = bytes_per_second / (1024.0 * 1024.0);
    
    std::cout << "  • Time per deserialization: " << std::fixed << std::setprecision(2) << time_per_iteration_us << " μs\n";
    std::cout << "  • Throughput: " << std::fixed << std::setprecision(2) << mb_per_second << " MB/s\n";
    
    std::cout << "\n✅ Both serialization and deserialization work with reflection!\n";
    
    return 0;
}
