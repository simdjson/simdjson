Performance Notes
=================

simdjson strives to be at its fastest *without tuning*, and generally achieves this. However, there
are still some scenarios where tuning can enhance performance.

* [Reusing the parser for maximum efficiency](#reusing-the-parser-for-maximum-efficiency)
  * [Keeping documents around for longer](#keeping-documents-around-for-longer)
* [Server Loops: Long-Running Processes and Memory Capacity](#server-loops-long-running-processes-and-memory-capacity)
* [Large files and huge page support](#large-files-and-huge-page-support)
* [Number parsing](#number-parsing)
* [Visual Studio](#visual-studio)
* [Downclocking](#downclocking)
* [Best Use of the DOM API](#best-use-of-the-dom-api)
* [Padding and Temporary Copies](#padding-and-temporary-copies)


Reusing the parser for maximum efficiency
-----------------------------------------

If you're using simdjson to parse multiple documents, or in a loop, you should make a parser once
and reuse it. The simdjson library will allocate and retain internal buffers between parses, keeping
buffers hot in cache and keeping memory allocation and initialization to a minimum. In this manner,
you can parse terabytes of JSON data without doing any new allocation.

```c++
dom::parser parser;

// This initializes buffers and a document big enough to handle this JSON.
dom::element doc = parser.parse("[ true, false ]"_padded);
cout << doc << endl;

// This reuses the existing buffers, and reuses and *overwrites* the old document
doc = parser.parse("[1, 2, 3]"_padded);
cout << doc << endl;

// This also reuses the existing buffers, and reuses and *overwrites* the old document
dom::element doc2 = parser.parse("true"_padded);
// Even if you keep the old reference around, doc and doc2 refer to the same document.
cout << doc << endl;
cout << doc2 << endl;
```

It's not just internal buffers though. The simdjson library reuses the document itself. The dom::element, dom::object and dom::array instances are *references* to the internal document.
You are only *borrowing* the document from simdjson, which purposely reuses and overwrites it each
time you call parse. This prevent wasteful and unnecessary memory allocation in 99% of cases where
JSON is just read, used, and converted to native values or thrown away.

> **You are only borrowing the document from the simdjson parser. Don't keep it long term!**

This is key: don't keep the `document&`, `dom::element`, `dom::array`, `dom::object`
or `string_view` objects you get back from the API. Convert them to C++ native values, structs and
arrays that you own.

Server Loops: Long-Running Processes and Memory Capacity
--------------------------------------------------------

The simdjson library automatically expands its memory capacity when larger documents are parsed, so
that you don't unexpectedly fail. In a short process that reads a bunch of files and then exits,
this works pretty flawlessly.

Server loops, though, are long-running processes that will keep the parser around forever. This
means that if you encounter a really, really large document, simdjson will not resize back down.
The simdjson library lets you adjust your allocation strategy to prevent your server from growing
without bound:

* You can set a *max capacity* when constructing a parser:

  ```c++
  dom::parser parser(1000*1000); // Never grow past documents > 1MB
  for (web_request request : listen()) {
    dom::element doc;
    auto error = parser.parse(request.body).get(doc);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { request.respond(413); continue; }
    // ...
  }
  ```

  This parser will grow normally as it encounters larger documents, but will never pass 1MB.

* You can set a *fixed capacity* that never grows, as well, which can be excellent for
  predictability and reliability, since simdjson will never call malloc after startup!

  ```c++
  dom::parser parser(0); // This parser will refuse to automatically grow capacity
  auto error = parser.allocate(1000*1000); // This allocates enough capacity to handle documents <= 1MB
  if (error) { cerr << error << endl; exit(1); }

  for (web_request request : listen()) {
    dom::element doc;
    error = parser.parse(request.body).get(doc);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { request.respond(413); continue; }
    // ...
  }
  ```

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

On Intel and AMD Windows platforms, Microsoft Visual Studio enables programmers to build either 32-bit (x86) or 64-bit (x64) binaries. We urge you to always use 64-bit mode. Visual Studio 2019 should default on 64-bit builds when you have a 64-bit version of Windows, which we recommend.

When compiling with Visual Studio, we recommend the flags `/Ob2 /O2` or better. We do not recommend that you compile simdjson with architecture-specific flags such as  `arch:AVX2`. The simdjson library automatically selects the best execution kernel at runtime.

Recent versions of Microsoft Visual Studio on Windows provides support for the LLVM Clang compiler. You  only need to install the "Clang compiler" optional component (ClangCL). You may also get a copy of the 64-bit LLVM CLang compiler for [Windows directly from LLVM](https://releases.llvm.org/download.html). The simdjson library fully supports the LLVM Clang compiler under Windows. In fact, you may get better performance out of simdjson with the LLVM Clang compiler than with the regular Visual Studio compiler. Meanwhile the [LLVM CLang compiler is binary compatible with Visual Studio](https://clang.llvm.org/docs/MSVCCompatibility.html) which means that you can combine their binaries (executables and libraries).

Under Windows, we also support the GNU GCC compiler via MSYS2. The performance of 64-bit MSYS2 under Windows excellent (on par with Linux).


Downclocking
--------------



SIMD instructions are the public transportation of computing. Instead of using 4 distinct instructions to add numbers, you can replace them with a single instruction that does the same work. Though the one instruction is slightly more expensive, the energy used per unit of work is much less with SIMD. If you can increase your speed using SIMD instructions (NEON, SSE, AVX), you should expect to reduce your power usage.

The SIMD instructions that simdjson relies upon (SSE and AVX under x64, NEON under ARM) are routinely part of runtime libraries (e.g., [Go](https://golang.org/src/runtime/memmove_amd64.s), [Glibc](https://github.com/ihtsae/glibc/commit/5f3d0b78e011d2a72f9e88b0e9ef5bc081d18f97), [LLVM](https://github.com/llvm/llvm-project/blob/96f3ea0d21b48ca088355db10d4d1a2e9bc9f884/lldb/tools/debugserver/source/MacOSX/i386/DNBArchImplI386.cpp), [Rust](https://github.com/rust-lang/rust/commit/070fad1701fb36b112853b0a6a9787a7bb7ff34c), [Java](http://hg.openjdk.java.net/jdk8u/jdk8u/hotspot/file/c1374141598c/src/cpu/x86/vm/stubGenerator_x86_64.cpp#l1297), [PHP](https://github.com/php/php-src/blob/e5cb53ec68603d4dbdd780fd3ecfca943b4fd383/ext/standard/string.c)). What distinguishes the simdjson library is that it is built from the ground up to benefit from these instructions. 


You should not expect the simdjson library to cause *downclocking* of your recent Intel CPU cores.

On some Intel processors, using SIMD instructions in a sustained manner on the same CPU core may result in a phenomenon called downclocking whereas the processor initially runs these instructions at a slow speed before reducing the frequency of the core for a short time (milliseconds). Intel refers to these states as licenses. On some current Intel processors, it occurs under two scenarios:

- [Whenever 512-bit AVX-512 instructions are used](https://lemire.me/blog/2018/09/07/avx-512-when-and-how-to-use-these-new-instructions/).
- Whenever heavy 256-bit or wider instructions are used. Heavy instructions are those involving floating point operations or integer multiplications (since these execute on the floating point unit).

The simdjson library does not currently support AVX-512 instructions and it does not make use of heavy 256-bit instructions. We do use vectorized multiplications, but only using 128-bit registers. Thus there should be no downclocking due to simdjson on recent processors. 

You may still be worried about which SIMD instruction set is used by simdjson.  Thankfully,  [you can always determine and change which architecture-specific implementation is used](implementation-selection.md) by simdjson. Thus even if your CPU supports AVX2, you do not need to use AVX2. You are in control.

Best Use of the DOM API
-------------------------

The simdjson API provides access to the JSON DOM (document-object-model) content as a tree of `dom::element` instances, each representing an object, an array or an atomic type (null, true, false, number). These `dom::element` instances are lightweight objects (e.g., spanning 16 bytes) and it might be advantageous to pass them by value, as opposed to passing them by reference or by pointer.

Padding and Temporary Copies
--------------

The simdjson function `parser.parse` reads data from a padded  buffer, containing SIMDJSON_PADDING extra bytes added at the end.
If you are passing a `padded_string` to `parser.parse` or loading the JSON directly from
disk (`parser.load`), padding is automatically  handled.
When calling `parser.parse` on a pointer (e.g., `parser.parse(my_char_pointer, my_length_in_bytes)`) a temporary copy  is made by default with adequate padding and you, again, do not need to be concerned with padding.

Some users may not be able use our `padded_string` class or to load the data directly from disk (`parser.load`). They may need to pass data pointers to the library.  If these users wish to avoid temporary copies and corresponding temporary memory allocations, they may want to call `parser.parse` with the `realloc_if_needed` parameter set to false (e.g., `parser.parse(my_char_pointer, my_length_in_bytes, false)`). In such cases, they need to ensure that there are at least SIMDJSON_PADDING extra bytes at the end that can be safely accessed and read. They do not need to initialize the padded bytes to any value in particular. The following example is safe:


```C++
const char *json      = R"({"key":"value"})";
const size_t json_len = std::strlen(json);
std::unique_ptr<char[]> padded_json_copy{new char[json_len + SIMDJSON_PADDING]};
memcpy(padded_json_copy.get(), json, json_len);
memset(padded_json_copy.get() + json_len, 0, SIMDJSON_PADDING);
simdjson::dom::parser parser;
simdjson::dom::element element = parser.parse(padded_json_copy.get(), json_len, false);
````

Setting the `realloc_if_needed` parameter `false` in this manner may lead to better performance since copies are avoided, but it requires that the user takes more responsibilities: the simdjson library cannot verify that the input buffer was padded with SIMDJSON_PADDING extra bytes.