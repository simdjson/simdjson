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
