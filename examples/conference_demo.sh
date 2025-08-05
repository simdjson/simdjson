#!/bin/bash
# Conference Demo Script - simdjson One-Liner Magic
# Run this during your talk for a smooth demo experience

# Check if cpr is built
if [ ! -f "../deps/cpr/build/lib/libcpr.a" ]; then
    echo "⚠️  CPR library not found. Building it first..."
    echo ""
    if [ ! -d "../deps/cpr" ]; then
        mkdir -p ../deps
        cd ../deps
        git clone https://github.com/libcpr/cpr.git
        cd cpr && git checkout 1.10.5
        cd ../..
    fi
    cd ../deps/cpr
    mkdir -p build && cd build
    cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCPR_USE_SYSTEM_CURL=ON -DBUILD_SHARED_LIBS=OFF -DCPR_ENABLE_SSL=OFF
    make -j4
    cd ../../../examples
    echo "✅ CPR built successfully!"
    echo ""
fi

clear
echo "🚀 simdjson: From Boilerplate to One-Liner"
echo "==========================================="
echo ""
echo "Press Enter to continue..."
read

# Show the legacy approach
echo "📚 First, let's look at the LEGACY approach..."
echo ""
echo "Opening github_legacy.cpp..."
sleep 1
echo ""
echo "Key points:"
echo "  • Manual parse_github_user() function"
echo "  • Extract each field individually"
echo "  • Handle optional fields explicitly"
echo "  • ~30 lines of parsing code"
echo ""
echo "Press Enter to see the code..."
read

# Display key parts of legacy code
cat github_legacy.cpp | grep -A 20 "parse_github_user" | head -25

echo ""
echo "😓 That's a lot of boilerplate!"
echo ""
echo "Press Enter to compile and run..."
read

# Compile legacy
echo "🔨 Compiling legacy version..."
echo "Command: clang++ -std=c++20 -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes -I../include -I.. github_legacy.cpp ../singleheader/simdjson.cpp ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o legacy_demo"
clang++ -std=c++20 -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes -I../include -I.. github_legacy.cpp ../singleheader/simdjson.cpp ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o legacy_demo
echo "✅ Compiled!"
echo ""
echo "🏃 Running legacy demo (fetching real GitHub data)..."
echo ""
./legacy_demo

echo ""
echo "Press Enter to see the MODERN approach..."
read

clear
echo "✨ Now, let's look at the MODERN approach with C++26 reflection..."
echo ""
echo "Opening github_modern.cpp..."
sleep 1
echo ""
echo "Key points:"
echo "  • Just declare your struct"
echo "  • ONE line of parsing code"
echo "  • C++26 reflection handles everything"
echo "  • No manual field extraction!"
echo ""
echo "Press Enter to see the magic..."
read

# Show the struct and the one-liner
echo "The struct (just a plain struct!):"
echo ""
cat github_modern.cpp | grep -A 10 "struct GitHubUser" | head -12
echo ""
echo "The parsing code (ONE LINE!):"
echo ""
echo "    GitHubUser user = simdjson::from(simdjson::padded_string(response.text));"
echo ""
echo "🤯 That's it! That's all the parsing code!"
echo ""
echo "Press Enter to compile with C++26 reflection..."
read

# Compile modern
echo "🔨 Compiling modern version with Bloomberg clang..."
echo "Command: clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes -I../include -I.. github_modern.cpp ../singleheader/simdjson.cpp ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o modern_demo"
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes -I../include -I.. github_modern.cpp ../singleheader/simdjson.cpp ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o modern_demo
echo "✅ Compiled!"
echo ""
echo "🏃 Running modern demo (fetching real GitHub data)..."
echo ""
./modern_demo

echo ""
echo "Press Enter to see side-by-side comparison..."
read

clear
echo "📊 SIDE-BY-SIDE COMPARISON"
echo "========================="
echo ""
echo "Legacy Approach:                    Modern Approach (C++26):"
echo "----------------                    ------------------------"
echo "❌ 30+ lines of parsing code        ✅ 1 line of parsing code"
echo "❌ Manual field extraction          ✅ Automatic with reflection"
echo "❌ Error-prone                      ✅ Type-safe"
echo "❌ Hard to maintain                 ✅ Just update the struct"
echo "❌ Boilerplate for each type        ✅ Works for any struct"
echo ""
echo ""
echo "Press Enter to run PERFORMANCE BENCHMARKS..."
read

clear
echo "⚡ DESERIALIZATION PERFORMANCE BENCHMARKS"
echo "========================================"
echo ""
echo "Let's measure the actual JSON → struct deserialization speed..."
echo ""

# Compile benchmarks if not exist
if [ ! -f "./legacy_benchmark" ] || [ ! -f "./modern_benchmark" ]; then
    echo "🔨 Compiling benchmark versions..."
    
    clang++ -std=c++20 -O3 -march=native \
      -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes \
      -I../include -I.. github_legacy_benchmark.cpp ../singleheader/simdjson.cpp \
      ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o legacy_benchmark 2>/dev/null
    
    clang++ -std=c++26 -freflection -O3 -march=native \
      -DSIMDJSON_STATIC_REFLECTION=1 \
      -I../deps/cpr/include -I../deps/cpr/build/cpr_generated_includes \
      -I../include -I.. github_modern_benchmark.cpp ../singleheader/simdjson.cpp \
      ../deps/cpr/build/lib/libcpr.a -lcurl -pthread -o modern_benchmark 2>/dev/null
fi

echo "🏃 Running legacy benchmark..."
echo ""
./legacy_benchmark

echo ""
echo "Press Enter to run modern benchmark..."
read

echo "🏃 Running modern benchmark..."
echo ""
./modern_benchmark

echo ""
echo "🎉 The future of C++ is here!"
echo ""
echo "Questions?"