Hacking simdjson
================

Here is wisdom about how to build, test and run simdjson from within the repository. *Users* of
simdjson should use the released simdjson.h and simdjson.cpp files.

If you plan to contribute to simdjson, please read our [CONTRIBUTING](https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md) guide.

Directory Structure and Source
------------------------------

simdjson's source structure, from the top level, looks like this:

* **CMakeLists.txt:** The main build system.
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
    namespace simdjson {
      namespace haswell {
        #include "generic/stage1/json_structural_indexer.h"
      }
    }
    ```

Other important files and directories:
* **.drone.yml:** Definitions for Drone CI.
* **.appveyor.yml:** Definitions for Appveyor CI (Windows).
* **.circleci:** Definitions for Circle CI.
* **amalgamate.sh:** Generates singleheader/simdjson.h and singleheader/simdjson.cpp for release.
* **benchmark:** This is where we do benchmarking. Benchmarking is core to every change we make; the
  cardinal rule is don't regress performance without knowing exactly why, and what you're trading
  for it. Many of our benchmarks are microbenchmarks. We are effectively doing controlled scientific experiments for the purpose of understanding what affects our performance. So we simplify as much as possible. We try to avoid irrelevant factors such as page faults, interrupts, unnnecessary system calls. We recommend checking the performance as follows:
  ```bash
  mkdir build
  cd build
  cmake ..
  cmake --build . --config Release
  benchmark/parse ../jsonexamples/twitter.json
  ```
  The last line becomes `./benchmark/Release/parse.exe ../jsonexample/twitter.json` under Windows. You may also use Google Benchmark:
  ```bash
  mkdir build
  cd build
  cmake ..
  cmake --build . --target bench_parse_call --config Release
  ./benchmark/bench_parse_call
  ```
  The last line becomes `./benchmark/Release/bench_parse_call.exe` under Windows. Under Windows, you can also build with the clang compiler by adding `-T ClangCL` to the call to `cmake ..`: `cmake .. - TClangCL`.
* **fuzz:** The source for fuzz testing. This lets us explore important edge and middle cases
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

1. On POSIX systems, the main compilers (LLVM clang, GNU gcc) allow us to use any intrinsic function after including the header, but they fail to inline the resulting instruction if the target processor does not support them. Because we compile for a generic processor, we would not be able to use most intrinsic functions. Thankfully, more recent versions of these compilers allow us to flag a region of code with a specific target, so that we can compile only some of the code with support for advanced instructions. Thus in our C++, one might notice macros like `TARGET_HASWELL`. It is then our responsability, at runtime, to only run the regions of code (that we call kernels) matching the properties of the runtime processor. The benefit of this approach is that the compiler not only let us use intrinsic functions, but it can also optimize the rest of the code in the kernel with advanced instructions we enabled.

2. Under Visual Studio, the problem is somewhat simpler. Visual Studio will not only provide the intrinsic functions, but it will also allow us to use them. They will compile just fine. It is at runtime that they may cause a crash. So we do not need to mark regions of code for compilation toward advanced processors (e.g., with  `TARGET_HASWELL` macros). The downside of the Visual Studio approach is that the compiler is not allowed to use advanced instructions others than those we specify. In principle, this means that Visual Studio has weaker optimization opportunities.



We also handle the special case where a user is compiling using LLVM clang under Windows, [using the Visual Studio toolchain](https://devblogs.microsoft.com/cppblog/clang-llvm-support-in-visual-studio/). If you compile with LLVM clang under Visual Studio, then the header files (intrin.h or x86intrin.h) no longer provides the intrinsic functions that are unsupported by the processor. This appears to be deliberate on the part of the LLVM engineers. With a few lines of code, we handle this scenario just like LLVM clang under a POSIX system, but forcing the inclusion of the specific headers, and rolling our own intrinsic function as needed.





Regenerating Single Headers From Master
---------------------------------------

simdjson.h and simdjson.cpp are not always up to date in master. To ensure you have the latest copy,
you can regenerate them by running this at the top level:

```bash
mkdir build
cd build
cmake ..
cmake --build . --target amalgamate
```

The amalgamator is at `amalgamate.sh` at the top level. It generates singleheader/simdjson.h by
reading through include/simdjson.h, copy/pasting each header file into the amalgamated file at the
point it gets included (but only once per header). singleheader/simdjson.cpp is generated from
src/simdjson.cpp the same way, except files under generic/ may be included and copy/pasted multiple
times.

### Usage (CMake on 64-bit platforms like Linux, freeBSD or macOS)

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
3. On freeBSD, you might be able to install bash and CMake as follows:
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
cmake ..
cmake --build .
ctest
```

CMake will build a library. By default, it builds a shared library (e.g., libsimdjson.so on Linux).

You can build a static library:

```
mkdir buildstatic
cd buildstatic
cmake -DSIMDJSON_BUILD_STATIC=ON ..
cmake --build .
ctest
```

In some cases, you may want to specify your compiler, especially if the default compiler on your system is too old.  You need to tell cmake which compiler you wish to use by setting the CC and CXX variables. Under bash, you can do so with commands such as `export CC=gcc-7` and `export CXX=g++-7`. You can also do it as part of the `cmake` command: `cmake .. -DCMAKE_CXX_COMPILER=g++`.  You may proceed as follows:

```
brew install gcc@8
mkdir build
cd build
export CXX=g++-8 CC=gcc-8
cmake ..
cmake --build .
ctest
```

If your compiler does not default on C++11 support or better you may get failing tests. If so, you may be able to exclude the failing  tests by replacing `ctest` with `ctest  -E "^quickstart$"`.

Note that the name of directory (`build`) is arbitrary, you can name it as you want (e.g., `buildgcc`) and you can have as many different such directories as you would like (one per configuration).



### Usage (CMake on 64-bit Windows using Visual Studio)

We assume you have a common 64-bit Windows PC with at least Visual Studio 2017 and an x64 processor with AVX2 support (2013 Intel Haswell or later) or SSE 4.2 + CLMUL (2010 Westmere or later).

- Grab the simdjson code from GitHub, e.g., by cloning it using [GitHub Desktop](https://desktop.github.com/).
- Install [CMake](https://cmake.org/download/). When you install it, make sure to ask that `cmake` be made available from the command line. Please choose a recent version of cmake.
- Create a subdirectory within simdjson, such as `build`.
- Using a shell, go to this newly created directory. You can start a shell directly from GitHub Desktop (Repository > Open in Command Prompt).
- Type `cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..` in the shell while in the `build` repository. (Alternatively, if you want to build a DLL, you may use the command line `cmake -DCMAKE_GENERATOR_PLATFORM=x64 -DSIMDJSON_BUILD_STATIC=OFF ..`.)
- This last command (`cmake ...`) created a Visual Studio solution file in the newly created directory (e.g., `simdjson.sln`). Open this file in Visual Studio. You should now be able to build the project and run the tests. For example, in the `Solution Explorer` window (available from the `View` menu), right-click `ALL_BUILD` and select `Build`. To test the code, still in the `Solution Explorer` window, select `RUN_TESTS` and select `Build`.


Though having Visual Studio installed is necessary, one can build simdjson using only cmake commands:

- `mkdir build`
- `cd build`
- `cmake ..`
- `cmake --build . -config Release`


Furthermore, if you have installed LLVM clang on Windows, for example as a component of Visual Studio 2019, you can configure and build simdjson using LLVM clang on Windows using cmake:


- `mkdir build`
- `cd build`
- `cmake ..  -T ClangCL`
- `cmake --build . -config Release`


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



### UTF-8 validation (lookup2)

The simdjson library relies on the lookup2 algorithm for UTF-8 validation on x64 platforms.

This algorithm validate the length of multibyte characters (that each multibyte character has the right number of continuation characters, and that all continuation characters are part of a multibyte  character).

####  Algorithm

This algorithm compares *expected* continuation characters with *actual* continuation bytes, and emits an error anytime there is a mismatch.

For example, in the string "𝄞₿֏ab", which has a 4-, 3-, 2- and 1-byte
characters, the file will look like this:

| Character             | 𝄞  |    |    |    | ₿  |    |    | ֏  |    | a  | b  |
|-----------------------|----|----|----|----|----|----|----|----|----|----|----|
| Character Length      |  4 |    |    |    |  3 |    |    |  2 |    |  1 |  1 |
| Byte                  | F0 | 9D | 84 | 9E | E2 | 82 | BF | D6 | 8F | 61 | 62 |
| is_second_byte        |    |  X |    |    |    |  X |    |    |  X |    |    |
| is_third_byte         |    |    |  X |    |    |    |  X |    |    |    |    |
| is_fourth_byte        |    |    |    |  X |    |    |    |    |    |    |    |
| expected_continuation |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
| is_continuation       |    |  X |  X |  X |    |  X |  X |    |  X |    |    |

The errors here are basically (Second Byte OR Third Byte OR Fourth Byte == Continuation):

- **Extra Continuations:** Any continuation that is not a second, third or fourth byte is not
  part of a valid 2-, 3- or 4-byte character and is thus an error. It could be that it's just
  floating around extra outside of any character, or that there is an illegal 5-byte character,
  or maybe it's at the beginning of the file before any characters have started; but it's an
  error in all these cases.
- **Missing Continuations:** Any second, third or fourth byte that *isn't* a continuation is an error, because that means
  we started a new character before we were finished with the current one.

####  Getting the Previous Bytes

Because we want to know if a byte is the *second* (or third, or fourth) byte of a multibyte
character, we need to "shift the bytes" to find that out. This is what they mean:

- `is_continuation`: if the current byte is a continuation.
- `is_second_byte`: if 1 byte back is the start of a 2-, 3- or 4-byte character.
- `is_third_byte`: if 2 bytes back is the start of a 3- or 4-byte character.
- `is_fourth_byte`: if 3 bytes back is the start of a 4-byte character.

We use shuffles to go n bytes back, selecting part of the current `input` and part of the
`prev_input` (search for `.prev<1>`, `.prev<2>`, etc.). These are passed in by the caller
function, because the 1-byte-back data is used by other checks as well.

####   Getting the Continuation Mask

Once we have the right bytes, we have to get the masks. To do this, we treat UTF-8 bytes as
numbers, using signed `<` and `>` operations to check if they are continuations or leads.
In fact, we treat the numbers as *signed*, partly because it helps us, and partly because
Intel's SIMD presently only offers signed `<` and `>` operations (not unsigned ones).

In UTF-8, bytes that start with the bits 110, 1110 and 11110 are 2-, 3- and 4-byte "leads,"
respectively, meaning they expect to have 1, 2 and 3 "continuation bytes" after them.
Continuation bytes start with 10, and ASCII (1-byte characters) starts with 0.

When treated as signed numbers, they look like this:

| Type         | High Bits  | Binary Range | Signed |
|--------------|------------|--------------|--------|
| ASCII        | `0`        | `01111111`   |   127  |
|              |            | `00000000`   |     0  |
| 4+-Byte Lead | `1111`     | `11111111`   |    -1  |
|              |            | `11110000    |   -16  |
| 3-Byte Lead  | `1110`     | `11101111`   |   -17  |
|              |            | `11100000    |   -32  |
| 2-Byte Lead  | `110`      | `11011111`   |   -33  |
|              |            | `11000000    |   -64  |
| Continuation | `10`       | `10111111`   |   -65  |
|              |            | `10000000    |  -128  |

This makes it pretty easy to get the continuation mask! It's just a single comparison:

```
is_continuation = input < -64`
```

We can do something similar for the others, but it takes two comparisons instead of one: "is
the start of a 4-byte character" is `< -32` and `> -65`, for example. And 2+ bytes is `< 0` and
`> -64`. Surely we can do better, they're right next to each other!

####  Getting the is_xxx Masks: Shifting the Range

Notice *why* continuations were a single comparison. The actual *range* would require two
comparisons--`< -64` and `> -129`--but all characters are always greater than -128, so we get
that for free. In fact, if we had *unsigned* comparisons, 2+, 3+ and 4+ comparisons would be
just as easy: 4+ would be `> 239`, 3+ would be `> 223`, and 2+ would be `> 191`.

Instead, we add 128 to each byte, shifting the range up to make comparison easy. This wraps
ASCII down into the negative, and puts 4+-Byte Lead at the top:

| Type                 | High Bits  | Binary Range | Signed |
|----------------------|------------|--------------|-------|
| 4+-Byte Lead (+ 127) | `0111`     | `01111111`   |   127 |
|                      |            | `01110000    |   112 |
|----------------------|------------|--------------|-------|
| 3-Byte Lead (+ 127)  | `0110`     | `01101111`   |   111 |
|                      |            | `01100000    |    96 |
|----------------------|------------|--------------|-------|
| 2-Byte Lead (+ 127)  | `010`      | `01011111`   |    95 |
|                      |            | `01000000    |    64 |
|----------------------|------------|--------------|-------|
| Continuation (+ 127) | `00`       | `00111111`   |    63 |
|                      |            | `00000000    |     0 |
|----------------------|------------|--------------|-------|
| ASCII (+ 127)        | `1`        | `11111111`   |    -1 |
|                      |            | `10000000`   |  -128 |
|----------------------|------------|--------------|-------|

*Now* we can use signed `>` on all of them:

```
prev1 = input.prev<1>
prev2 = input.prev<2>
prev3 = input.prev<3>
prev1_flipped = input.prev<1>(prev_input) ^ 0x80; // Same as `+ 128`
prev2_flipped = input.prev<2>(prev_input) ^ 0x80; // Same as `+ 128`
prev3_flipped = input.prev<3>(prev_input) ^ 0x80; // Same as `+ 128`
is_second_byte = prev1_flipped > 63;2+-byte lead
is_third_byte  = prev2_flipped > 95;3+-byte lead
is_fourth_byte = prev3_flipped > 111; // 4+-byte lead
```

NOTE: we use `^ 0x80` instead of `+ 128` in the code, which accomplishes the same thing, and even takes the same number
of cycles as `+`, but on many Intel architectures can be parallelized better (you can do 3
`^`'s at a time on Haswell, but only 2 `+`'s).

That doesn't look like it saved us any instructions, did it? Well, because we're adding the
same number to all of them, we can save one of those `+ 128` operations by assembling
`prev2_flipped` out of prev 1 and prev 3 instead of assembling it from input and adding 128
to it. One more instruction saved!

```
prev1 = input.prev<1>
prev3 = input.prev<3>
prev1_flipped = prev1 ^ 0x80; // Same as `+ 128`
prev3_flipped = prev3 ^ 0x80; // Same as `+ 128`
prev2_flipped = prev1_flipped.concat<2>(prev3_flipped): // <shuffle: take the first 2 bytes from prev1 and the rest from prev3  
```

####  Bringing It All Together: Detecting the Errors

At this point, we have `is_continuation`, `is_first_byte`, `is_second_byte` and `is_third_byte`.
All we have left to do is check if they match!

```
return (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
```

But wait--there's more. The above statement is only 3 operations, but they *cannot be done in
parallel*. You have to do 2 `|`'s and then 1 `&`. Haswell, at least, has 3 ports that can do
bitwise operations, and we're only using 1!

####  Epilogue: Addition For Booleans

There is one big case the above code doesn't explicitly talk about--what if is_second_byte
and is_third_byte are BOTH true? That means there is a 3-byte and 2-byte character right next
to each other (or any combination), and the continuation could be part of either of them!
Our algorithm using `&` and `|` won't detect that the continuation byte is problematic.

Never fear, though. If that situation occurs, we'll already have detected that the second
leading byte was an error, because it was supposed to be a part of the preceding multibyte
character, but it *wasn't a continuation*.

We could stop here, but it turns out that we can fix it using `+` and `-` instead of `|` and
`&`, which is both interesting and possibly useful (even though we're not using it here). It
exploits the fact that in SIMD, a *true* value is -1, and a *false* value is 0. So those
comparisons were giving us numbers!

Given that, if you do `is_second_byte + is_third_byte + is_fourth_byte`, under normal
circumstances you will either get 0 (0 + 0 + 0) or -1 (-1 + 0 + 0, etc.). Thus,
`(is_second_byte + is_third_byte + is_fourth_byte) - is_continuation` will yield 0 only if
*both* or *neither* are 0 (0-0 or -1 - -1). You'll get 1 or -1 if they are different. Because
*any* nonzero value is treated as an error (not just -1), we're just fine here :)

Further, if *more than one* multibyte character overlaps,
`is_second_byte + is_third_byte + is_fourth_byte` will be -2 or -3! Subtracting `is_continuation`
from *that* is guaranteed to give you a nonzero value (-1, -2 or -3). So it'll always be
considered an error.

One reason you might want to do this is parallelism. ^ and | are not associative, so
(A | B | C) ^ D will always be three operations in a row: either you do A | B -> | C -> ^ D, or
you do B | C -> | A -> ^ D. But addition and subtraction *are* associative: (A + B + C) - D can
be written as `(A + B) + (C - D)`. This means you can do A + B and C - D at the same time, and
then adds the result together. Same number of operations, but if the processor can run
independent things in parallel (which most can), it runs faster.

This doesn't help us on Intel, but might help us elsewhere: on Haswell, at least, | and ^ have
a super nice advantage in that more of them can be run at the same time (they can run on 3
ports, while + and - can run on 2)! This means that we can do A | B while we're still doing C,
saving us the cycle we would have earned by using +. Even more, using an instruction with a
wider array of ports can help *other* code run ahead, too, since these instructions can "get
out of the way," running on a port other instructions can't.

####  Epilogue II: One More Trick

There's one more relevant trick up our sleeve, it turns out: it turns out on Intel we can "pay
for" the (prev<1> + 128) instruction, because it can be used to save an instruction in
check_special_cases()--but we'll talk about that there :)




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
