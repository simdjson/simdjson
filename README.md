#  simdjson : Parsing gigabytes of JSON per second
[![Build Status](https://cloud.drone.io/api/badges/lemire/simdjson/status.svg)](https://cloud.drone.io/lemire/simdjson/)
[![CircleCI](https://circleci.com/gh/lemire/simdjson.svg?style=svg)](https://circleci.com/gh/lemire/simdjson)
[![Build Status](https://img.shields.io/appveyor/ci/lemire/simdjson.svg)](https://ci.appveyor.com/project/lemire/simdjson)
[![][license img]][license]


## A C++ library to see how fast we can parse JSON with complete validation.

JSON documents are everywhere on the Internet. Servers spend a lot of time parsing these documents. We want to accelerate the parsing of JSON per se using commonly available SIMD instructions as much as possible while doing full validation (including character encoding).

<img src="images/logo.png" width="10%">


## Paper

A description of the design and implementation of simdjson appears at https://arxiv.org/abs/1902.08318 and an informal blog post providing some background and context is at https://branchfree.org/2019/02/25/paper-parsing-gigabytes-of-json-per-second/.

Some people [enjoy reading our paper](https://arxiv.org/abs/1902.08318):

[<img src="images/halvarflake.png" width="50%">](https://twitter.com/halvarflake/status/1118459536686362625)


## Performance results

simdjson uses three-quarters less instructions than state-of-the-art parser RapidJSON and fifty percent less than sajson. To our knowledge, simdjson is the first fully-validating JSON parser to run at gigabytes per second on commodity processors.

<img src="doc/gbps.png" width="90%">

On a Skylake processor, the parsing speeds (in GB/s) of various processors on the twitter.json file are as follows.

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

## Requirements

- We support platforms like Linux or macOS, as well as Windows through Visual Studio 2017 or later.
- A processor with AVX2 (i.e., Intel processors starting with the Haswell microarchitecture released 2013 and AMD processors starting with the Zen microarchitecture released 2017).
- A recent C++ compiler (e.g., GNU GCC or LLVM CLANG or Visual Studio 2017), we assume C++17. GNU GCC 7 or better or LLVM's clang 6 or better.
- Some benchmark scripts assume bash and other common utilities, but they are optional.

## License

This code is made available under the Apache License 2.0.

Under Windows, we build some tools using the windows/dirent_portable.h file (which is outside our library code): it under the liberal (business-friendly) MIT license.

## Code example

```C
#include "simdjson/jsonparser.h"

/...

const char * filename = ... //

// use whatever means you want to get a string (UTF-8) of your JSON document
padded_string p = get_corpus(filename); 
ParsedJson pj;
pj.allocateCapacity(p.size()); // allocate memory for parsing up to p.size() bytes
const int res = json_parse(p, pj); // do the parsing, return 0 on success
// parsing is done!
if (res != 0) {
    // You can use the "simdjson/simdjson.h" header to access the error message
    std::cout << "Error parsing:" << simdjson::errorMsg(res) << std::endl;
}
// the ParsedJson document can be used here
// pj can be reused with other json_parse calls.
```

It is also possible to use a simpler API if you do not mind having the overhead
of memory allocation with each new JSON document:

```C
#include "simdjson/jsonparser.h"

/...

const char * filename = ... //
padding_string p = get_corpus(filename);
ParsedJson pj = build_parsed_json(p); // do the parsing
if( ! pj.isValid() ) {
    // something went wrong
}
```

Though the `padded_string` class is recommended for best performance, you can call `json_parse` and `build_parsed_json`, passing a standard `std::string` object.


```C
#include "simdjson/jsonparser.h"

/...
std::string mystring = ... //
ParsedJson pj;
pj.allocateCapacity(mystring.size()); // allocate memory for parsing up to p.size() bytes
// std::string may not overallocate so a copy will be needed
const int res = json_parse(mystring, pj); // do the parsing, return 0 on success
// parsing is done!
if (res != 0) {
    // You can use the "simdjson/simdjson.h" header to access the error message
    std::cout << "Error parsing:" << simdjson::errorMsg(res) << std::endl;
}
// pj can be reused with other json_parse calls.
```

or

```C
#include "simdjson/jsonparser.h"

/...

std::string mystring = ... //
// std::string may not overallocate so a copy will be needed
ParsedJson pj = build_parsed_json(mystring); // do the parsing
if( ! pj.isValid() ) {
    // something went wrong
}
```

As needed, the `json_parse` and `build_parsed_json` functions copy the input data to a temporary buffer readable up to SIMDJSON_PADDING bytes beyond the end of the data. 

## Usage: easy single-header version

See the "singleheader" repository for a single header version. See the included
file "amalgamation_demo.cpp" for usage. This requires no specific build system: just
copy the files in your project in your include path. You can then include them quite simply:

```C
#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  const char * filename = argv[1];
  padded_string p = get_corpus(filename);
  ParsedJson pj = build_parsed_json(p); // do the parsing
  if( ! pj.isValid() ) {
    std::cout << "not valid" << std::endl;
  } else {
    std::cout << "valid" << std::endl;
  }
  return EXIT_SUCCESS;
}
```

We require hardware support for AVX2 instructions. You have to make sure that you instruct your 
compiler to use these instructions as needed. Under compilers such as GNU GCC or LLVM clang, the
flag `-march=native` used on a recent Intel processor (Haswell or better) is sufficient. For portability
of the binary files you can also specify directly the Haswell processor (`-march=haswell`). You may 
also use the flags `-mavx2 -mbmi2`. Under Visual Studio, you need to target x64 and add the 
flag `/arch:AVX2`. 


Note: In some settings, it might be desirable to precompile `simdjson.cpp` instead of including it.



## Usage (old-school Makefile on platforms like Linux or macOS)

Requirements: recent clang or gcc, and make. We recommend at least GNU GCC/G++ 7 or LLVM clang 6. A system like Linux or macOS is expected.

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

## Usage (CMake on platforms like Linux or macOS)

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

## Usage (CMake on Windows using Visual Studio)

We assume you have a common Windows PC with at least Visual Studio 2017 and an x64 processor with AVX2 support (2013 Intel Haswell or later).

- Grab the simdjson code from GitHub, e.g., by cloning it using [GitHub Desktop](https://desktop.github.com/).
- Install [CMake](https://cmake.org/download/). When you install it, make sure to ask that `cmake` be made available from the command line. Please choose a recent version of cmake.
- Create a subdirectory within simdjson, such as `VisualStudio`.
- Using a shell, go to this newly created directory.
- Type `cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..` in the shell while in the `VisualStudio` repository. (Alternatively, if you want to build a DLL, you may use the command line `cmake -DCMAKE_GENERATOR_PLATFORM=x64 -DSIMDJSON_BUILD_STATIC=OFF ..`.)
- This last command created a Visual Studio solution file in the newly created directory (e.g., `simdjson.sln`). Open this file in Visual Studio. You should now be able to build the project and run the tests. For example, in the `Solution Explorer` window (available from the `View` menu), right-click `ALL_BUILD` and select `Build`. To test the code, still in the `Solution Explorer` window, select `RUN_TESTS` and select `Build`.


## Usage (Using `vcpkg` on Windows, Linux and MacOS)

[vcpkg](https://github.com/Microsoft/vcpkg) users on Windows, Linux and MacOS can download and install `simdjson` with one single command from their favorite shell.

On Linux and MacOS:

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

If you find the version of `simdjson` shipped with `vcpkg` is out-of-date, feel free to report it to `vcpkg` community either by submiting an issue or by creating a PR.


## Tools

- `json2json mydoc.json` parses the document, constructs a model and then dumps back the result to standard output.
- `json2json -d mydoc.json` parses the document, constructs a model and then dumps model (as a tape) to standard output. The tape format is described in the accompanying file `tape.md`.
- `minify mydoc.json` minifies the JSON document, outputting the result to standard output. Minifying means to remove the unneeded white space characters.

## Scope

We provide a fast parser, that fully validates an input according to various specifications.
The parser builds a useful immutable (read-only) DOM (document-object model) which can be later accessed.

To simplify the engineering, we make some assumptions.

- We support UTF-8 (and thus ASCII), nothing else (no Latin, no UTF-16). We do not believe this is a genuine limitation, because we do not think there is any serious application that needs to process JSON data without an ASCII or UTF-8 encoding.
- All strings in the JSON document may have up to 4294967295 bytes in UTF-8 (4GB). To enforce this constraint, we refuse to parse a document that contains more than 4294967295 bytes (4GB). This should accommodate most JSON documents.
- We assume AVX2 support, which is available in all recent mainstream x86 processors produced by AMD and Intel. No support for non-x86 processors is included, though it can be done. We plan to support ARM processors (help is invited).
- In cases of failure, we report a failure without any indication to the nature of the problem. (This can be easily improved without affecting performance.)
- As allowed by the specification, we allow repeated keys within an object (other parsers like sajson do the same).
- Performance is optimized for JSON documents spanning at least a tens kilobytes up to many megabytes: the performance issues with having to parse many tiny JSON documents or one truly enormous JSON document are different.

_We do not aim to provide a general-purpose JSON library._ A library like RapidJSON offers much more than just parsing, it helps you generate JSON and offers various other convenient functions. We merely parse the document.

## Features

- The input string is unmodified. (Parsers like sajson and RapidJSON use the input string as a buffer.)
- We parse integers and floating-point numbers as separate types which allows us to support large 64-bit integers in [-9223372036854775808,9223372036854775808), like a Java `long` or a C/C++ `long long`. Among the parsers that differentiate between integers and floating-point numbers, not all support 64-bit integers. (For example, sajson rejects JSON files with integers larger than or equal to 2147483648. RapidJSON will parse a file containing an overly long integer like 18446744073709551616 as a floating-point number.) When we cannot represent exactly an integer as a signed 64-bit value, we reject the JSON document.
- We do full UTF-8 validation as part of the parsing. (Parsers like fastjson, gason and dropbox json11 do not do UTF-8 validation.)
- We fully validate the numbers. (Parsers like gason and ultranjson will accept `[0e+]` as valid JSON.)
- We validate string content for unescaped characters. (Parsers like fastjson and ultrajson accept unescaped line breaks and tabs in strings.)

## Architecture

The parser works in two stages:

- Stage 1. (Find marks) Identifies quickly structure elements, strings, and so forth. We validate UTF-8 encoding at that stage.
- Stage 2. (Structure building) Involves constructing a "tree" of sort (materialized as a tape) to navigate through the data. Strings and numbers are parsed at this stage.

## Navigating the parsed document

Here is a code sample to dump back the parsed JSON to a string:

```c
    ParsedJson::iterator pjh(pj);
    if (!pjh.isOk()) {
      std::cerr << " Could not iterate parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
    compute_dump(pj);
    //
    // where compute_dump is :

void compute_dump(ParsedJson::iterator &pjh) {
  if (pjh.is_object()) {
    std::cout << "{";
    if (pjh.down()) {
      pjh.print(std::cout); // must be a string
      std::cout << ":";
      pjh.next();
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        pjh.print(std::cout);
        std::cout << ":";
        pjh.next();
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "}";
  } else if (pjh.is_array()) {
    std::cout << "[";
    if (pjh.down()) {
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "]";
  } else {
    pjh.print(std::cout); // just print the lone value
  }
}
```

The following function will find all user.id integers:

```C
void simdjson_traverse(std::vector<int64_t> &answer, ParsedJson::iterator &i) {
  switch (i.get_type()) {
  case '{':
    if (i.down()) {
      do {
        bool founduser = equals(i.get_string(), "user");
        i.next(); // move to value
        if (i.is_object()) {
          if (founduser && i.move_to_key("id")) {
            if (i.is_integer()) {
              answer.push_back(i.get_integer());
            }
            i.up();
          }
          simdjson_traverse(answer, i);
        } else if (i.is_array()) {
          simdjson_traverse(answer, i);
        }
      } while (i.next());
      i.up();
    }
    break;
  case '[':
    if (i.down()) {
      do {
        if (i.is_object_or_array()) {
          simdjson_traverse(answer, i);
        }
      } while (i.next());
      i.up();
    }
    break;
  case 'l':
  case 'd':
  case 'n':
  case 't':
  case 'f':
  default:
    break;
  }
}
```

## In-depth comparisons

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
a table-oriented output that can be conventiently parsed by other tools.

## Other programming languages

We distinguish between "bindings" (which just wrap the C++ code) and a port to another programming language (which reimplements everything).

- [pysimdjson](https://github.com/TkTech/pysimdjson): Python bindings for the simdjson project.
- [simdjson-rs](https://github.com/Licenser/simdjson-rs): Rust port
- [SimdJsonSharp](https://github.com/EgorBo/SimdJsonSharp): C# version for .NET Core (bindings and full port)
- [simdjson_nodejs](https://github.com/luizperes/simdjson_nodejs): Node.js bindings for the simdjson project.
- [simdjson_php](https://github.com/crazyxman/simdjson_php): PHP bindings for the simdjson project.

## Various References

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

## Remarks on JSON parsing

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

## Academic References

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

[license]: LICENSE
[license img]: https://img.shields.io/badge/License-Apache%202-blue.svg
