
Hacking simdjson
================

Here is wisdom about how to build, test and run simdjson from within the repository. This is mostly useful for people who plan to contribute simdjson, or maybe study the design.

If you plan to contribute to simdjson, please read our [CONTRIBUTING](https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md) guide.

- [Hacking simdjson](#hacking-simdjson)
  - [Build Quickstart](#build-quickstart)
  - [Design notes](#design-notes)
  - [Developer mode](#developer-mode)
  - [Directory Structure and Source](#directory-structure-and-source)
  - [Runtime Dispatching](#runtime-dispatching)
  - [Regenerating Single-Header Files](#regenerating-single-header-files)
  - [Usage (CMake on 64-bit platforms like Linux, FreeBSD or macOS)](#usage-cmake-on-64-bit-platforms-like-linux-freebsd-or-macos)
  - [Usage (CMake on 64-bit Windows using Visual Studio 2019 or better)](#usage-cmake-on-64-bit-windows-using-visual-studio-2019-or-better)
  - [Various References](#various-references)

Build Quickstart
------------------------------

```bash
mkdir build
cd build
cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
cmake --build .
```

Design notes
------------------------------

The parser works in two stages:

- Stage 1. (Find marks) Identifies quickly structure elements, strings, and so forth. We validate UTF-8 encoding at that stage.
- Stage 2. (Structure building) Involves constructing a "tree" of sort (materialized as a tape) to navigate through the data. Strings and numbers are parsed at this stage.


The role of stage 1 is to identify pseudo-structural characters as quickly as possible. A character is pseudo-structural if and only if:

1. Not enclosed in quotes, AND
2. Is a non-whitespace character, AND
3. Its preceding character is either:
   (a) a structural character, OR
   (b) whitespace OR
   (c) the final quote in a string.

This helps as we redefine some new characters as pseudo-structural such as the characters 1, G, n in the following:

> { "foo" : 1.5, "bar" : 1.5 GEOFF_IS_A_DUMMY bla bla , "baz", null }

Stage 1 also does unicode validation.

Stage 2 handles all of the rest: number parsings, recognizing atoms like true, false, null, and so forth.

Developer mode
--------------

Build system targets that are only useful for developers of the simdjson
library are behind the `SIMDJSON_DEVELOPER_MODE` option. Enabling this option
makes tests, examples, benchmarks and other developer targets available. Not
enabling this option means that you are a consumer of simdjson and thus you
only get the library targets and options.

Developer mode is forced to be on when the `CI` environment variable is set to
a value that CMake recognizes as "on", which is set to `true` in all of the CI
workflows used by simdjson.

Directory Structure and Source
------------------------------

simdjson's source structure, from the top level, looks like this:

* **CMakeLists.txt:** The main build system.
* **include:** User-facing declarations and inline definitions (most user-facing functions are inlined).
  * simdjson.h: the `simdjson` namespace. A "main include" that includes files from include/simdjson/. This is equivalent to
    the distributed simdjson.h.
    * simdjson/*.h: Declarations for public simdjson classes and functions.
    * simdjson/*-inl.h: Definitions for public simdjson classes and functions.
    * simdjson/internal/*.h: the `simdjson::internal` namespace. Private classes and functions used by the rest of simdjson.
  * simdjson/dom.h: the `simdjson::dom` namespace. Includes all public DOM classes.
    * simdjson/dom/*.h: Declarations/definitions for individual DOM classes.
  * simdjson/arm64|fallback|haswell|icelake|ppc64|westmere.h: `simdjson::<implementation>` namesapce. Common implementation-specific tools like number and string parsing, as well as minification.
    * simdjson/arm64|fallback|haswell|icelake|ppc64|westmere/*.h: implementation-specific functions such as , etc.
    * simdjson/generic/*.h: the bulk of the actual code, written generically and compiled for each implementation, using functions defined in the implementation's .h files.
      * simdjson/generic/dependencies.h: dependencies on common, non-implementation-specific simdjson classes. This will be included before including amalgamated.h.
      * simdjson/generic/amalgamated.h: all generic ondemand classes for an implementation.
  * simdjson/ondemand.h: the `simdjson::ondemand` namespace. Includes all public ondemand classes.
    * simdjson/builtin.h: the `simdjson::builtin` namespace. Aliased to the most universal implementation available.
    * simdjson/builtin/ondemand.h: the `simdjson::builtin::ondemand` namespace.
    * simdjson/arm64|fallback|haswell|icelake|ppc64|westmere/ondemand.h: the `simdjson::<implementation>::ondemand` namespace. on demand compiled for the specific implementation.
    * simdjson/generic/ondemand/*.h: individual on demand classes, generically written.
      * simdjson/generic/ondemand/dependencies.h: dependencies on common, non-implementation-specific simdjson classes. This will be included before including amalgamated.h.
      * simdjson/generic/ondemand/amalgamated.h: all generic ondemand classes for an implementation.
* **src:** The source files for non-inlined functionality (e.g. the architecture-specific parser
  implementations).
  * simdjson.cpp: A "main source" that includes all implementation files from src/. This is
    equivalent to the distributed simdjson.cpp.
  * *.cpp: other misc. implementations, such as `simdjson::implementation` and the minifier.
  * arm64|fallback|haswell|icelake|ppc64|westmere.cpp: Architecture-specific parser implementations.
    * generic/*.h: `simdjson::<implementation>` namespace. Generic implementation of the parser, particularly the `dom_parser_implementation`.
    * generic/stage1/*.h: `simdjson::<implementation>::stage1` namespace. Generic implementation of the simd-heavy tokenizer/indexer pass of the simdjson parser. Used for the On Demand interface
    * generic/stage2/*.h: `simdjson::<implementation>::stage2` namespace. Generic implementation of the tape creator, which consumes the index from stage 1 and actually parses numbers and string and such. Used for the DOM interface.

Other important files and directories:
* **.drone.yml:** Definitions for Drone CI.
* **.appveyor.yml:** Definitions for Appveyor CI (Windows).
* **.circleci:** Definitions for Circle CI.
* **.github/workflows:** Definitions for GitHub Actions (CI).
* **singleheader:** Contains generated `simdjson.h` and `simdjson.cpp` that we release. The files `singleheader/simdjson.h` and `singleheader/simdjson.cpp` should never be edited by hand.
* **singleheader/amalgamate.py:** Generates `singleheader/simdjson.h` and `singleheader/simdjson.cpp` for release (python script).
* **benchmark:** This is where we do benchmarking. Benchmarking is core to every change we make; the
  cardinal rule is don't regress performance without knowing exactly why, and what you're trading
  for it. Many of our benchmarks are microbenchmarks. We are effectively doing controlled scientific experiments for the purpose of understanding what affects our performance. So we simplify as much as possible. We try to avoid irrelevant factors such as page faults, interrupts, unnecessary system calls. We recommend checking the performance as follows:
  ```bash
  mkdir build
  cd build
  cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
  cmake --build . --config Release
  benchmark/dom/parse ../jsonexamples/twitter.json
  ```
  The last line becomes `./benchmark/Release/parse.exe ../jsonexample/twitter.json` under Windows. You may also use Google Benchmark:
  ```bash
  mkdir build
  cd build
  cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
  cmake --build . --target bench_parse_call --config Release
  ./benchmark/bench_parse_call
  ```
  The last line becomes `./benchmark/Release/bench_parse_call.exe` under Windows. Under Windows, you can also build with the clang compiler by adding `-T ClangCL` to the call to `cmake ..`: `cmake -T ClangCL ..`.
* **fuzz:** The source for fuzz testing. This lets us explore important edge and middle cases
* **fuzz:** The source for fuzz testing. This lets us explore important edge and middle cases
  automatically, and is run in CI.
* **jsonchecker:** A set of JSON files used to check different functionality of the parser.
  * **pass*.json:** Files that should pass validation.
  * **fail*.json:** Files that should fail validation.
  * **jsonchecker/minefield/y_*.json:** Files that should pass validation.
  * **jsonchecker/minefield/n_*.json:** Files that should fail validation.
* **jsonexamples:** A wide spread of useful, real-world JSON files with different characteristics
  and sizes.
* **test:** The tests are here. basictests.cpp and errortests.cpp are the primary ones.
* **tools:** Source for executables that can be distributed with simdjson. Some examples:
  * `json2json mydoc.json` parses the document, constructs a model and then dumps back the result to standard output.
  * `json2json -d mydoc.json` parses the document, constructs a model and then dumps model (as a tape) to standard output. The tape format is described in the accompanying file `tape.md`.
  * `minify mydoc.json` minifies the JSON document, outputting the result to standard output. Minifying means to remove the unneeded white space characters.
  * `jsonpointer mydoc.json <jsonpath> <jsonpath> ... <jsonpath>` parses the document, constructs a model and then processes a series of [JSON Pointer paths](https://tools.ietf.org/html/rfc6901). The result is itself a JSON document.


> **Don't modify the files in singleheader/ directly; these are automatically generated.**


While simdjson distributes just two files from the singleheader/ directory, we *maintain* the code in
multiple files under include/ and src/. The files include/simdjson.h and src/simdjson.cpp are the "spine" for
these, and you can include them as if they were the corresponding singleheader/ files.



Runtime Dispatching
--------------------

A key feature of simdjson is the ability to compile different processing kernels, optimized for specific instruction sets, and to select
the most appropriate kernel at runtime. This ensures that users get the very best performance while still enabling simdjson to run everywhere.
This technique is frequently called runtime dispatching. The simdjson achieves runtime dispatching entirely in C++: we do not assume
that the user is building the code using CMake, for example.

To make runtime dispatching work, it is critical that the code be compiled for the lowest supported processor. In particular, you should
not use flags such as -mavx2, /arch:AVX2 and so forth while compiling simdjson. When you do so, you allow the compiler to use advanced
instructions. In turn, these advanced instructions present in the code may cause a runtime failure if the runtime processor does not
support them. Even a simple loop, compiled with these flags, might generate binary code that only run on advanced processors.

So we compile simdjson for a generic processor. Our users should do the same if they want simdjson's runtime dispatch to work. It is important
to understand that if runtime dispatching does not work, then simdjson will cause crashes on older processors. Of course, if a user chooses
to compile their code for a specific instruction set (e.g., AVX2), they are responsible for the failures if they later run their code
on a processor that does not support AVX2. Yet, if we were to entice these users to do so, we would share the blame: thus we carefully instruct
users to compile their code in a generic way without doing anything to enable advanced instructions.


We only use runtime dispatching on x64 (AMD/Intel) platforms, at the moment. On ARM processors, we would need a standard way to query, at runtime,
the processor for its supported features. We do not know how to do so on ARM systems in general. Thankfully it is not yet a concern: 64-bit ARM
processors are fairly uniform as far as the instruction sets they support.


In all cases, simdjson uses advanced instructions by relying on  "intrinsic functions": we do not write assembly code. The intrinsic functions
are special functions that the compiler might recognize and translate into fast code. To make runtime dispatching work, we rely on the fact that
the header providing these instructions
(intrin.h under Visual Studio, x86intrin.h elsewhere) defines all of the intrinsic functions, including those that are not supported
processor.

At this point, we are require to use one of two main strategies.

1. On POSIX systems, the main compilers (LLVM clang, GNU gcc) allow us to use any intrinsic function after including the header, but they fail to inline the resulting instruction if the target processor does not support them. Because we compile for a generic processor, we would not be able to use most intrinsic functions. Thankfully, more recent versions of these compilers allow us to flag a region of code with a specific target, so that we can compile only some of the code with support for advanced instructions. Thus in our C++, one might notice macros like `TARGET_HASWELL`. It is then our responsibility, at runtime, to only run the regions of code (that we call kernels) matching the properties of the runtime processor. The benefit of this approach is that the compiler not only let us use intrinsic functions, but it can also optimize the rest of the code in the kernel with advanced instructions we enabled.

2. Under Visual Studio, the problem is somewhat simpler. Visual Studio will not only provide the intrinsic functions, but it will also allow us to use them. They will compile just fine. It is at runtime that they may cause a crash. So we do not need to mark regions of code for compilation toward advanced processors (e.g., with  `TARGET_HASWELL` macros). The downside of the Visual Studio approach is that the compiler is not allowed to use advanced instructions others than those we specify. In principle, this means that Visual Studio has weaker optimization opportunities.



We also handle the special case where a user is compiling using LLVM clang under Windows, [using the Visual Studio toolchain](https://devblogs.microsoft.com/cppblog/clang-llvm-support-in-visual-studio/). If you compile with LLVM clang under Visual Studio, then the header files (intrin.h or x86intrin.h) no longer provides the intrinsic functions that are unsupported by the processor. This appears to be deliberate on the part of the LLVM engineers. With a few lines of code, we handle this scenario just like LLVM clang under a POSIX system, but forcing the inclusion of the specific headers, and rolling our own intrinsic function as needed.





Regenerating Single-Header Files
---------------------------------------

The simdjson.h and simdjson.cpp files in the singleheader directory are not always up-to-date with the rest of the code; they are only ever
systematically regenerated on releases. To ensure you have the latest code, you can regenerate them by running this at the top level:

```bash
mkdir build
cd build
cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
cmake --build . # needed, because currently dependencies do not work fully for the amalgamate target
cmake --build . --target amalgamate
```

You need to have python3 installed on your system.

The amalgamator script `amalgamate.py` generates singleheader/simdjson.h by
reading through include/simdjson.h, copy/pasting each header file into the amalgamated file at the
point it gets included (but only once per header). singleheader/simdjson.cpp is generated from
src/simdjson.cpp the same way, except files under generic/ may be included and copy/pasted multiple
times.

## Usage (CMake on 64-bit platforms like Linux, FreeBSD or macOS)

Requirements: In addition to git, we require a recent version of CMake as well as bash.

1. On macOS, the easiest way to install cmake might be to use [brew](https://brew.sh) and then type
```
brew install cmake
```
2. Under Linux, you might be able to install CMake as follows:
```
apt-get update -qq
apt-get install -y cmake
```
3. On FreeBSD, you might be able to install bash and CMake as follows:
```
pkg update -f
pkg install bash
pkg install cmake
```

You need a recent compiler like clang or gcc. We recommend at least GNU GCC/G++ 7 or LLVM clang 6.


Building: While in the project repository, do the following:

```
mkdir build
cd build
cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
cmake --build .
ctest
```

CMake will build a library. By default, it builds a static library (e.g., libsimdjson.a on Linux).

You can build a shared library:

```
mkdir buildshared
cd buildshared
cmake -D BUILD_SHARED_LIBS=ON -D SIMDJSON_DEVELOPER_MODE=ON ..
cmake --build .
ctest
```

In some cases, you may want to specify your compiler, especially if the default compiler on your system is too old.  You need to tell cmake which compiler you wish to use by setting the CC and CXX variables. Under bash, you can do so with commands such as `export CC=gcc-7` and `export CXX=g++-7`. You can also do it as part of the `cmake` command: `cmake -DCMAKE_CXX_COMPILER=g++ ..`.  You may proceed as follows:

```
brew install gcc@8
mkdir build
cd build
export CXX=g++-8 CC=gcc-8
cmake -D SIMDJSON_DEVELOPER_MODE=ON ..
cmake --build .
ctest
```

If your compiler does not default on C++11 support or better you may get failing tests. If so, you may be able to exclude the failing  tests by replacing `ctest` with `ctest  -E "^quickstart$"`.

Note that the name of directory (`build`) is arbitrary, you can name it as you want (e.g., `buildgcc`) and you can have as many different such directories as you would like (one per configuration).



## Usage (CMake on 64-bit Windows using Visual Studio 2019 or better)

Recent versions of Visual Studio support CMake natively, [please refer to the Visual Studio documentation](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170).

We assume you have a common 64-bit Windows PC with at least Visual Studio 2019.

- Grab the simdjson code from GitHub, e.g., by cloning it using [GitHub Desktop](https://desktop.github.com/).
- Install [CMake](https://cmake.org/download/). When you install it, make sure to ask that `cmake` be made available from the command line. Please choose a recent version of cmake.
- Create a subdirectory within simdjson, such as `build`.
- Using a shell, go to this newly created directory. You can start a shell directly from GitHub Desktop (Repository > Open in Command Prompt).
- Type `cmake  ..` in the shell while in the `build` repository.
- This last command (`cmake ...`) created a Visual Studio solution file in the newly created directory (e.g., `simdjson.sln`). Open this file in Visual Studio. You should now be able to build the project and run the tests. For example, in the `Solution Explorer` window (available from the `View` menu), right-click `ALL_BUILD` and select `Build`. To test the code, still in the `Solution Explorer` window, select `RUN_TESTS` and select `Build`.


Though having Visual Studio installed is necessary, one can build simdjson using only cmake commands:

- `mkdir build`
- `cd build`
- `cmake ..`
- `cmake --build . -config Release`


Furthermore, if you have installed LLVM clang on Windows, for example as a component of Visual Studio 2019, you can configure and build simdjson using LLVM clang on Windows using cmake:


- `mkdir build`
- `cd build`
- `cmake -T ClangCL ..`
- `cmake --build . -config Release`


## Various References

- [How to implement atoi using SIMD?](https://stackoverflow.com/questions/35127060/how-to-implement-atoi-using-simd)
- [Parsing JSON is a Minefield 💣](http://seriot.ch/parsing_json.php)
- https://tools.ietf.org/html/rfc7159
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
