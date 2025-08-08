// Complete JSON Round-Trip Demo with C++26 Reflection
// Compile with: clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 reflection_roundtrip_demo.cpp ../singleheader/simdjson.cpp -o reflection_roundtrip_demo
#include <iostream>
#include <vector>
#include <optional>
#include "simdjson.h"

// Define our data structure - just a plain struct!
struct Product {
    std::string name;
    double price;
    std::vector<std::string> tags;
    std::optional<std::string> description;
    bool in_stock;
};

int main() {
    std::cout << "🔄 simdjson C++26 Reflection Round-Trip Demo\n";
    std::cout << "============================================\n\n";
    
    // Original JSON data
    const char* json_data = R"({
        "name": "High-Performance JSON Parser",
        "price": 0.0,
        "tags": ["C++", "performance", "json", "reflection"],
        "description": "The fastest JSON parser with C++26 reflection support",
        "in_stock": true
    })";
    
    std::cout << "📄 Original JSON:\n" << json_data << "\n\n";
    
    // Step 1: Deserialize JSON to struct with ONE line
    std::cout << "📥 Deserializing JSON → Struct...\n";
    Product product = simdjson::from(simdjson::padded_string(json_data));
    
    std::cout << "✅ Deserialized successfully!\n";
    std::cout << "   • Name: " << product.name << "\n";
    std::cout << "   • Price: $" << product.price << "\n";
    std::cout << "   • Tags: ";
    for (const auto& tag : product.tags) {
        std::cout << tag << " ";
    }
    std::cout << "\n   • In stock: " << (product.in_stock ? "Yes" : "No") << "\n\n";
    
    // Step 2: Modify the data
    std::cout << "✏️  Modifying data...\n";
    product.price = 99.99;  // It's worth something now!
    product.tags.push_back("C++26");
    product.description = "Now with serialization support!";
    std::cout << "   • Changed price to $99.99\n";
    std::cout << "   • Added 'C++26' tag\n";
    std::cout << "   • Updated description\n\n";
    
    // Step 3: Serialize struct back to JSON with ONE line
    std::cout << "📤 Serializing Struct → JSON...\n";
    auto json_result = simdjson::builder::to_json_string(product);
    
    if (json_result.error()) {
        std::cerr << "❌ Serialization failed!\n";
        return 1;
    }
    
    std::string new_json = json_result.value();
    std::cout << "✅ Serialized successfully!\n\n";
    
    std::cout << "📄 Generated JSON:\n" << new_json << "\n\n";
    
    // Step 4: Verify round-trip by parsing again
    std::cout << "🔍 Verifying round-trip...\n";
    Product product2 = simdjson::from(simdjson::padded_string(new_json));
    
    std::cout << "✅ Round-trip successful!\n";
    std::cout << "   • Price preserved: $" << product2.price << "\n";
    std::cout << "   • New tag present: " 
              << (std::find(product2.tags.begin(), product2.tags.end(), "C++26") != product2.tags.end() ? "Yes" : "No") 
              << "\n";
    std::cout << "   • Description updated: " << (product2.description.has_value() ? "Yes" : "No") << "\n\n";
    
    std::cout << "🎉 That's it! Complete JSON handling in just 2 lines:\n";
    std::cout << "   • Deserialize: auto obj = simdjson::from(json);\n";
    std::cout << "   • Serialize: auto json = simdjson::builder::to_json_string(obj);\n\n";
    
    std::cout << "⚡ No manual parsing, no boilerplate, just pure C++26 magic!\n";
    
    return 0;
}