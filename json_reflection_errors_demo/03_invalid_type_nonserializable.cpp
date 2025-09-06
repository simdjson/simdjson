// Example 3: Expected COMPILE ERROR - Non-serializable type
// According to the slides, this should fail at compile time because
// std::thread cannot be serialized to JSON
//
// Compilation command:
// clang++ -std=c++26 -freflection -fexpansion-statements -stdlib=libc++ \
//         -DSIMDJSON_STATIC_REFLECTION=1 -DSIMDJSON_EXCEPTIONS=1 \
//         -I../include -I../singleheader \
//         03_invalid_type_nonserializable.cpp ../singleheader/simdjson.cpp \
//         -o 03_invalid_type_nonserializable
//
// EXPECTED: Compile error
// ACTUAL: Compiles and runs successfully, serializes thread as JSON object

#include <simdjson.h>
#include <string>
#include <thread>
#include <iostream>

// ❌ COMPILE ERROR: Non-serializable type
struct InvalidType {
    std::string name;
    std::thread t;  // Threads can't be serialized!
    int value;
};

int main() {
    InvalidType invalid{
        "test",
        std::thread([]{ 
            // Empty thread function
        }),
        42
    };
    
    // According to slides: simdjson::to_json(InvalidType{}) fails at compile time
    // Reality: This compiles and even runs, serializing thread as an object
    std::string json = simdjson::to_json_string(invalid);
    
    std::cout << "InvalidType serialized (shouldn't reach here):" << std::endl;
    std::cout << json << std::endl;
    
    // Clean up thread
    if (invalid.t.joinable()) {
        invalid.t.join();
    }
    
    return 0;
}