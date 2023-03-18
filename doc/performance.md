Performance Notes
=================

simdjson strives to be at its fastest *without tuning*, and generally achieves this. However, there
are still some scenarios where tuning can enhance performance.
Once your code is tested, we
further encourage you to define `NDEBUG` in your Release builds to disable additional runtime
testing and get the best performance.
* [NDEBUG directive](#ndebug-directive)
* [Reusing the parser for maximum efficiency](#reusing-the-parser-for-maximum-efficiency)
* [Reusing string buffers](#reusing-string-buffers)
* [Server Loops: Long-Running Processes and Memory Capacity](#server-loops-long-running-processes-and-memory-capacity)
* [Large files and huge page support](#large-files-and-huge-page-support)
* [Number parsing](#number-parsing)
* [Visual Studio](#visual-studio)
* [Power Usage and Downclocking](#power-usage-and-downclocking)


NDEBUG directive
-------------

In C/C++, the `NDEBUG` pre-processor directive is not set by default. When it is not set, the simdjson library does
many additional checks that may impact negatively the performance. We recommend that, once your code
is well tested, you define `NDEBUG` directive in your Release builds. The `NDEBUG` directive should be defined
prior to including the `simdjson.h` header.

The `NDEBUG` directive is generally independent from optimization flags. For example, setting `-O3` under
GCC does not set the `NDEBUG` directive.

Reusing the parser for maximum efficiency
-----------------------------------------

If you're using simdjson to parse multiple documents, or in a loop, you should make a parser once
and reuse it. The simdjson library will allocate and retain internal buffers between parses, keeping
buffers hot in cache and keeping memory allocation and initialization to a minimum. In this manner,
you can parse terabytes of JSON data without doing any new allocation.

```c++
ondemand::parser parser;

// This initializes buffers  big enough to handle this JSON.
auto json = "[ true, false ]"_padded;
auto doc = parser.iterate(json);
for(bool i : doc.get_array()) {
  cout << i << endl;
}

// This reuses the existing buffers
auto number_json = "[1, 2, 3]"_padded;
doc = parser.iterate(number_json);
for(int64_t i : doc.get_array()) {
  cout << i << endl;
}
```


Reusing string buffers
-----------------------------------------

We recommend against creating many `std::string` or `simdjson::padded_string` instances to store the JSON content in your application. [Creating many non-trivial objects is convenient but often surprisingly slow](https://lemire.me/blog/2020/08/08/performance-tip-constructing-many-non-trivial-objects-is-slow/). Instead, as much as possible, you should allocate (once or a few times) reusable memory buffers where you write your JSON content. If you have a buffer `json_str` (of type `char*`) allocated for  `capacity` bytes and you store a JSON document spanning `length` bytes, you can pass it to simdjson as follows:

```c++
 auto doc = parser.iterate(padded_string_view(json_str, length, capacity));
```

or simply


```c++
 auto doc = parser.iterate(json_str, length, capacity);
```


Server Loops: Long-Running Processes and Memory Capacity
---------------------------------

The On Demand approach also automatically expands its memory capacity when larger documents are parsed. However, for longer processes where very large files are processed (such as server loops), this capacity is not resized down. On Demand also lets you adjust the maximal capacity that the parser can process:

* You can set an upper bound (*max_capacity*) when construction the parser:
```C++
    ondemand::parser parser(1000*1000);  // Never grows past documents > 1 MB
    auto doc = parser.iterate(json);
    for (web_request request : listen()) {
      padded_string json;
      padded_string json = padded_string::load(request.body);
      auto error = parser.iterate(json);
      // If the document was above our limit, emit 413 = payload too large
      if (error == CAPACITY) { request.respond(413); continue; }
      // ...
    }
```

The capacity will grow as the parser encounters larger documents up to 1 MB.

* You can also allocate a *fixed capacity* that will never grow:
```C++
    ondemand::parser parser(1000*1000);
    parser.allocate(1000*1000)  // Fix the capacity to 1 MB
    auto doc = parser.iterate(json);
    for (web_request request : listen()) {
      padded_string json;
      padded_string json = padded_string::load(request.body);
      auto error = parser.iterate(json);
      // If the document was above our limit, emit 413 = payload too large
      if (error == CAPACITY) { request.respond(413); continue; }
      // ...
    }
```
You can also manually set the maximal capacity using the method `set_max_capacity()`.

Large files and huge page support
---------------------------------

There is a memory allocation performance cost the first time you process a large file (e.g. 100MB).
Between the cost of allocation, the fact that the memory is not in cache, and the initial zeroing of
memory, [on some systems, allocation runs far slower than parsing (e.g., 1.4GB/s)](https://lemire.me/blog/2020/01/14/how-fast-can-you-allocate-a-large-block-of-memory-in-c/). Reusing the parser mitigates this by
paying the cost once, but does not eliminate it.

In large file use cases, enabling transparent huge page allocation on the OS can help a lot. We
haven't found the right way to do this on Windows or OS/X, but on Linux, you can enable transparent
huge page allocation with a command like:

```bash
echo always > /sys/kernel/mm/transparent_hugepage/enabled
```

In general, when running benchmarks over large files, we recommend that you report performance
numbers with and without huge pages if possible. Furthermore, you should amortize the parsing (e.g.,
by parsing several large files) to distinguish the time spent parsing from the time spent allocating
memory. If you are using the `parse` benchmarking tool provided with the simdjson library, you can
use the `-H` flag to omit the memory allocation cost from the benchmark results.

```
./parse largefile # includes memory allocation cost
./parse -H largefile # without memory allocation
```

Number parsing
--------------

Some JSON files contain many floating-point values. It is the case with many GeoJSON files. Accurately
parsing decimal strings into binary floating-point values with proper rounding is challenging. To
our knowledge, it is not possible, in general, to parse streams of numbers at gigabytes per second
using a single core. While using the simdjson library, it is possible that you might be limited to a
few hundred megabytes per second if your JSON documents are densely packed with floating-point values.


- When possible, you should favor integer values written without a decimal point, as it simpler and faster to parse decimal integer values.
- When serializing numbers, you should not use more digits than necessary: 17 digits is all that is needed to exactly represent double-precision floating-point numbers. Using many more digits than necessary will make your files larger and slower to parse.
- When benchmarking parsing speeds, always report whether your JSON documents are made mostly of floating-point numbers when it is the case, since number parsing can then dominate the parsing time.


Visual Studio
--------------

On Intel and AMD Windows platforms, Microsoft Visual Studio enables programmers to build either 32-bit (x86) or 64-bit (x64 or ARM64) binaries. We urge you to always use 64-bit mode. Visual Studio 2019 should default on 64-bit builds when you have a 64-bit version of Windows, which we recommend.

When compiling with Visual Studio, we recommend the flags `/Ob2 /O2` or better. We do not recommend that you compile simdjson with architecture-specific flags such as  `arch:AVX2`. The simdjson library automatically selects the best execution kernel at runtime.

Recent versions of Microsoft Visual Studio on Windows provides support for the LLVM Clang compiler. You  only need to install the "Clang compiler" optional component (ClangCL). You may also get a copy of the 64-bit LLVM CLang compiler for [Windows directly from LLVM](https://releases.llvm.org/download.html). The simdjson library fully supports the LLVM Clang compiler under Windows. In fact, you may get better performance out of simdjson with the LLVM Clang compiler than with the regular Visual Studio compiler. Meanwhile the [LLVM CLang compiler is binary compatible with Visual Studio](https://clang.llvm.org/docs/MSVCCompatibility.html) which means that you can combine their binaries (executables and libraries).

Under Windows, we also support the GNU GCC compiler via MSYS2. The performance of 64-bit MSYS2 under Windows excellent (on par with Linux).


Power Usage and Downclocking
--------------

The simdjson library relies on SIMD instructions. SIMD instructions are the public transportation of computing. Instead of using 4 distinct instructions to add numbers, you can replace them with a single instruction that does the same work. Though the one instruction is slightly more expensive, the energy used per unit of work is much less with SIMD. If you can increase your speed using SIMD instructions (NEON, SSE, AVX), you should expect to reduce your power usage.

The SIMD instructions that simdjson relies upon (SSE and AVX under x64, NEON under ARM, ALTIVEC under PPC) are routinely part of runtime libraries (e.g., [Go](https://golang.org/src/runtime/memmove_amd64.s), [Glibc](https://github.com/ihtsae/glibc/commit/5f3d0b78e011d2a72f9e88b0e9ef5bc081d18f97), [LLVM](https://github.com/llvm/llvm-project/blob/96f3ea0d21b48ca088355db10d4d1a2e9bc9f884/lldb/tools/debugserver/source/MacOSX/i386/DNBArchImplI386.cpp), [Rust](https://github.com/rust-lang/rust/commit/070fad1701fb36b112853b0a6a9787a7bb7ff34c), [Java](http://hg.openjdk.java.net/jdk8u/jdk8u/hotspot/file/c1374141598c/src/cpu/x86/vm/stubGenerator_x86_64.cpp#l1297), [PHP](https://github.com/php/php-src/blob/e5cb53ec68603d4dbdd780fd3ecfca943b4fd383/ext/standard/string.c)). What distinguishes the simdjson library is that it is built from the ground up to benefit from these instructions.


You should not expect the simdjson library to cause *downclocking* of your recent Intel CPU cores. On some Intel processors, using SIMD instructions in a sustained manner on the same CPU core may result in a phenomenon called downclocking whereas the processor initially runs these instructions at a slow speed before reducing the frequency of the core for a short time (milliseconds). Intel refers to these states as licenses. On some current Intel processors, it occurs under two scenarios:

- [Whenever 512-bit AVX-512 instructions are used](https://lemire.me/blog/2018/09/07/avx-512-when-and-how-to-use-these-new-instructions/).
- Whenever heavy 256-bit or wider instructions are used. Heavy instructions are those involving floating point operations or integer multiplications (since these execute on the floating point unit).

The simdjson library does not generally make use of heavy 256-bit instructions. On AVX2 kernels, we use vectorized multiplications, but only using 128-bit registers. On recent processors (Ice Lake/Tiger Lake or better, AMD Zen 4 or better) [no frequency throttling is found](https://travisdowns.github.io/blog/2020/08/19/icl-avx512-freq.html) due to SIMD instructions: we are thus more aggressive with SIMD on these machines. If you can still concerned, you can easily disable AVX-512 with the CMake option `SIMDJSON_AVX512_ALLOWED` set to `OFF` (e.g., `cmake -D SIMDJSON_AVX512_ALLOWED=OFF -B build && cmake --build build`) or by setting
the macro `SIMDJSON_AVX512_ALLOWED` to `0` in C++ prior to importing the headers.

You may still be worried about which SIMD instruction set is used by simdjson.  Thankfully,  [you can always determine and change which architecture-specific implementation is used](implementation-selection.md) by simdjson. Thus even if your CPU supports AVX2, you do not need to use AVX2. You are in control.
