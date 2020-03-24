[![Build Status](https://cloud.drone.io/api/badges/simdjson/simdjson/status.svg)](https://cloud.drone.io/simdjson/simdjson)
[![CircleCI](https://circleci.com/gh/simdjson/simdjson.svg?style=svg)](https://circleci.com/gh/simdjson/simdjson)
[![Fuzzing Status](https://oss-fuzz-build-logs.storage.googleapis.com/badges/simdjson.svg)](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-opened&q=proj%3Asimdjson&can=2)
[![Build status](https://ci.appveyor.com/api/projects/status/ae77wp5v3lebmu6n/branch/master?svg=true)](https://ci.appveyor.com/project/lemire/simdjson-jmmti/branch/master)
[![][license img]][license]

# simdjson : Parsing gigabytes of JSON per second

<img src="images/logo.png" width="10%" style="float: right">
JSON is everywhere on the Internet. Servers spend a *lot* of time parsing it. We need a fresh
approach. The simdjson libraryuses commonly available SIMD instructions and microparallel algorithms
to parse JSON 2.5x faster than anything else out there.

* **Fast:** Over 2.5x faster than other production-grade JSON parsers.
* **Easy:** First-class, easy to use API.
* **Strict:** Full JSON and UTF-8 validation, lossless parsing. Performance with no compromises.
* **Automatic:** Selects a CPU-tailored parser at runtime. No configuration needed.
* **Reliable:** From memory allocation to error handling, simdjson's design avoids surprises.

This library is part of the [Awesome Modern C++](https://awesomecpp.com) list.

## Table of Contents

* [Quick Start](#quick-start)
* [About simdjson](#about-simdjson)
    * [Performance results](#performance-results)
    * [Real-world usage](#real-world-usage)
* [Code usage and examples](#code-usage-and-examples)
    * [Including simdjson](#including-simdjson)
    * [The Basics: Loading and Parsing JSON Documents](#the-basics-loading-and-parsing-json-documents)
    * [Using the Parsed JSON](#using-the-parsed-json)
    * [JSON Pointer](#json-pointer)
    * [Error Handling](#error-handling)
      * [Error Handling Example](#error-handling-example)
      * [Exceptions](#exceptions)
    * [Newline-Delimited JSON (ndjson) and JSON lines](#newline-delimited-json-ndjson-and-json-lines)
    * [Thread safety](#thread-safety)
* [Advanced Usage](#advanced-usage)
    * [Reusing the parser for maximum efficiency](#reusing-the-parser-for-maximum-efficiency)
    * [Keeping documents around for longer](#keeping-documents-around-for-longer)
    * [Server Loops: Long-Running Processes and Memory Capacity](#server-loops-long-running-processes-and-memory-capacity)
    * [Large files and huge page support](#large-files-and-huge-page-support)
    * [CPU Architecture-Specific Implementations](#cpu-architecture-specific-implementations)
      * [Runtime CPU Detection](#runtime-cpu-detection)
      * [Querying Available Implementations](#querying-available-implementations)
      * [Manually Selecting the Parser Implementation](#manually-selecting-the-parser-implementation)
    * [Computed GOTOs](#computed-gotos)
* [Detailed Build Instructions](#detailed-build-instructions)
    * [Usage: easy single-header version](#usage-easy-single-header-version)
    * [Usage (old-school Makefile on platforms like Linux or macOS)](#usage-old-school-makefile-on-platforms-like-linux-or-macos)
    * [Usage (CMake on 64-bit platforms like Linux or macOS)](#usage-cmake-on-64-bit-platforms-like-linux-or-macos)
    * [Usage (CMake on 64-bit Windows using Visual Studio)](#usage-cmake-on-64-bit-windows-using-visual-studio)
    * [Usage (Using vcpkg on 64-bit Windows, Linux and macOS)](#usage-using-vcpkg-on-64-bit-windows-linux-and-macos)
    * [Usage (Docker)](#usage-docker)
* [Architecture and Design Notes](#architecture-and-design-notes)
    * [Requirements](#requirements)
    * [Scope](#scope)
    * [Features](#features)
    * [Architecture](#architecture)
    * [Remarks on JSON parsing](#remarks-on-json-parsing)
    * [Pseudo-structural elements](#pseudo-structural-elements)
* [About the Project](#about-the-project)
    * [Bindings and Ports of simdjson](#bindings-and-ports-of-simdjson)
    * [Tools](#tools)
    * [In-depth comparisons](#in-depth-comparisons)
    * [Various References](#various-references)
    * [Academic References](#academic-references)
    * [Funding](#funding)
    * [License](#license)

## Quick Start

The simdjson libraryis easily consumable with a single .h and .cpp file.

0. Prerequisites: `g++` or `clang++`.
1. Pull [simdjson.h](singleheader/simdjson.h) and [simdjson.cpp](singleheader/simdjson.cpp) into a directory, along with the sample file [twitter.json](jsonexamples/twitter.json).
   ```
   wget https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.h https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.cpp https://raw.githubusercontent.com/simdjson/simdjson/master/jsonexamples/twitter.json
   ```
2. Create `parser.cpp`:

   ```c++
   #include "simdjson.h"
   int main(void) {
     simdjson::document::parser parser;
     simdjson::document& tweets = parser.load("twitter.json");
     std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;
   }
   ```
3. `c++ -o parser parser.cpp simdjson.cpp -std=c++17`
4. `./parser`
   ```
   100 results.
   ```

## About simdjson

The simdjson librarytakes advantage of modern microarchitectures, parallelizing with SIMD vector
instructions, reducing branch misprediction, and reducing data dependency to take advantage of each
CPU's multiple execution cores.

Some people [enjoy reading our paper](https://arxiv.org/abs/1902.08318): A description of the design
and implementation of simdjson is in our research article in VLDB journal: Geoff Langdale, Daniel
Lemire, [Parsing Gigabytes of JSON per Second](https://arxiv.org/abs/1902.08318), VLDB Journal 28 (6), 2019appear)

We also have an informal [blog post providing some background and context](https://branchfree.org/2019/02/25/paper-parsing-gigabytes-of-json-per-second/).

For the video inclined, [![simdjson at QCon San Francisco 2019](http://img.youtube.com/vi/wlvKAT7SZIQ/0.jpg)](http://www.youtube.com/watch?v=wlvKAT7SZIQ)
was the best voted talk.

### Performance results

The simdjson libraryuses three-quarters less instructions than state-of-the-art parser RapidJSON and
fifty percent less than sajson. To our knowledge, simdjson is the first fully-validating JSON parser
to run at gigabytes per second on commodity processors.

<img src="doc/gbps.png" width="90%">

On a Skylake processor, the parsing speeds (in GB/s) of various processors on the twitter.json file
are as follows.

| parser                                | GB/s |
| ------------------------------------- | ---- |
| simdjson                              | 2.2  |
| RapidJSON encoding-validation         | 0.51 |
| RapidJSON encoding-validation, insitu | 0.71 |
| sajson (insitu, dynamic)              | 0.70 |
| sajson (insitu, static)               | 0.97 |
| dropbox                               | 0.14 |
| fastjson                              | 0.26 |
| gason                                 | 0.85 |
| ultrajson                             | 0.42 |
| jsmn                                  | 0.28 |
| cJSON                                 | 0.34 |
| JSON for Modern C++ (nlohmann/json)   | 0.10 |

### Real-world usage

- [Microsoft FishStore](https://github.com/microsoft/FishStore)
- [Yandex ClickHouse](https://github.com/yandex/ClickHouse)
- [Clang Build Analyzer](https://github.com/aras-p/ClangBuildAnalyzer)

If you are planning to use simdjson in a product, please work from one of our releases.

## Code usage and examples

### Including simdjson

To include simdjson, copy [simdjson.h](singleheader/simdjson.h) and [simdjson.cpp](singleheader/simdjson.cpp)
into your project. Then include it in your project with:

```c++
#include "simdjson.h"
using namespace simdjson; // optional
```

You can compile with:

```
c++ myproject.cpp simdjson.cpp --std=c++17
```

### The Basics: Loading and Parsing JSON Documents

The simdjson library offers a simple DOM tree API, which you can access by creating a
`document::parser` and calling the `load()` method:

```c++
document::parser parser;
document &doc = parser.load(filename); // load and parse a file
```

Or by creating a padded string (for efficiency reasons, simdjson requires a string with
SIMDJSON_PADDING bytes at the end) and calling `parse()`:

```c++
document::parser parser;
document &doc = parser.parse("[1,2,3]"_padded); // parse a string
```

### Using the Parsed JSON

Once you have a document, you can navigate it with idiomatic C++ iterators, operators and casts.

* **Document Root:** To get the top level JSON element, get `doc.root()`. Many of the
  methods below will work on the document object itself, as well.
* **Extracting Values:** You can cast a JSON element to a native type: `double(element)` or
  `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  document::object and document::array. You can also use is_*typename*()` to test if it is a
  given type, and as_*typename*() to do the cast and return an error code on failure instead of an
  exception.
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`.
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. If you
  know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`
* **Object Iteration:** You can iterate through an object's fields, too: `for (auto [key, value] : object)`

Here are some examples of all of the above:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
document::parser parser;
document::array cars = parser.parse(cars_json).as_array();

// Iterating through an array of objects
for (document::object car : cars) {
  // Accessing a field by name
  cout << "Make/Model: " << car["make"] << "/" << car["model"] << endl;

  // Casting a JSON element to an integer
  uint64_t year = car["year"];
  cout << "- This car is " << 2020 - year << "years old." << endl;

  // Iterating through an array of floats
  double total_tire_pressure = 0;
  for (double tire_pressure : car["tire_pressure"]) {
    total_tire_pressure += tire_pressure;
  }
  cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

  // Writing out all the information about the car
  for (auto [key, value] : car) {
    cout << "- " << key << ": " << value << endl;
  }
}
```

### JSON Pointer

The simdjson libraryalso supports JSON pointer, letting you reach further down into the document:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
document::parser parser;
document &cars = parser.parse(cars_json);
cout << cars["/0/tire_pressure/1"] << endl; // Prints 39.9
```

### Error Handling

All simdjson APIs that can fail return `simdjson_result<T>`, which is a &lt;value, error_code&gt;
pair. The error codes and values can be accessed directly, reading the error like so:

```c++
auto [doc, error] = parser.parse(json); // doc is a document&
if (error) { cerr << error << endl; exit(1); }
// Use document here now that we've checked for the error
```

When you use the code this way, it is your responsibility to check for error before using the
result: if there is an error, the result value will not be valid and using it will caused undefined
behavior.

> Note: because of the way `auto [x, y]` works in C++, you have to define new variables each time you
> use it. If your project treats aliased, this means you can't use the same names in `auto [x, error]`
> without triggering warnings or error (and particularly can't use the word "error" every time). To
> circumvent this, you can use this instead:
> 
> ```c++
> document &doc;
> error_code error;
> parser.parse(json).tie(doc, error); // <-- Assigns to doc and error just like "auto [doc, error]"
> ```

#### Error Handling Example

This is how the example in "Using the Parsed JSON" could be written using only error code checking:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
document::parser parser;
auto [doc, error] = parser.parse(cars_json);
if (error) { cerr << error << endl; exit(1); }

// Iterating through an array of objects
for (document::element car_element : cars) {
  document::object car;
  car_element.as_object().tie(car, error);
  if (error) { cerr << error << endl; exit(1); }

  // Accessing a field by name
  document::element make, model;
  car["make"].tie(make, error);
  if (error) { cerr << error << endl; exit(1); }
  car["model"].tie(model, error);
  if (error) { cerr << error << endl; exit(1); }
  cout << "Make/Model: " << make << "/" << model << endl;

  // Casting a JSON element to an integer
  uint64_t year;
  car["year"].as_uint64_t().tie(year, error);
  if (error) { cerr << error << endl; exit(1); }
  cout << "- This car is " << 2020 - year << "years old." << endl;

  // Iterating through an array of floats
  double total_tire_pressure = 0;
  for (document::element tire_pressure_element : car["tire_pressure"]) {
    double tire_pressure;
    tire_pressure_element.as_uint64_t().tie(tire_pressure, error);
    if (error) { cerr << error << endl; exit(1); }
    total_tire_pressure += tire_pressure;
  }
  cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

  // Writing out all the information about the car
  for (auto [key, value] : car) {
    cout << "- " << key << ": " << value << endl;
  }
}
```

#### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```c++
document &doc = parser.parse(json); // Throws an exception if there was an error!
```

When used this way, a `simdjson_error` exception will be thrown if an error occurs, preventing the
program from continuing if there was an error.

### Newline-Delimited JSON (ndjson) and JSON lines

The simdjson library also support multithreaded JSON streaming through a large file containing many smaller JSON documents in either [ndjson](http://ndjson.org) or [JSON lines](http://jsonlines.org) format. If your JSON documents all contain arrays or objects, we even support direct file concatenation without whitespace. The concatenated file has no size restrictions (including larger than 4GB), though each individual document must be less than 4GB.

Here is a simple example:

```cpp
auto ndjson = R"(
{ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 }
)"_padded;
document::parser parser;
for (document &doc : parser.load_many(filename)) {
  cout << doc["foo"] << endl;
}
// Prints 1 2 3
```

In-memory ndjson strings can be parsed as well, with `parser.parse_many(string)`.

### Thread safety

The simdjson library is mostly single-threaded. Thread safety is the responsibility of the caller:
it is unsafe to reuse a document::parser object between different threads.

simdjson's CPU detection, which runs the first time parsing is attempted and switches to the fastest
parser for your CPU, is transparent and thread-safe.

The json stream parser is threaded, using a second thread under its own control. Like the single
document parser

## Advanced Usage

### Reusing the parser for maximum efficiency

If you're using simdjson to parse multiple documents, or in a loop, you should make a parser once
and reuse it. The simdjson library will allocate and retain internal buffers between parses, keeping buffers
hot in cache and keeping allocation to a minimum.

```c++
document::parser parser;
for (padded_string json : { string("[1, 2, 3]"), string("true"), string("[ true, false ]") }) {
  document& doc = parser.parse(json);
  cout << doc;
}
```

It's not just internal buffers though. The simdjson library reuses the document itself. Notice that reference?
`document &doc`? That's key. You are only *borrowing* the document from simdjson, which purposely
reuses and overwrites it each time you call parse. This prevent wasteful and unnecessary memory
allocation in 99% of cases where JSON is just read, used, and converted to native values
or thrown away.

> **You are only borrowing the document from the simdjson parser. Don't keep it long term!**

This is key: don't keep the `document&`, `document::element`, `document::array`, `document::object`
or `string_view` objects you get back from the API. Convert them to C++ native values, structs and
arrays that you own.

### Keeping documents around for longer

If you really need to keep parsed JSON documents around for a long time, you can **take** the
document by declaring an actual `document` value.

```c++
document doc = parser.parse(json); // "steals" the document from the parser
```

If you're using error codes, it can be done like this:

```c++
auto [doc_ref, error] = parser.parse(json); // doc_ref is a document&
document doc = doc_ref; // "steal" the document from the parser
```

This won't allocate anything or copy document memory: instead, it will *steal* the document memory
from the parser. The parser will simply allocate new document memory the next time you call parse.

### Server Loops: Long-Running Processes and Memory Capacity

The simdjson library automatically expands its memory capacity when larger documents are parsed, so
that you don't unexpectedly fail. In a short process that reads a bunch of files and then exits,
this works pretty flawlessly.

Server loops, though, are long-running processes that will keep the parser around forever. This
means that if you encounter a really, really large document, simdjson will not resize back down.
The simdjson library lets you adjust your allocation strategy to prevent your server from growing
without bound:

* You can set a *max capacity* when constructing a parser:

  ```c++
  document::parser parser(1024*1024); // Never grow past documents > 1MB
  for (web_request request : listen()) {
    auto [doc, error] = parser.parse(request.body);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { request.respond(413); continue; }
    // ...
  }
  ```

  This parser will grow normally as it encounters larger documents, but will never pass 1MB.

* You can set a *fixed capacity* that never grows, as well, which can be excellent for
  predictability and reliability, since simdjson will never call malloc after startup!

  ```c++
  document::parser parser(0); // This parser will refuse to automatically grow capacity
  parser.set_capacity(1024*1024); // This allocates enough capacity to handle documents <= 1MB
  for (web_request request : listen()) {
    auto [doc, error] = parser.parse(request.body);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { request.respond(413); continue; }
    // ...
  }
  ```

### Large files and huge page support

There is a memory allocation performance cost the first time you process a large file (e.g. 100MB).
Between the cost of allocation, the fact that the memory is not in cache, and the initial zeroing of
memory, [on some systems, allocation runs far slower than parsing (e.g., 1.4GB/s)](https://lemire.me/blog/2020/01/14/how-fast-can-you-allocate-a-large-block-of-memory-in-c/). Reusing the parser mitigates this by
paying the cost once, but does not eliminate it.

In large file use cases, enabling transparent huge page allocation on the OS can help a lot. We
haven't found the right way to do this on Windows or OS/X, but on Linux, you can enable transparent
huge page allocation with a command like:

```
echo always > /sys/kernel/mm/transparent_hugepage/enabled
```

In general, when running benchmarks over large files, we recommend that you report performance numbers with and without huge pages if possible. Furthermore, you should amortize the parsing (e.g., by parsing several large files) to distinguish the time spent parsing from the time spent allocating memory.

### CPU Architecture-Specific Implementations

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

#### Runtime CPU Detection

When you first use simdjson, it will detect the CPU you're running on, and swap over to the fastest
implementation for it. This is a small, one-time cost and for many people will be paid the first
time they call `parse()` or `load()`.

You can check what implementation is running with `active_implementation`:

```c++
cout << "simdjson v" << #SIMDJSON_VERSION << endl;
cout << "Detected the best implementation for your machine: " << simdjson::active_implementation->name();
cout << "(" << simdjson::active_implementation->description() << ")" << endl;
```

Implementation detection will happen in this case when you first call `name()`.

#### Querying Available Implementations

You can list all available implementations:

```c++
for (auto implementation : simdjson::available_implementations) {
  cout << implementation->name() << ": " << implementation->description() << endl;
}
```

And look them up by name:

```c++
cout << simdjson::available_implementations["fallback"]->description() << endl;
```

#### Manually Selecting the Parser Implementation

If you're trying to do performance tests or see how different implementations of simdjson run, you
can select the CPU architecture yourself:

```c++
// Use the fallback implementation, even though my machine is fast enough for anything
simdjson::active_implementation = simdjson::available_implementations["fallback"];
```

### Computed GOTOs

For best performance, we use a technique called "computed goto" when the compiler supports it, it is also sometimes described as "Labels as Values".
Though it is not part of the C++ standard, it is supported by many major compilers and it brings measurable performance benefits that
are difficult to achieve otherwise.
The computed gotos are  automatically disabled under Visual Studio.
If you wish to forcefully disable computed gotos, you can do so by compiling the code with the macro `SIMDJSON_NO_COMPUTED_GOTO`
defined. It is not recommended to disable computed gotos if your compiler supports it. In fact, you should almost never need to
be concerned with computed gotos.

## Detailed Build Instructions

### Usage: easy single-header version

See the [singleheader](singleheader) directory for a single header version. See the included
file "amalgamation_demo.cpp" for usage. This requires no specific build system: just
copy the files in your project in your include path. You can then include them quite simply:

```c++
#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
using namespace simdjson;
int main(int argc, char *argv[]) {
  document::parser parser;
  auto [doc, error] = parser.load(argv[1]);
  if(error) {
    std::cout << "not valid: " << error << std::endl;
  } else {
    std::cout << "valid" << std::endl;
  }
  return EXIT_SUCCESS;
}
```

Note: In some settings, it might be desirable to precompile `simdjson.cpp` instead of including it.

### Usage (old-school Makefile on platforms like Linux or macOS)

Requirements: recent clang or gcc, and make. We recommend at least GNU GCC/G++ 7 or LLVM clang 6. A 64-bit system like Linux or macOS is expected.

To test:

```
make
make test
```

To run benchmarks:

```
make parse
./parse jsonexamples/twitter.json
```

Under Linux, the `parse` command gives a detailed analysis of the performance counters.

To run comparative benchmarks (with other parsers):

```
make benchmark
```

### Usage (CMake on 64-bit platforms like Linux or macOS)

Requirements: We require a recent version of cmake. On macOS, the easiest way to install cmake might be to use [brew](https://brew.sh) and then type

```
brew install cmake
```

There is an [equivalent brew on Linux which works the same way as well](https://linuxbrew.sh).

You need a recent compiler like clang or gcc. We recommend at least GNU GCC/G++ 7 or LLVM clang 6. For example, you can install a recent compiler with brew:

```
brew install gcc@8
```

Optional: You need to tell cmake which compiler you wish to use by setting the CC and CXX variables. Under bash, you can do so with commands such as `export CC=gcc-7` and `export CXX=g++-7`.

Building: While in the project repository, do the following:

```
mkdir build
cd build
cmake ..
make
make test
```

CMake will build a library. By default, it builds a shared library (e.g., libsimdjson.so on Linux).

You can build a static library:

```
mkdir buildstatic
cd buildstatic
cmake -DSIMDJSON_BUILD_STATIC=ON ..
make
make test
```

In some cases, you may want to specify your compiler, especially if the default compiler on your system is too old. You may proceed as follows:

```
brew install gcc@8
mkdir build
cd build
export CXX=g++-8 CC=gcc-8
cmake ..
make
make test
```

### Usage (CMake on 64-bit Windows using Visual Studio)

We assume you have a common 64-bit Windows PC with at least Visual Studio 2017 and an x64 processor with AVX2 support (2013 Intel Haswell or later) or SSE 4.2 + CLMUL (2010 Westmere or later).

- Grab the simdjson code from GitHub, e.g., by cloning it using [GitHub Desktop](https://desktop.github.com/).
- Install [CMake](https://cmake.org/download/). When you install it, make sure to ask that `cmake` be made available from the command line. Please choose a recent version of cmake.
- Create a subdirectory within simdjson, such as `VisualStudio`.
- Using a shell, go to this newly created directory.
- Type `cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..` in the shell while in the `VisualStudio` repository. (Alternatively, if you want to build a DLL, you may use the command line `cmake -DCMAKE_GENERATOR_PLATFORM=x64 -DSIMDJSON_BUILD_STATIC=OFF ..`.)
- This last command (`cmake ...`) created a Visual Studio solution file in the newly created directory (e.g., `simdjson.sln`). Open this file in Visual Studio. You should now be able to build the project and run the tests. For example, in the `Solution Explorer` window (available from the `View` menu), right-click `ALL_BUILD` and select `Build`. To test the code, still in the `Solution Explorer` window, select `RUN_TESTS` and select `Build`.

### Usage (Using `vcpkg` on 64-bit Windows, Linux and macOS)

[vcpkg](https://github.com/Microsoft/vcpkg) users on Windows, Linux and macOS can download and install `simdjson` with one single command from their favorite shell.

On 64-bit Linux and macOS:

```
$ ./vcpkg install simdjson
```

will build and install `simdjson` as a static library.

On Windows (64-bit):

```
.\vcpkg.exe install simdjson:x64-windows
```

will build and install `simdjson` as a shared library.

```
.\vcpkg.exe install simdjson:x64-windows-static  
```

will build and install `simdjson` as a static library.

These commands will also print out instructions on how to use the library from MSBuild or CMake-based projects.

If you find the version of `simdjson` shipped with `vcpkg` is out-of-date, feel free to report it to
`vcpkg` community either by submitting an issue or by creating a PR.

### Usage (Docker)

One can run tests and benchmarks using docker. It especially makes sense under Linux. Privileged
access may be needed to get performance counters.

```
git clone https://github.com/simdjson/simdjson.git
cd simdjson
docker build -t simdjson .
docker run --privileged -t simdjson
```

## Architecture and Design Notes

### Requirements

- 64-bit platforms like Linux or macOS, as well as Windows through Visual Studio 2017 or later.
- Any 64-bit processor:
  - AVX2 (i.e., Intel processors starting with the Haswell microarchitecture released 2013 and AMD
    processors starting with the Zen microarchitecture released 2017),
  - SSE 4.2 and CLMUL (i.e., Intel processors going back to Westmere released in 2010 or AMD
    processors starting with the Jaguar used in the PS4 and XBox One),
  - 64-bit ARM processor (ARMv8-A NEON): this covers a wide range of mobile processors, including
    all Apple processors currently available for sale, going as far back as the iPhone 5s (2013).
  - Any 64-bit processor (simdjson has a fallback generic 64-bit implementation that is still super
    fast).
- A recent C++ compiler (e.g., GNU GCC or LLVM CLANG or Visual Studio 2017), we assume C++17. GNU
  GCC 7 or better or LLVM's clang 6 or better.
- Some benchmark scripts assume bash and other common utilities, but they are optional.

### Scope

We provide a fast parser, that fully validates an input according to various specifications.
The parser builds a useful immutable (read-only) DOM (document-object model) which can be later accessed.

To simplify the engineering, we make some assumptions.

- We support UTF-8 (and thus ASCII), nothing else (no Latin, no UTF-16). We do not believe this is a
  genuine limitation, because we do not think there is any serious application that needs to process
  JSON data without an ASCII or UTF-8 encoding. If the UTF-8 contains a leading BOM, it should be
  omitted: the user is responsible for detecting and skipping the BOM; UTF-8 BOMs are discouraged.
- All strings in the JSON document may have up to 4294967295 bytes in UTF-8 (4GB). To enforce this
  constraint, we refuse to parse a document that contains more than 4294967295 bytes (4GB). This
  should accommodate most JSON documents.
- As allowed by the specification, we allow repeated keys within an object (other parsers like
  sajson do the same).
- [The simdjson library is fast for JSON documents spanning a few bytes up to many megabytes](https://github.com/lemire/simdjson/issues/312).

_We do not aim to provide a general-purpose JSON library._ A library like RapidJSON offers much more
than just parsing, it helps you generate JSON and offers various other convenient functions. We
merely parse the document. This may change in the future.

### Features

- The input string is unmodified. (Parsers like sajson and RapidJSON use the input string as a buffer.)
- We parse integers and floating-point numbers as separate types which allows us to support large signed 64-bit integers in [-9223372036854775808,9223372036854775808), like a Java `long` or a C/C++ `long long` and large unsigned integers up to the value 18446744073709551615. Among the parsers that differentiate between integers and floating-point numbers, not all support 64-bit integers. (For example, sajson rejects JSON files with integers larger than or equal to 2147483648. RapidJSON will parse a file containing an overly long integer like 18446744073709551616 as a floating-point number.) When we cannot represent exactly an integer as a signed or unsigned 64-bit value, we reject the JSON document.
- We support the full range of 64-bit floating-point numbers (binary64). The values range from ` std::numeric_limits<double>::lowest()`  to `std::numeric_limits<double>::max()`, so from -1.7976e308 all the way to 1.7975e308. Extreme values (less or equal to -1e308, greater or equal to 1e308) are rejected: we refuse to parse the input document.
- We test for accurate float parsing with a perfect accuracy (ULP 0). Many parsers offer only approximate floating parsing. For example, RapidJSON also offers the option of accurate float parsing (`kParseFullPrecisionFlag`) but it comes at a significant performance penalty compared to the default settings. By default, RapidJSON tolerates an error of 3 ULP.
- We do full UTF-8 validation as part of the parsing. (Parsers like fastjson, gason and dropbox json11 do not do UTF-8 validation. The sajson parser does incomplete UTF-8 validation, accepting code point
sequences like 0xb1 0x87.)
- We fully validate the numbers. (Parsers like gason and ultranjson will accept `[0e+]` as valid JSON.)
- We validate string content for unescaped characters. (Parsers like fastjson and ultrajson accept unescaped line breaks and tabs in strings.)
- We fully validate the white-space characters outside of the strings. Parsers like RapidJSON will accept JSON documents with null characters outside of strings.

### Architecture

The parser works in two stages:

- Stage 1. (Find marks) Identifies quickly structure elements, strings, and so forth. We validate UTF-8 encoding at that stage.
- Stage 2. (Structure building) Involves constructing a "tree" of sort (materialized as a tape) to navigate through the data. Strings and numbers are parsed at this stage.

### Remarks on JSON parsing

- The JSON spec defines what a JSON parser is:
  > A JSON parser transforms a JSON text into another representation. A JSON parser MUST accept all texts that conform to the JSON grammar. A JSON parser MAY accept non-JSON forms or extensions. An implementation may set limits on the size of texts that it accepts. An implementation may set limits on the maximum depth of nesting. An implementation may set limits on the range and precision of numbers. An implementation may set limits on the length and character contents of strings.

* JSON is not JavaScript:

  > All JSON is Javascript but NOT all Javascript is JSON. So {property:1} is invalid because property does not have double quotes around it. {'property':1} is also invalid, because it's single quoted while the only thing that can placate the JSON specification is double quoting. JSON is even fussy enough that {"property":.1} is invalid too, because you should have of course written {"property":0.1}. Also, don't even think about having comments or semicolons, you guessed it: they're invalid. (credit:https://github.com/elzr/vim-json)

* The structural characters are:

      begin-array     =  [ left square bracket
      begin-object    =  { left curly bracket
      end-array       =  ] right square bracket
      end-object      =  } right curly bracket
      name-separator  = : colon
      value-separator = , comma

### Pseudo-structural elements

A character is pseudo-structural if and only if:

1. Not enclosed in quotes, AND
2. Is a non-whitespace character, AND
3. Its preceding character is either:
   (a) a structural character, OR
   (b) whitespace.

This helps as we redefine some new characters as pseudo-structural such as the characters 1, G, n in the following:

> { "foo" : 1.5, "bar" : 1.5 GEOFF_IS_A_DUMMY bla bla , "baz", null }

## About the Project

### Bindings and Ports of simdjson

We distinguish between "bindings" (which just wrap the C++ code) and a port to another programming language (which reimplements everything).


- [ZippyJSON](https://github.com/michaeleisel/zippyjson): Swift bindings for the simdjson project.
- [pysimdjson](https://github.com/TkTech/pysimdjson): Python bindings for the simdjson project.
- [simdjson-rs](https://github.com/Licenser/simdjson-rs): Rust port.
- [simdjson-rust](https://github.com/SunDoge/simdjson-rust): Rust wrapper (bindings).
- [SimdJsonSharp](https://github.com/EgorBo/SimdJsonSharp): C# version for .NET Core (bindings and full port).
- [simdjson_nodejs](https://github.com/luizperes/simdjson_nodejs): Node.js bindings for the simdjson project.
- [simdjson_php](https://github.com/crazyxman/simdjson_php): PHP bindings for the simdjson project.
- [simdjson_ruby](https://github.com/saka1/simdjson_ruby): Ruby bindings for the simdjson project.
- [simdjson-go](https://github.com/minio/simdjson-go): Go port using Golang assembly.
- [rcppsimdjson](https://github.com/eddelbuettel/rcppsimdjson): R bindings.

### Tools

- `json2json mydoc.json` parses the document, constructs a model and then dumps back the result to standard output.
- `json2json -d mydoc.json` parses the document, constructs a model and then dumps model (as a tape) to standard output. The tape format is described in the accompanying file `tape.md`.
- `minify mydoc.json` minifies the JSON document, outputting the result to standard output. Minifying means to remove the unneeded white space characters.
- `jsonpointer mydoc.json <jsonpath> <jsonpath> ... <jsonpath>` parses the document, constructs a model and then processes a series of [JSON Pointer paths](https://tools.ietf.org/html/rfc6901). The result is itself a JSON document.

### In-depth comparisons

If you want to see how a wide range of parsers validate a given JSON file:

```
make allparserscheckfile
./allparserscheckfile myfile.json
```

For performance comparisons:

```
make parsingcompetition
./parsingcompetition myfile.json
```

For broader comparisons:

```
make allparsingcompetition
./allparsingcompetition myfile.json
```

Both the `parsingcompetition` and `allparsingcompetition` tools take a `-t` flag which produces
a table-oriented output that can be conveniently parsed by other tools.

### Various References

- [Google double-conv](https://github.com/google/double-conversion/)
- [How to implement atoi using SIMD?](https://stackoverflow.com/questions/35127060/how-to-implement-atoi-using-simd)
- [Parsing JSON is a Minefield 💣](http://seriot.ch/parsing_json.php)
- https://tools.ietf.org/html/rfc7159
- The Mison implementation in rust https://github.com/pikkr/pikkr
- http://rapidjson.org/md_doc_sax.html
- https://github.com/Geal/parser_benchmarks/tree/master/json
- Gron: A command line tool that makes JSON greppable https://news.ycombinator.com/item?id=16727665
- GoogleGson https://github.com/google/gson
- Jackson https://github.com/FasterXML/jackson
- https://www.yelp.com/dataset_challenge
- RapidJSON. http://rapidjson.org/

Inspiring links:

- https://auth0.com/blog/beating-json-performance-with-protobuf/
- https://gist.github.com/shijuvar/25ad7de9505232c87034b8359543404a
- https://github.com/frankmcsherry/blog/blob/master/posts/2018-02-11.md

Validating UTF-8 takes no more than 0.7 cycles per byte:

- https://github.com/lemire/fastvalidate-utf-8 https://lemire.me/blog/2018/05/16/validating-utf-8-strings-using-as-little-as-0-7-cycles-per-byte/

### Academic References

- T.Mühlbauer, W.Rödiger, R.Seilbeck, A.Reiser, A.Kemper, and T.Neumann. Instant loading for main memory databases. PVLDB, 6(14):1702–1713, 2013. (SIMD-based CSV parsing)
- Mytkowicz, Todd, Madanlal Musuvathi, and Wolfram Schulte. "Data-parallel finite-state machines." ACM SIGARCH Computer Architecture News. Vol. 42. No. 1. ACM, 2014.
- Lu, Yifan, et al. "Tree structured data processing on GPUs." Cloud Computing, Data Science & Engineering-Confluence, 2017 7th International Conference on. IEEE, 2017.
- Sidhu, Reetinder. "High throughput, tree automata based XML processing using FPGAs." Field-Programmable Technology (FPT), 2013 International Conference on. IEEE, 2013.
- Dai, Zefu, Nick Ni, and Jianwen Zhu. "A 1 cycle-per-byte XML parsing accelerator." Proceedings of the 18th annual ACM/SIGDA international symposium on Field programmable gate arrays. ACM, 2010.
- Lin, Dan, et al. "Parabix: Boosting the efficiency of text processing on commodity processors." High Performance Computer Architecture (HPCA), 2012 IEEE 18th International Symposium on. IEEE, 2012. http://parabix.costar.sfu.ca/export/1783/docs/HPCA2012/final_ieee/final.pdf
- Deshmukh, V. M., and G. R. Bamnote. "An empirical evaluation of optimization parameters in XML parsing for performance enhancement." Computer, Communication and Control (IC4), 2015 International Conference on. IEEE, 2015.
- Moussalli, Roger, et al. "Efficient XML Path Filtering Using GPUs." ADMS@ VLDB. 2011.
- Jianliang, Ma, et al. "Parallel speculative dom-based XML parser." High Performance Computing and Communication & 2012 IEEE 9th International Conference on Embedded Software and Systems (HPCC-ICESS), 2012 IEEE 14th International Conference on. IEEE, 2012.
- Li, Y., Katsipoulakis, N.R., Chandramouli, B., Goldstein, J. and Kossmann, D., 2017. Mison: a fast JSON parser for data analytics. Proceedings of the VLDB Endowment, 10(10), pp.1118-1129. http://www.vldb.org/pvldb/vol10/p1118-li.pdf
- Cameron, Robert D., et al. "Parallel scanning with bitstream addition: An xml case study." European Conference on Parallel Processing. Springer, Berlin, Heidelberg, 2011.
- Cameron, Robert D., Kenneth S. Herdy, and Dan Lin. "High performance XML parsing using parallel bit stream technology." Proceedings of the 2008 conference of the center for advanced studies on collaborative research: meeting of minds. ACM, 2008.
- Shah, Bhavik, et al. "A data parallel algorithm for XML DOM parsing." International XML Database Symposium. Springer, Berlin, Heidelberg, 2009.
- Cameron, Robert D., and Dan Lin. "Architectural support for SWAR text processing with parallel bit streams: the inductive doubling principle." ACM Sigplan Notices. Vol. 44. No. 3. ACM, 2009.
- Amagasa, Toshiyuki, Mana Seino, and Hiroyuki Kitagawa. "Energy-Efficient XML Stream Processing through Element-Skipping Parsing." Database and Expert Systems Applications (DEXA), 2013 24th International Workshop on. IEEE, 2013.
- Medforth, Nigel Woodland. "icXML: Accelerating Xerces-C 3.1. 1 using the Parabix Framework." (2013).
- Zhang, Qiang Scott. Embedding Parallel Bit Stream Technology Into Expat. Diss. Simon Fraser University, 2010.
- Cameron, Robert D., et al. "Fast Regular Expression Matching with Bit-parallel Data Streams."
- Lin, Dan. Bits filter: a high-performance multiple string pattern matching algorithm for malware detection. Diss. School of Computing Science-Simon Fraser University, 2010.
- Yang, Shiyang. Validation of XML Document Based on Parallel Bit Stream Technology. Diss. Applied Sciences: School of Computing Science, 2013.
- N. Nakasato, "Implementation of a parallel tree method on a GPU", Journal of Computational Science, vol. 3, no. 3, pp. 132-141, 2012.

### Funding

The work is supported by the Natural Sciences and Engineering Research Council of Canada under grant number RGPIN-2017-03910.

[license]: LICENSE
[license img]: https://img.shields.io/badge/License-Apache%202-blue.svg

### License

This code is made available under the Apache License 2.0.

Under Windows, we build some tools using the windows/dirent_portable.h file (which is outside our library code): it under the liberal (business-friendly) MIT license.
