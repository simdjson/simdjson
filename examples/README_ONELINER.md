# 🚀 simdjson One-Liner Demo: From Boilerplate to Magic

This demo showcases the dramatic simplification of JSON parsing in C++ using simdjson's new API combined with C++26 reflection.

## Files

- `github_legacy.cpp` - Traditional manual JSON parsing approach (~30 lines of parsing code)
- `github_modern.cpp` - Modern C++26 reflection approach (1 line of parsing code!)
- `CONFERENCE_TALK.md` - Complete conference presentation guide
- `BUILD_INSTRUCTIONS.md` - Detailed compilation instructions
- `conference_demo.sh` - Interactive demo script for presentations

## Quick Start

### Prerequisites
- Bloomberg clang for C++26: https://github.com/bloomberg/clang-p2996
- curl library (usually pre-installed)
- cpr library (built automatically by demo script)

### Easy Demo
```bash
# Just run the demo script - it handles everything!
./conference_demo.sh
```

The script will:
1. Build cpr if needed
2. Compile both examples
3. Run interactive presentation

### Manual Compilation

If you want to compile manually:

```bash
# Build cpr first (if not installed)
mkdir -p ../deps && cd ../deps
git clone https://github.com/libcpr/cpr.git
cd cpr && git checkout 1.10.5
mkdir build && cd build
cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCPR_USE_SYSTEM_CURL=ON -DBUILD_SHARED_LIBS=OFF -DCPR_ENABLE_SSL=OFF
make -j4
cd ../../../examples

# Compile examples
# Legacy
clang++ -std=c++20 -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes \
        -I../include -I.. github_legacy.cpp ../singleheader/simdjson.cpp \
        ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o legacy_demo

# Modern
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 \
        -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes \
        -I../include -I.. github_modern.cpp ../singleheader/simdjson.cpp \
        ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o modern_demo
```

## The Magic ✨

**Before (Legacy):**
```cpp
// 30+ lines of manual parsing...
user.login = std::string(doc["login"].get_string().value());
user.id = doc["id"].get_int64().value();
// ... etc for each field
```

**After (Modern):**
```cpp
// Just ONE line!
GitHubUser user = simdjson::from(simdjson::padded_string(response.text));
```

Both examples fetch real data from GitHub API to demonstrate real-world usage!