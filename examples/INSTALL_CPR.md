# Installing CPR Library

The examples require the CPR library for making HTTP requests to the GitHub API.

## Option 1: Install via Package Manager (Recommended)

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libcpr-dev
```

### macOS (Homebrew)
```bash
brew install cpr
```

### Arch Linux
```bash
sudo pacman -S cpr
```

## Option 2: Build from Source

```bash
# Clone cpr
git clone https://github.com/libcpr/cpr.git
cd cpr

# Build and install
mkdir build && cd build
cmake .. -DCPR_USE_SYSTEM_CURL=ON
make
sudo make install
```

## Option 3: Using vcpkg

```bash
vcpkg install cpr
```

## Option 4: Using Conan

```bash
conan install cpr/1.10.5@
```

## Compilation

Once cpr is installed, compile the examples:

```bash
# Legacy example
clang++ -std=c++20 -I../include -I.. github_legacy.cpp -lcpr -lcurl -o github_legacy_demo

# Modern example (requires Bloomberg clang)
clang++ -std=c++26 -freflection -DSIMDJSON_STATIC_REFLECTION=1 \
        -I../include -I.. github_modern.cpp -lcpr -lcurl -o github_modern_demo
```

## Troubleshooting

If you get "cpr/cpr.h not found", check:
- `pkg-config --cflags --libs cpr`
- Add include path: `-I/usr/local/include`
- Add library path: `-L/usr/local/lib`

For SSL issues, ensure OpenSSL is installed:
- Ubuntu/Debian: `sudo apt-get install libssl-dev`
- macOS: `brew install openssl`
- Link OpenSSL: `-lssl -lcrypto`