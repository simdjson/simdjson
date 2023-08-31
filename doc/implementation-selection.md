CPU Architecture-Specific Implementations
=========================================

* [Overview](#overview)
* [Runtime CPU Detection](#runtime-cpu-detection)
* [Inspecting the Detected Implementation](#inspecting-the-detected-implementation)
* [Querying Available Implementations](#querying-available-implementations)
* [Manually Selecting the Implementation](#manually-selecting-the-implementation)
* [Checking that an Implementation can Run on your System](#checking-that-an-implementation-can-run-on-your-system)

Overview
--------

The simdjson library takes advantage of SIMD instruction sets such as NEON, SSE and AVX to achieve
much of its speed. Because these instruction sets work differently, simdjson has to compile a
different version of the JSON parser for different CPU architectures, often with different
algorithms to take better advantage of a given CPU!

The current implementations are:
* icelake: AVX-512F, AVX-512VBMI, etc.
* haswell: AVX2 (2013 Intel Haswell or later)
* westmere: SSE4.2 (2010 Westmere or later).
* arm64: 64-bit ARMv8-A NEON
* ppc64: 64-bit POWER8 and POWER9 with VSX and ALTIVEC extensions. Both big endian and little endian are implemented, depends on the compiler you are using. The library is tested on recent, little-endian, POWER systems.
* fallback: A generic implementation that runs on any 64-bit processor.

In many cases, you don't know where your compiled binary is going to run, so simdjson automatically
compiles *all* the implementations into the executable. On Intel, it will include 4 implementations
(icelake, haswell, westmere and fallback), on ARM it will include 2 (arm64 and fallback), and on PPC
it will include 2 (ppc64 and fallback).

If you know more about where you're going to run and want to save the space, you can disable any of
these implementations at compile time with `-DSIMDJSON_IMPLEMENTATION_X=0` (where X is ICELAKE, HASWELL,
WESTMERE, ARM64, PPC64 and FALLBACK).

The simdjson library automatically sets header flags for each implementation as it compiles; there
is no need to set architecture-specific flags yourself (e.g., `-mavx2`, `/AVX2`  or
`-march=haswell`), and it may even break runtime dispatch and your binaries will fail to run on
older processors. _Note:_ for POWER9 processors make sure you compile it with `-mcpu=power9` and `-mtune=power9` to
get maximum performance.

Runtime CPU Detection
---------------------

When you first use simdjson, it will detect the CPU you're running on, and swap over to the fastest
implementation for it. This is a small, one-time cost and for many people will be paid the first
time they call `parse()` or `load()`.

Inspecting the Detected Implementation
--------------------------------------

You can check what implementation is running with `active_implementation`:

```c++
cout << "simdjson v" << SIMDJSON_VERSION << endl;
cout << "Detected the best implementation for your machine: " << simdjson::get_active_implementation()->name();
cout << "(" << simdjson::get_active_implementation()->description() << ")" << endl;
```

Implementation detection will happen in this case when you first call `name()`.

Querying Available Implementations
----------------------------------

You can list all available implementations, regardless of which one was selected:

```c++
for (auto implementation : simdjson::get_available_implementations()) {
  cout << implementation->name() << ": " << implementation->description() << endl;
}
```

And look them up by name:

```c++
cout << simdjson::get_available_implementations()["fallback"]->description() << endl;
```
Though the fallback implementation should always be available, others might be missing. When
an implementation is not available, the bracket call `simdjson::get_available_implementations()[name]`
will return the null pointer.

The available implementations have been compiled but may not necessarily be run safely on your system
see [Checking that an Implementation can Run on your System](#checking-that-an-implementation-can-run-on-your-system).



Manually Selecting the Implementation
-------------------------------------

If you're trying to do performance tests or see how different implementations of simdjson run, you
can select the CPU architecture yourself:

```c++
// Use the fallback implementation, even though my machine is fast enough for anything
simdjson::get_active_implementation() = simdjson::get_available_implementations()["fallback"];
```

You are responsible for ensuring that the requirements of the selected implementation match your current system.
Furthermore, you should check that the implementation is available before setting it to `simdjson::get_active_implementation()`
by comparing it with the null pointer.

```c++
auto my_implementation = simdjson::get_available_implementations()["haswell"];
if (! my_implementation) { exit(1); }
if (! my_implementation->supported_by_runtime_system()) { exit(1); }
simdjson::get_active_implementation() = my_implementation;
```

Checking that an Implementation can Run on your System
-------------------------------------

You should call `supported_by_runtime_system()` to compare the processor's features with the need of the implementation.

```c++
for (auto implementation : simdjson::get_available_implementations()) {
  if (implementation->supported_by_runtime_system()) {
    cout << implementation->name() << ": " << implementation->description() << endl;
  }
}
```

The call to `supported_by_runtime_system()` may be relatively expensive. Do not call  `supported_by_runtime_system()` each
time you parse a JSON input (for example). It is meant to be called a handful of times at most in the life of a program.
