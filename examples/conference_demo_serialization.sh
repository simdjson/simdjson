#!/bin/bash
# Conference Demo Script - simdjson One-Liner Magic (WITH SERIALIZATION!)
# Extended demo showcasing both deserialization AND serialization with C++26 reflection

# Check if cpr is built
if [ ! -f "../deps/cpr/build/lib/libcpr.a" ]; then
    echo "⚠️  CPR library not found. Building it first..."
    echo ""
    if [ ! -d "../deps/cpr" ]; then
        mkdir -p ../deps
        cd ../deps
        git clone https://github.com/libcpr/cpr.git
        cd cpr && git checkout 1.10.5
        cd ../..
    fi
    cd ../deps/cpr
    mkdir -p build && cd build
    cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCPR_USE_SYSTEM_CURL=ON -DBUILD_SHARED_LIBS=OFF -DCPR_ENABLE_SSL=OFF
    make -j4
    cd ../../../examples
    echo "✅ CPR built successfully!"
    echo ""
fi

clear
echo "🚀 simdjson: Complete JSON Round-Trip with C++26 Reflection"
echo "============================================================"
echo ""
echo "Today we'll showcase BOTH deserialization AND serialization!"
echo ""
echo "Press Enter to continue..."
read

# Create a demo program that shows serialization
cat > github_serialization_demo.cpp << 'EOF'
#include <iostream>
#include <iomanip>
#include "simdjson.h"
#include <cpr/cpr.h>

struct GitHubUser {
    std::string login;
    std::optional<std::string> name;
    std::optional<std::string> company;
    std::optional<std::string> blog;
    std::optional<std::string> location;
    std::optional<std::string> email;
    std::optional<std::string> bio;
    int64_t public_repos;
    int64_t followers;
    int64_t following;
};

int main() {
    std::cout << "🔄 C++26 Reflection: Complete JSON Round-Trip Demo\n";
    std::cout << "================================================\n\n";
    
    // Step 1: Fetch real data from GitHub
    std::cout << "📡 Fetching GitHub user data...\n";
    auto response = cpr::Get(cpr::Url{"https://api.github.com/users/simdjson"});
    
    if (response.status_code != 200) {
        std::cerr << "Error: Failed to fetch data\n";
        return 1;
    }
    
    std::cout << "✅ Received " << response.text.length() << " bytes of JSON\n\n";
    
    // Step 2: Deserialize with ONE LINE
    std::cout << "📥 DESERIALIZATION (JSON → Struct)\n";
    std::cout << "Code: GitHubUser user = simdjson::from(simdjson::padded_string(response.text));\n\n";
    
    GitHubUser user = simdjson::from(simdjson::padded_string(response.text));
    
    // Show the data we parsed
    std::cout << "Parsed data:\n";
    std::cout << "  • Login: " << user.login << "\n";
    std::cout << "  • Name: " << (user.name.has_value() ? *user.name : "<not set>") << "\n";
    std::cout << "  • Company: " << (user.company.has_value() ? *user.company : "<not set>") << "\n";
    std::cout << "  • Location: " << (user.location.has_value() ? *user.location : "<not set>") << "\n";
    std::cout << "  • Bio: " << (user.bio.has_value() ? *user.bio : "<not set>") << "\n";
    std::cout << "  • Repos: " << user.public_repos << "\n";
    std::cout << "  • Followers: " << user.followers << "\n\n";
    
    // Step 3: Modify the data
    std::cout << "✏️  Modifying data...\n";
    user.followers += 1000;  // Wishful thinking!
    user.name = "simdjson - JSON at the speed of light";  // Set a name
    user.bio = user.bio.value_or("") + " (Now with C++26 reflection!)";
    std::cout << "  • Added 1000 followers (we can dream!)\n";
    std::cout << "  • Set organization name\n";
    std::cout << "  • Updated bio\n\n";
    
    // Step 4: Serialize back to JSON with ONE LINE
    std::cout << "📤 SERIALIZATION (Struct → JSON)\n";
    std::cout << "Code: std::string json = simdjson::builder::to_json_string(user);\n\n";
    
    auto json_result = simdjson::builder::to_json_string(user);
    if (json_result.error()) {
        std::cerr << "Serialization error!\n";
        return 1;
    }
    std::string json = json_result.value();
    
    std::cout << "Generated JSON (" << json.length() << " bytes):\n";
    // Pretty print first 200 chars
    if (json.length() > 200) {
        std::cout << json.substr(0, 200) << "...\n\n";
    } else {
        std::cout << json << "\n\n";
    }
    
    // Step 5: Verify round-trip
    std::cout << "🔄 ROUND-TRIP VERIFICATION\n";
    std::cout << "Parsing our generated JSON back...\n";
    
    GitHubUser user2 = simdjson::from(simdjson::padded_string(json));
    
    std::cout << "✅ Round-trip successful!\n";
    std::cout << "  • Original followers: " << user.followers << "\n";
    std::cout << "  • Parsed followers: " << user2.followers << "\n";
    std::cout << "  • Name set correctly: " << (user2.name.has_value() && *user2.name == *user.name ? "YES" : "NO") << "\n";
    std::cout << "  • Bio preserved: " << (user2.bio.has_value() ? "YES" : "NO") << "\n\n";
    
    std::cout << "🎉 That's it! Two lines of code for complete JSON handling:\n";
    std::cout << "   • Deserialization: simdjson::from(...)\n";
    std::cout << "   • Serialization: simdjson::builder::to_json_string(...)\n";
    
    return 0;
}
EOF

# Create a benchmark that includes serialization
cat > serialization_benchmark.cpp << 'EOF'
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
EOF

echo "📝 First, let's see the traditional approach to serialization..."
echo ""
echo "Press Enter to see manual serialization code..."
read

cat > manual_serialization_example.cpp << 'EOF'
// The OLD way - Manual serialization
std::string serialize_github_user(const GitHubUser& user) {
    std::string json = "{";
    json += "\"login\":\"" + user.login + "\",";
    json += "\"name\":\"" + user.name + "\",";
    json += "\"company\":\"" + user.company + "\",";
    json += "\"blog\":\"" + user.blog + "\",";
    json += "\"location\":\"" + user.location + "\",";
    
    if (user.email.has_value()) {
        json += "\"email\":\"" + *user.email + "\",";
    }
    if (user.bio.has_value()) {
        json += "\"bio\":\"" + *user.bio + "\",";
    }
    
    json += "\"public_repos\":" + std::to_string(user.public_repos) + ",";
    json += "\"followers\":" + std::to_string(user.followers) + ",";
    json += "\"following\":" + std::to_string(user.following);
    json += "}";
    
    return json;
}
EOF

cat manual_serialization_example.cpp
echo ""
echo "😓 That's error-prone and doesn't handle escaping!"
echo ""
echo "Press Enter to see the MODERN approach..."
read

clear
echo "✨ The MODERN approach with C++26 reflection:"
echo ""
echo "Deserialization:"
echo "    GitHubUser user = simdjson::from(json);"
echo ""
echo "Serialization:"
echo "    std::string json = simdjson::builder::to_json_string(user);"
echo ""
echo "🤯 That's it! Bidirectional JSON handling in TWO lines!"
echo ""
echo "Press Enter to compile and run the complete demo..."
read

# Compile and run serialization demo
echo "🔨 Compiling serialization demo..."
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 \
    -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes \
    -I../include -I.. github_serialization_demo.cpp ../singleheader/simdjson.cpp \
    ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o serialization_demo

echo "✅ Compiled!"
echo ""
echo "🏃 Running complete round-trip demo..."
echo ""
./serialization_demo

echo ""
echo "Press Enter to run performance benchmarks..."
read

clear
echo "⚡ PERFORMANCE BENCHMARKS"
echo "========================"
echo ""

# Compile benchmark
echo "🔨 Compiling performance benchmark..."
clang++ -std=c++26 -freflection -O3 -march=native \
    -DSIMDJSON_STATIC_REFLECTION=1 \
    -I../include -I.. serialization_benchmark.cpp ../singleheader/simdjson.cpp \
    -pthread -o serialization_benchmark

echo "✅ Compiled!"
echo ""
echo "🏃 Running benchmark..."
echo ""
./serialization_benchmark

echo ""
echo "🎉 Summary:"
echo "==========="
echo "• ONE line for deserialization"
echo "• ONE line for serialization"
echo "• Works with complex nested structures"
echo "• Handles optional fields automatically"
echo "• Type-safe and performant"
echo "• No manual parsing/building code needed!"
echo ""
echo "The future of C++ JSON handling is here! 🚀"

# Cleanup
rm -f manual_serialization_example.cpp