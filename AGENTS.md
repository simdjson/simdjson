# AGENTS.md

Instructions for coding agents. Read this before editing the tree.
Human contributors start with [CONTRIBUTING.md](CONTRIBUTING.md) and
[HACKING.md](HACKING.md). Every contribution also follows
[AI_USAGE_POLICY.md](AI_USAGE_POLICY.md).

## What this library is

simdjson parses JSON with SIMD instructions. The sources you edit are under
`include/` and `src/`. The files `singleheader/simdjson.h` and
`singleheader/simdjson.cpp` are generated at release time.

The public library is C++11 (`cxx_std_11`). In-tree tests and tools build as
C++17. Anything that needs a newer standard, including static reflection, is
compiled only when that standard is available.

Two front ends share stage 1:

- DOM (`simdjson::dom`) materializes a tape. Stage 2 parses numbers, strings, and atoms.
- On-Demand (`simdjson::ondemand`) walks the document without building a tree.

Stage 1 finds structural indexes and validates UTF-8.

## Build and test

From the repository root:

```bash
cmake -B build -D SIMDJSON_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The default build type is Release. Developer mode is required for tests,
examples, and benchmarks. One test:

```bash
ctest --test-dir build -R ondemand_number_tests --output-on-failure
```

Sanitizers, when the compiler supports them:

```bash
cmake -B build-sani -D SIMDJSON_SANITIZE=ON -D SIMDJSON_DEVELOPER_MODE=ON
cmake --build build-sani
ctest --test-dir build-sani --output-on-failure
```

`SIMDJSON_EXCEPTIONS` defaults to ON. Error handling and `noexcept` changes
also have to build with exceptions off:

```bash
cmake -B build-noexcept -D SIMDJSON_EXCEPTIONS=OFF -D SIMDJSON_DEVELOPER_MODE=ON
cmake --build build-noexcept
ctest --test-dir build-noexcept --output-on-failure
```

Do not pass `-march=native`, `-mavx2`, `-march=armv8-a+sve`, or `/arch:AVX2`
when compiling the library. Kernels opt into an instruction set with target
attributes, and the runtime picks a kernel. A global ISA flag crashes on
older processors.

## Where to change code

- Public API and inline implementations: `include/simdjson/`.
- Non-inline implementations: `src/`.
- Tests: `tests/`. A bug fix comes with a test that failed before the fix.
- Benchmarks: `benchmark/`.
- CI: `.github/workflows/`.

`include/simdjson.h` and `src/simdjson.cpp` are the amalgamation spines.
Implementation kernels are `arm64`, `haswell`, `icelake`, `westmere`,
`ppc64`, `lasx`, `lsx`, `rvv-vls`, and `fallback`. Shared logic under
`include/simdjson/generic/` and `src/generic/` is compiled once per kernel.
A change there is multiplied.

Do not edit `singleheader/` by hand. Regenerate it only for a release:

```bash
cmake --build build --target amalgamate
```

Python 3 is required. The `amalgamate` target needs a prior build of the
same directory.

## Rules that are easy to break

- No printing to stdout or stderr in the core library. No `abort()` or `exit()`.
- `.h` and `.cpp` files are ASCII.
- Macros in public headers start with `SIMDJSON_` or `simdjson_`.
- No trailing whitespace. CI rejects it. `scripts/remove_trailing_whitespace.sh` strips it.
- Use `SIMDJSON_ASSUME`, not `assert`. Extra checks belong under `#if SIMDJSON_DEVELOPMENT_CHECKS`.
- Stage 1 reads up to one SIMD vector past the document, inside `SIMDJSON_PADDING` (64 bytes). Those bytes are not part of the JSON. `padded_string` zeroes them. A reused buffer may hold anything.
- Code that throws sits behind `#if SIMDJSON_EXCEPTIONS`. The error-code API is the one that always exists.
- Do not include `<asm/hwcap.h>`. Read CPU feature bits from `<sys/auxv.h>` and keep local fallbacks for the constants.
- On GCC 12, `"literal" + std::string(...)` can fail a Release build with a false `-Werror=restrict`. Build the string with `append` or `reserve`.
- `<windows.h>` defines `max` and `min` as function-like macros. Write `(std::numeric_limits<T>::max)()`, `(std::max)(a, b)`, and `(std::min)(a, b)`, in headers, sources, tests, and benchmarks. A bare `std::numeric_limits<T>::max()` breaks the Windows CI and any user who includes `<windows.h>` without `NOMINMAX`.
- Comments do not use contractions. Comment a non-obvious algorithm. Do not narrate the next line.
- Functions and variables are snake_case. Compile-time constants are `UPPER_SNAKE`.

## Do not use value_unsafe()

`simdjson_result<T>::value_unsafe()` returns the stored value and does not look at the error. On failure that value is a default or a partial result. Do not call it in new code, tests, examples, or benchmarks. Do not "check `error()` and then call `value_unsafe()`".

Take the value with `get`. It writes the output only on success and returns the `error_code`:

```cpp
T value;
auto error = result.get(value);
if (error) {
  // handle the error and do not use value
}
```

On the generic `simdjson_result<T>`, `get` is an rvalue method, so a named result is moved: `std::move(result).get(value)`. A temporary binds directly, as in `doc["foo"].get(value)`. The DOM element and On-Demand specializations also accept an lvalue.

`value()` throws `simdjson_error` and is compiled only when `SIMDJSON_EXCEPTIONS` is on. Library code and tests that must build with exceptions off use `get`.

Inside a function that returns `error_code`, propagate with `SIMDJSON_TRY` or by returning the code from `get`. Do not unpack the result with `value_unsafe()`.

## What not to change

Leave code that the task does not require alone. Do not reformat, rename, or
refactor it. A static-analyzer warning is not a reason to change the code
unless you can explain the bug.

Do not add a dependency. Do not use a C++ feature newer than C++11 in the
core library unless it is gated and the gain is performance or a real
portability fix. Portability to old compilers is a requirement.

A performance change needs before/after numbers from a Release build. The
usual check is:

```bash
cmake -B build -D SIMDJSON_DEVELOPER_MODE=ON
cmake --build build --config Release
build/benchmark/dom/parse jsonexamples/twitter.json
```

See [HACKING.md](HACKING.md) for the other benchmarks. Do not claim a speedup
without the numbers.

## Pull requests

State the benefit in the description: a bug fix, a measured speedup, a
feature, or a concrete maintainability gain. Keep the diff small. New
behavior needs a test. Link the issue when there is one.

`[skip ci]` belongs only on documentation or comment-only changes, in both
the pull request title and the commit subject.

Commit only the files the change needs. Leave build directories and local
notes untracked.

[AI_USAGE_POLICY.md](AI_USAGE_POLICY.md) requires a human in the loop. An
agent may draft the change. A person reviews the diff, can explain it, and
only then asks a maintainer to review it.
