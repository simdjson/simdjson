Hacking simdjson
================

Here is wisdom about how to build, test and run simdjson from within the repository. *Users* of
simdjson should use the released simdjson.h and simdjson.cpp files.

Directory Structure and Source
------------------------------

simdjson's source structure, from the top level, looks like this:

* **Makefile:** The main Makefile for Linux. This is not the same as CMakeLists.txt.
* **CMakeLists.txt:** A Makefile generator for non-default cases and options.
* **include:** User-facing declarations and inline definitions (most user-facing functions are inlined).
  * simdjson.h: A "master include" that includes files from include/simdjson/. This is equivalent to
    the distributed simdjson.h.
  * simdjson/*.h: Declarations for public simdjson classes and functions.
  * simdjson/inline/*.h: Definitions for public simdjson classes and functions.
* **src:** The source files for non-inlined functionality (e.g. the architecture-specific parser
  implementations).
  * simdjson.cpp: A "master source" that includes all implementation files from src/. This is
    equivalent to the distributed simdjson.cpp.
  * arm64/|fallback/|haswell/|westmere/: Architecture-specific implementations. All functions are 
    Each architecture defines its own namespace, e.g. simdjson::haswell.
  * generic/: Generic implementations of the simdjson parser. These files may be included and
    compiled multiple times, from whichever architectures use them. They assume they are already
    enclosed in a namespace, e.g.:
    ```c++
    namespace simdjson::haswell {
      #include "generic/stage1_find_marks.h"
    }
    ```

Other important files and directories:
* **.drone.yml:** Definitions for Drone CI.
* **.appveyor.yml:** Definitions for Appveyor CI (Windows).
* **.circleci:** Definitions for Circle CI.
* **amalgamation.sh:** Generates singleheader/simdjson.h and singleheader/simdjson.cpp for release.
* **benchmark:** This is where we do benchmarking. Benchmarking is core to every change we make; the
  cardinal rule is don't regress performance without knowing exactly why, and what you're trading
  for it. If you're not sure what else to do to check your performance, this is always a good start:
  ```bash
  make parse && ./parse jsonexamples/twitter.json
  ```
* **fuzz:** The source for fuzz testing. This lets us explore important edge and middle cases
  automatically, and is run in CI.
* **jsonchecker:** A set of JSON files used to check different functionality of the parser.
  * **pass*.json:** Files that should pass validation.
  * **fail*.json:** Files that should fail validation.
* **jsonexamples:** A wide spread of useful, real-world JSON files with different characteristics
  and sizes.
* **singleheader:** Contains generated simdjson.h and simdjson.cpp that we release.
* **test:** The tests are here. basictests.cpp and errortests.cpp are the primary ones.
* **tools:** Source for executables that can be distributed with simdjson

> **Don't modify the files in singleheader/ directly; these are automatically generated.**
> 
> While we distribute those files on release, we *maintain* the files under include/ and src/.

While simdjson distributes just two files from the singleheader/ directory, we *maintain* the code in
multiple files under include/ and src/. include/simdjson.h and src/simdjson.cpp are the "spine" for
these, and you can include

Regenerating Single Headers From Master
---------------------------------------

simdjson.h and simdjson.cpp are not always up to date in master. To ensure you have the latest copy,
you can regenerate them by running this at the top level:

```bash
make amalgamate
```

The amalgamator is at `amalgamation.sh` at the top level. It generates singleheader/simdjson.h by
reading through include/simdjson.h, copy/pasting each header file into the amalgamated file at the
point it gets included (but only once per header). singleheader/simdjson.cpp is generated from
src/simdjson.cpp the same way, except files under generic/ may be included and copy/pasted multiple
times.

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
