# 0.5

## Highlights

Performance
* Faster and simpler UTF-8 validation with the lookup4 algorithm https://github.com/simdjson/simdjson/pull/993
* We improved the performance of simdjson under Visual Studio by about 25%. Users will still get better performance with clang-cl (+30%) but the gap has been reduced. https://github.com/simdjson/simdjson/pull/1031

Code usability
* In `parse_many`, when parsing streams of JSON documetns, we give to the users runtime control as to whether threads are used (via the parser.threaded attribute). https://github.com/simdjson/simdjson/issues/925
* Prefixed public macros to avoid name clashes with other libraries. https://github.com/simdjson/simdjson/issues/1035
* Better documentation regarding package managers (brew, MSYS2, conan, apt, vcpkg, FreeBSD package manager, etc.).
* Better documentation regarding  CMake usage.

Standards
* We improved standard compliance with respect to both the JSON RFC 8259 and JSON Pointer RFC 6901. We added the at_pointer method to nodes for standard-compliant JSON Pointer queries. The legacy `at(std::string_view)` method remains but is deprecated since it is not standard-compliant as per  RFC 6901.
* We removed computed GOTOs without sacrificing performance thus improving the C++ standard compliance (since computed GOTOs are compiler-specific extensions).
* Better support for C++20 https://github.com/simdjson/simdjson/pull/1050

# 0.4

## Highlights

- Test coverage has been greatly improved and we have resolved many static-analysis warnings on different systems.
- We added a fast (8GB/s) minifier that works directly on JSON strings.
- We added fast (10GB/s) UTF-8 validator that works directly on strings (any strings, including non-JSON).
- The array and object elements have a constant-time size() method.
- Performance improvements to the API (type(), get<>()).
- The parse_many function (ndjson) has been entirely reworked. It now uses a single secondary thread instead of several new threads.
- We have introduced a faster UTF-8 validation algorithm (lookup3) for all kernels (ARM, x64 SSE, x64 AVX).
- C++11 support for older compilers and systems.
- FreeBSD support (and tests).
- We support the clang front-end compiler (clangcl) under Visual Studio.
- It is now possible to target ARM platforms under Visual Studio.
- The simdjson library will never abort or print to standard output/error.

# 0.3

## Highlights

- **Multi-Document Parsing:** Read a bundle of JSON documents (ndjson) 2-4x faster than doing it
  individually. [API docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#newline-delimited-json-ndjson-and-json-lines) / [Design Details](https://github.com/simdjson/simdjson/blob/master/doc/parse_many.md)
- **Simplified API:** The API has been completely revamped for ease of use, including a new JSON
  navigation API and fluent support for error code *and* exception styles of error handling with a
  single API. [Docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#the-basics-loading-and-parsing-json-documents)
- **Exact Float Parsing:** Now simdjson parses floats flawlessly *without* any performance loss,
  thanks to [great work by @michaeleisel and @lemire](https://github.com/simdjson/simdjson/pull/558).
  [Blog Post](https://lemire.me/blog/2020/03/10/fast-float-parsing-in-practice/)
- **Even Faster:** The fastest parser got faster! With a [shiny new UTF-8 validator](https://github.com/simdjson/simdjson/pull/387)
  and meticulously refactored SIMD core, simdjson 0.3 is 15% faster than before, running at 2.5 GB/s
  (where 0.2 ran at 2.2 GB/s).

## Minor Highlights

- Fallback implementation: simdjson now has a non-SIMD fallback implementation, and can run even on
  very old 64-bit machines.
- Automatic allocation: as part of API simplification, the parser no longer has to be preallocated--
  it will adjust automatically when it encounters larger files.
- Runtime selection API: We've exposed simdjson's runtime CPU detection and implementation selection
  as an API, so you can tell what implementation we detected and test with other implementations.
- Error handling your way: Whether you use exceptions or check error codes, simdjson lets you handle
  errors in your style. APIs that can fail return simdjson_result<T>, letting you check the error
  code before using the result. But if you are more comfortable with exceptions, skip the error code
  and cast straight to T, and exceptions will be thrown automatically if an error happens. Use the
  same API either way!
- Error chaining: We also worked to keep non-exception error-handling short and sweet. Instead of
  having to check the error code after every single operation, now you can *chain* JSON navigation
  calls like looking up an object field or array element, or casting to a string, so that you only
  have to check the error code once at the very end.
