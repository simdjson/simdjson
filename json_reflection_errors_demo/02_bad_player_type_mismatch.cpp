// Example 2: Expected COMPILE ERROR - Type mismatch detected
// According to the slides, this should fail at compile time when reflection
// detects that JSON will have a string but the struct expects an int
//
// Compilation command:
// clang++ -std=c++26 -freflection -fexpansion-statements -stdlib=libc++ \
//         -DSIMDJSON_STATIC_REFLECTION=1 -DSIMDJSON_EXCEPTIONS=1 \
//         -I../include -I../singleheader \
//         02_bad_player_type_mismatch.cpp ../singleheader/simdjson.cpp \
//         -o 02_bad_player_type_mismatch
//
// EXPECTED: Compile error
// ACTUAL: Compiles successfully, throws runtime error

#include <simdjson.h>
#include <string>
#include <iostream>

// ❌ COMPILE ERROR: Type mismatch detected
struct BadPlayer {
    int username;  // Oops, should be string!
    int level;
    double health;
};

int main() {
    // This JSON has "username" as a string, but BadPlayer expects int
    std::string json = R"({"username":"Alice","level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    
    // According to slides: simdjson::from<BadPlayer>(json) won't compile if JSON has string
    // Reality: This compiles but throws runtime error
    BadPlayer p = simdjson::from(padded);
    
    std::cout << "BadPlayer parsed (shouldn't reach here):" << std::endl;
    std::cout << "  Username (as int): " << p.username << std::endl;
    std::cout << "  Level: " << p.level << std::endl;
    std::cout << "  Health: " << p.health << std::endl;
    
    return 0;
}