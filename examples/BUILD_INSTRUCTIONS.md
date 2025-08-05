# 🛠️ Build Instructions for simdjson One-Liner Demo

## Prerequisites

### For Both Examples
- **cpr library** (for HTTP requests) - See [INSTALL_CPR.md](INSTALL_CPR.md) for installation
- **curl library** (cpr dependency) - Usually pre-installed on most systems
- **simdjson library** - Included in this repository

### For Legacy Example (github_legacy.cpp)
- Any C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)

### For Modern Example (github_modern.cpp)  
- Bloomberg Clang fork with C++26 reflection support
  - Get it from: https://github.com/bloomberg/clang-p2996

## 🔨 Compilation Commands

### Quick Setup (Recommended)

1. **Build cpr from source** (if not installed system-wide):
```bash
# From the examples directory
mkdir -p ../deps && cd ../deps
git clone https://github.com/libcpr/cpr.git
cd cpr && git checkout 1.10.5
mkdir build && cd build
cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCPR_USE_SYSTEM_CURL=ON -DBUILD_SHARED_LIBS=OFF -DCPR_ENABLE_SSL=OFF
make -j4
cd ../../../examples
```

2. **Compile the examples**:
```bash
# Legacy approach (any C++20 compiler)
clang++ -std=c++20 \
  -I../deps/cpr/include \
  -I../deps/cpr/build/cpr_generated_includes \
  -I../include -I.. \
  github_legacy.cpp \
  ../singleheader/simdjson.cpp \
  ../deps/cpr/build/lib/libcpr.a \
  -lcurl -pthread \
  -o github_legacy_demo

# Modern approach (Bloomberg clang with reflection)
clang++ -std=c++26 -freflection \
  -DSIMDJSON_STATIC_REFLECTION=1 \
  -I../deps/cpr/include \
  -I../deps/cpr/build/cpr_generated_includes \
  -I../include -I.. \
  github_modern.cpp \
  ../singleheader/simdjson.cpp \
  ../deps/cpr/build/lib/libcpr.a \
  -lcurl -pthread \
  -o github_modern_demo
```

### Using System-Installed cpr

If cpr is installed system-wide (see [INSTALL_CPR.md](INSTALL_CPR.md)):

```bash
# Legacy approach
clang++ -std=c++20 \
  -I../include -I.. \
  github_legacy.cpp \
  -lcpr -lcurl \
  -o github_legacy_demo

# Modern approach
clang++ -std=c++26 -freflection \
  -DSIMDJSON_STATIC_REFLECTION=1 \
  -I../include -I.. \
  github_modern.cpp \
  -lcpr -lcurl \
  -o github_modern_demo
```

### Using simdjson Single Header

```bash
# Legacy approach
clang++ -std=c++20 \
  -I../singleheader \
  github_legacy.cpp \
  ../singleheader/simdjson.cpp \
  -lcpr -lcurl \
  -o github_legacy_demo

# Modern approach
clang++ -std=c++26 -freflection \
  -DSIMDJSON_STATIC_REFLECTION=1 \
  -I../singleheader -I../include \
  github_modern.cpp \
  ../singleheader/simdjson.cpp \
  -lcpr -lcurl \
  -o github_modern_demo
```

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_CXX_COMPILER=clang++ \
         -DCMAKE_CXX_STANDARD=26 \
         -DCMAKE_CXX_FLAGS="-freflection -DSIMDJSON_STATIC_REFLECTION=1"

# Build both examples
make github_legacy github_modern
```

### Quick Build with simdjson Single Header

```bash
# Legacy approach
clang++ -std=c++20 \
  -I../singleheader \
  github_legacy.cpp \
  ../singleheader/simdjson.cpp \
  -lcpr -lcurl \
  -o github_legacy_demo

# Modern approach  
clang++ -std=c++26 -freflection \
  -DSIMDJSON_STATIC_REFLECTION=1 \
  -I../singleheader \
  github_modern.cpp \
  ../singleheader/simdjson.cpp \
  -lcpr -lcurl \
  -o github_modern_demo
```

## 🏃 Running the Examples

```bash
# Run legacy version
./github_legacy_demo

# Run modern version  
./github_modern_demo
```

## 🎯 Platform-Specific Notes

### Linux (x86_64)
```bash
-DSIMDJSON_IMPLEMENTATION_HASWELL=1
```

### Linux (ARM64)
```bash
-DSIMDJSON_IMPLEMENTATION_ARM64=1
```

### macOS (Apple Silicon)
```bash
-DSIMDJSON_IMPLEMENTATION_ARM64=1
```

### macOS (Intel)
```bash
-DSIMDJSON_IMPLEMENTATION_HASWELL=1
```

### Windows (MSVC)
```cmd
cl /std:c++20 /I..\include /I.. github_legacy.cpp /Fe:github_legacy_demo.exe
```

## 🐛 Troubleshooting

### "reflection feature not available"
- Ensure you're using the Bloomberg clang fork
- Check version: `clang++ --version` should show bloomberg/clang-p2996

### "SIMDJSON_STATIC_REFLECTION not working"
- Make sure to define it before including headers:
  ```cpp
  #define SIMDJSON_STATIC_REFLECTION 1
  #include <simdjson.h>
  ```

### Linking errors
- Use single-header approach for simplicity
- Or ensure simdjson library is properly built and linked

### Performance issues
- Add optimization flags: `-O3 -march=native`
- Enable LTO: `-flto`

## 📦 Creating a Portable Demo

For conferences, prepare the demo environment:

```bash
# Install cpr library (if not available)
# Ubuntu/Debian:
sudo apt-get install libcpr-dev

# macOS:
brew install cpr

# Or build from source:
git clone https://github.com/libcpr/cpr.git
cd cpr && mkdir build && cd build
cmake .. && make && sudo make install

# Create demo directory
mkdir simdjson_oneliner_demo
cd simdjson_oneliner_demo

# Copy necessary files
cp path/to/github_legacy.cpp .
cp path/to/github_modern.cpp .
cp -r path/to/simdjson/include .
cp -r path/to/simdjson/singleheader .

# Create build script
cat > build_demo.sh << 'EOF'
#!/bin/bash
echo "🔨 Building Legacy Example..."
clang++ -std=c++20 -O3 -I./include -I. \
  github_legacy.cpp -lcpr -lcurl -o legacy_demo

echo "🔨 Building Modern Example (C++26)..."  
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 -O3 \
  -I./include -I. github_modern.cpp -lcpr -lcurl -o modern_demo

echo "✅ Build complete!"
echo "Run: ./legacy_demo or ./modern_demo"
EOF

chmod +x build_demo.sh
```

## 🎪 Conference Checklist

- [ ] Bloomberg clang installed on demo machine
- [ ] Both examples compile cleanly  
- [ ] Internet connection for live API calls (or use standalone)
- [ ] Backup: pre-recorded video of compilation and execution
- [ ] Slides with code snippets
- [ ] QR code for GitHub repo

## 🔗 Quick Links

- simdjson: https://github.com/simdjson/simdjson
- Bloomberg Clang: https://github.com/bloomberg/clang-p2996  
- P2996 Reflection Proposal: https://wg21.link/p2996
- Talk Resources: [Your GitHub repo with examples]