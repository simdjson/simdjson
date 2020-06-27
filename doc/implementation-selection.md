CPU Architecture-Specific Implementations
=========================================

* [Overview](#overview)
* [Runtime CPU Detection](#runtime-cpu-detection)
* [Inspecting the Detected Implementation](#inspecting-the-detected-implementation)
* [Querying Available Implementations](#querying-available-implementations)
* [Manually Selecting the Implementation](#manually-selecting-the-implementation)

Overview
--------

The simdjson library takes advantage of SIMD instruction sets such as NEON, SSE and AVX to achieve
much of its speed. Because these instruction sets work differently, simdjson has to compile a
different version of the JSON parser for different CPU architectures, often with different
algorithms to take better advantage of a given CPU!

The current implementations are:
* haswell: AVX2 (2013 Intel Haswell or later)
* westmere: SSE4.2 (2010 Westmere or later).
* arm64: 64-bit ARMv8-A NEON
* fallback: A generic implementation that runs on any 64-bit processor.

In many cases, you don't know where your compiled binary is going to run, so simdjson automatically
compiles *all* the implementations into the executable. On Intel, it will include 3 implementations
(haswell, westmere and fallback), and on ARM it will include 2 (arm64 and fallback).

If you know more about where you're going to run and want to save the space, you can disable any of
these implementations at compile time with `-DSIMDJSON_IMPLEMENTATION_X=0` (where X is HASWELL,
WESTMERE, ARM64 and FALLBACK).

The simdjson library automatically sets header flags for each implementation as it compiles; there
is no need to set architecture-specific flags yourself (e.g., `-mavx2`, `/AVX2`  or
`-march=haswell`), and it may even break runtime dispatch and your binaries will fail to run on
older processors.

Runtime CPU Detection
---------------------

When you first use simdjson, it will detect the CPU you're running on, and swap over to the fastest
implementation for it. This is a small, one-time cost and for many people will be paid the first
time they call `parse()` or `load()`.

Inspecting the Detected Implementation
--------------------------------------

You can check what implementation is running with `active_implementation`:

```c++
cout << "simdjson v" << #SIMDJSON_VERSION << endl;
cout << "Detected the best implementation for your machine: " << simdjson::active_implementation->name();
cout << "(" << simdjson::active_implementation->description() << ")" << endl;
```

Implementation detection will happen in this case when you first call `name()`.

Querying Available Implementations
----------------------------------

You can list all available implementations, regardless of which one was selected:

```c++
for (auto implementation : simdjson::available_implementations) {
  cout << implementation->name() << ": " << implementation->description() << endl;
}
```

And look them up by name:

```c++
cout << simdjson::available_implementations["fallback"]->description() << endl;
```

Manually Selecting the Implementation
-------------------------------------

If you're trying to do performance tests or see how different implementations of simdjson run, you
can select the CPU architecture yourself:

```c++
// Use the fallback implementation, even though my machine is fast enough for anything
simdjson::active_implementation = simdjson::available_implementations["fallback"];
```
