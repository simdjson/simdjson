// Modern approach - C++26 reflection with simdjson
// Compile with Bloomberg clang fork:
// clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 ...
#include <simdjson.h>
#include <simdjson/convert.h>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <optional>

// 🎯 JUST DECLARE YOUR STRUCT - THAT'S IT!
// No serialization code needed with C++26 reflection
struct GitHubUser {
    std::string login;
    int64_t id;
    std::string name;
    std::optional<std::string> company;
    std::optional<std::string> location;
    int64_t public_repos;
    int64_t followers;
};
// NO BOILERPLATE CODE NEEDED! 🎉

int main() {
    std::cout << "✨ Modern Approach - C++26 Reflection\n";
    std::cout << "=====================================\n\n";
    
    // Fetch data from GitHub API
    auto response = cpr::Get(
        cpr::Url{"https://api.github.com/users/lemire"},
        cpr::Header{{"User-Agent", "simdjson-modern-demo"}}
    );
    
    if (response.status_code != 200) {
        std::cerr << "❌ Failed to fetch data: HTTP " << response.status_code << "\n";
        return 1;
    }
    
    try {
        // ✨ THE MAGIC - Just one line, no boilerplate!
        // C++26 reflection handles everything automatically
        GitHubUser user = simdjson::from(simdjson::padded_string(response.text));
        
        // Display results
        std::cout << "GitHub User: " << user.name << " (@" << user.login << ")\n";
        std::cout << "ID: " << user.id << "\n";
        if (user.company) std::cout << "Company: " << *user.company << "\n";
        if (user.location) std::cout << "Location: " << *user.location << "\n";
        std::cout << "Public Repos: " << user.public_repos << "\n";
        std::cout << "Followers: " << user.followers << "\n";
        
        std::cout << "\n🚀 That's it! No manual parsing code needed!\n";
        std::cout << "   C++26 reflection generates everything automatically.\n";
        
    } catch (const simdjson::simdjson_error& e) {
        std::cerr << "❌ Parsing error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
