# simdjson Serialization with C++26 Reflection

This directory now includes examples demonstrating **both** deserialization and serialization using C++26 reflection features.

## 🚀 Quick Start

### Basic Round-Trip Demo
```bash
# Compile and run the simple round-trip demo
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 \
    reflection_roundtrip_demo.cpp ../singleheader/simdjson.cpp \
    -o reflection_roundtrip_demo
./reflection_roundtrip_demo
```

### Conference Demo with Serialization
```bash
# Run the extended conference demo that includes serialization
./conference_demo_serialization.sh
```

## 📝 What's New?

The experimental `simdjson::builder::to_json_string()` function enables one-line serialization:

```cpp
// Deserialize JSON → Struct
MyStruct obj = simdjson::from(json);

// Serialize Struct → JSON (NEW!)
std::string json = simdjson::builder::to_json_string(obj);
```

## 🎯 Features

- **Zero boilerplate**: No need to write serialization code
- **Automatic handling**: Optional fields, containers, nested structures
- **Type-safe**: Compile-time checking with reflection
- **Performant**: Optimized string building

## ⚠️ Requirements

- Bloomberg's clang fork (clang-p2996) with C++26 reflection support
- Build with `-DSIMDJSON_STATIC_REFLECTION=1`
- Include `simdjson/builder/json_builder.h`

## 📊 Performance

While serialization is generally slower than deserialization (due to string building vs parsing), the reflection-based approach is still highly optimized and significantly faster than manual string concatenation approaches.

## 🔧 Status

This serialization support is **experimental** and part of the builder API. The API may change as C++26 reflection features evolve.