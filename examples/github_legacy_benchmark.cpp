// Legacy approach with performance measurement
#include <simdjson.h>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <optional>
#include <chrono>
#include <iomanip>
#include <sstream>

struct GitHubUser {
    std::string login;
    int64_t id;
    std::string name;
    std::optional<std::string> company;
    std::optional<std::string> location;
    int64_t public_repos;
    int64_t followers;
};

// Manual parsing function (the old way)
GitHubUser parse_github_user(const std::string& json_str) {
    GitHubUser user;
    
    simdjson::ondemand::parser parser;
    simdjson::padded_string json(json_str);
    simdjson::ondemand::document doc = parser.iterate(json);
    
    // Manual field extraction with error checking
    user.login = std::string(doc["login"].get_string().value());
    user.id = doc["id"].get_int64().value();
    user.name = std::string(doc["name"].get_string().value());
    
    // Handle optional fields
    auto company_result = doc["company"];
    if (!company_result.is_null()) {
        user.company = std::string(company_result.get_string().value());
    }
    
    auto location_result = doc["location"];
    if (!location_result.is_null()) {
        user.location = std::string(location_result.get_string().value());
    }
    
    user.public_repos = doc["public_repos"].get_int64().value();
    user.followers = doc["followers"].get_int64().value();
    
    return user;
}

// Manual serialization function (the old way)
std::string serialize_github_user(const GitHubUser& user) {
    std::ostringstream json;
    json << "{";
    json << "\"login\":\"" << user.login << "\",";
    json << "\"id\":" << user.id << ",";
    json << "\"name\":\"" << user.name << "\",";
    
    if (user.company.has_value()) {
        json << "\"company\":\"" << *user.company << "\",";
    } else {
        json << "\"company\":null,";
    }
    
    if (user.location.has_value()) {
        json << "\"location\":\"" << *user.location << "\",";
    } else {
        json << "\"location\":null,";
    }
    
    json << "\"public_repos\":" << user.public_repos << ",";
    json << "\"followers\":" << user.followers;
    json << "}";
    
    return json.str();
}

int main() {
    std::cout << "📚 Legacy Approach - Deserialization Performance Benchmark\n";
    std::cout << "=========================================================\n\n";
    
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
        auto user = parse_github_user(response.text);
    }
    
    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto user = parse_github_user(response.text);
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
    
    // Now benchmark serialization
    std::cout << "\n📝 Serialization Performance Benchmark\n";
    std::cout << "=====================================\n\n";
    
    // Parse once to get a user object
    auto user = parse_github_user(response.text);
    
    // Warm up serialization
    for (int i = 0; i < 100; ++i) {
        auto json_str = serialize_github_user(user);
    }
    
    // Benchmark serialization
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto json_str = serialize_github_user(user);
        // Prevent optimization
        if (i == 0) {
            std::cout << "Serialized JSON size: " << json_str.length() << " bytes\n\n";
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate serialization performance metrics
    time_per_parse = duration.count() / static_cast<double>(iterations);
    double serialized_size = serialize_github_user(user).length();
    gb_per_second = (serialized_size * iterations) / (duration.count() * 1000.0);
    
    std::cout << "📊 Serialization Performance Results:\n";
    std::cout << "  • JSON size: " << serialized_size << " bytes\n";
    std::cout << "  • Iterations: " << iterations << "\n";
    std::cout << "  • Total time: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "  • Time per serialization: " << std::fixed << std::setprecision(2) << time_per_parse << " μs\n";
    std::cout << "  • Serialization speed: " << std::fixed << std::setprecision(2) << gb_per_second << " GB/s\n";
    std::cout << "\n⚠️  Note: Manual serialization with string concatenation\n";
    
    return 0;
}