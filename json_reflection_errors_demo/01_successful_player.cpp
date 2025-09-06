// Example 1: Successful compile-time code generation with static reflection
// This demonstrates how reflection reads the struct at compile time and generates parsing code
//
// Compilation command:
// clang++ -std=c++26 -freflection -fexpansion-statements -stdlib=libc++ \
//         -DSIMDJSON_STATIC_REFLECTION=1 -DSIMDJSON_EXCEPTIONS=1 \
//         -I../include -I../singleheader \
//         01_successful_player.cpp ../singleheader/simdjson.cpp \
//         -o 01_successful_player

#include <simdjson.h>
#include <string>
#include <iostream>

struct Player {
    std::string username;    // ← Compile-time: reflection sees this
    int level;               // ← Compile-time: reflection sees this
    double health;           // ← Compile-time: reflection sees this
};

// COMPILE TIME: Reflection reads Player's structure and generates:
// - Code to read "username" as string
// - Code to read "level" as int
// - Code to read "health" as double

int main() {
    // RUNTIME: The generated code processes actual JSON data
    std::string json = R"({"username":"Alice","level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    
    Player p = simdjson::from(padded);
    // Runtime values flow through compile-time generated code
    
    std::cout << "Player successfully parsed:" << std::endl;
    std::cout << "  Username: " << p.username << std::endl;
    std::cout << "  Level: " << p.level << std::endl;
    std::cout << "  Health: " << p.health << std::endl;
    
    // Also test serialization
    std::string serialized = simdjson::to_json_string(p);
    std::cout << "\nSerialized back to JSON: " << serialized << std::endl;
    
    return 0;
}