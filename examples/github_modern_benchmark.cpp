// Modern approach with performance measurement - C++26 reflection
#include <simdjson.h>
#include <simdjson/convert.h>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <optional>
#include <chrono>
#include <iomanip>
#include <sstream>

// Just declare your struct - reflection handles the rest!
struct GitHubUser {
    std::string login;
    int64_t id;
    std::string name;
    std::optional<std::string> company;
    std::optional<std::string> location;
    int64_t public_repos;
    int64_t followers;
};

int main() {
    std::cout << "✨ Modern Approach - Deserialization Performance Benchmark (C++26)\n";
    std::cout << "================================================================\n\n";
    
    // Fetch data from GitHub API
    auto response = cpr::Get(
        cpr::Url{"https://api.github.com/users/lemire"},
        cpr::Header{{"User-Agent", "simdjson-benchmark"}}
    );
    
    if (response.status_code != 200) {
        std::cerr << "❌ Failed to fetch data: HTTP " << response.status_code << "\n";
        return 1;
    }
    
    // Warm up
    for (int i = 0; i < 100; ++i) {
        GitHubUser user = simdjson::from(simdjson::padded_string(response.text));
    }
    
    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        GitHubUser user = simdjson::from(simdjson::padded_string(response.text));
        // Prevent optimization
        if (i == 0) {
            std::cout << "Parsing: " << user.name << " (@" << user.login << ")\n\n";
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate performance metrics
    double time_per_parse = duration.count() / static_cast<double>(iterations);
    double bytes_per_parse = response.text.size();
    double gb_per_second = (bytes_per_parse * iterations) / (duration.count() * 1000.0);
    
    std::cout << "📊 Deserialization Performance Results:\n";
    std::cout << "  • JSON size: " << bytes_per_parse << " bytes\n";
    std::cout << "  • Iterations: " << iterations << "\n";
    std::cout << "  • Total time: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "  • Time per deserialization: " << std::fixed << std::setprecision(2) << time_per_parse << " μs\n";
    std::cout << "  • Deserialization speed: " << std::fixed << std::setprecision(2) << gb_per_second << " GB/s\n";
    
    std::cout << "\n🚀 Same performance, just ONE line of deserialization code!\n";
    
    // Serialization note
    std::cout << "\n📝 Serialization with Reflection\n";
    std::cout << "================================\n\n";
    std::cout << "⚠️  NOTE: simdjson doesn't yet support reflection-based serialization.\n";
    std::cout << "   The `simdjson::to` function is not yet implemented.\n";
    std::cout << "   This is a future enhancement that would provide:\n";
    std::cout << "   • One-line serialization: simdjson::to<std::string>(user)\n";
    std::cout << "   • Automatic JSON generation from C++ structs\n";
    std::cout << "   • No manual string building required\n";
    std::cout << "\n";
    std::cout << "   For now, serialization still requires manual implementation,\n";
    std::cout << "   but deserialization is fully automated with reflection! 🎉\n";
    
    return 0;
}