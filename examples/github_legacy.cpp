// Legacy approach - manual JSON parsing with simdjson
#include <simdjson.h>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <optional>

struct GitHubUser {
    std::string login;
    int64_t id;
    std::string name;
    std::optional<std::string> company;
    std::optional<std::string> location;
    int64_t public_repos;
    int64_t followers;
};

// Legacy approach - manual parsing with lots of boilerplate
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

int main() {
    std::cout << "📚 Legacy Approach - Manual JSON Parsing\n";
    std::cout << "========================================\n\n";
    
    // Fetch data from GitHub API
    auto response = cpr::Get(
        cpr::Url{"https://api.github.com/users/lemire"},
        cpr::Header{{"User-Agent", "simdjson-legacy-demo"}}
    );
    
    if (response.status_code != 200) {
        std::cerr << "❌ Failed to fetch data: HTTP " << response.status_code << "\n";
        return 1;
    }
    
    try {
        // 😓 The old way - manual parsing with lots of code
        GitHubUser user = parse_github_user(response.text);
        
        // Display results
        std::cout << "GitHub User: " << user.name << " (@" << user.login << ")\n";
        std::cout << "ID: " << user.id << "\n";
        if (user.company) std::cout << "Company: " << *user.company << "\n";
        if (user.location) std::cout << "Location: " << *user.location << "\n";
        std::cout << "Public Repos: " << user.public_repos << "\n";
        std::cout << "Followers: " << user.followers << "\n";
        
        std::cout << "\n⚠️  Notice all the manual parsing code required!\n";
        
    } catch (const simdjson::simdjson_error& e) {
        std::cerr << "❌ Parsing error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}