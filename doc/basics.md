The Basics
==========

An overview of what you need to know to use simdjson, with examples.

- [The Basics](#the-basics)
  - [Requirements](#requirements)
  - [Including simdjson](#including-simdjson)
  - [Using simdjson with package managers](#using-simdjson-with-package-managers)
  - [Using simdjson as a CMake dependency](#using-simdjson-as-a-cmake-dependency)
  - [Versions](#versions)
  - [The basics: loading and parsing JSON documents](#the-basics-loading-and-parsing-json-documents)
  - [Documents are iterators](#documents-are-iterators)
    - [Parser, document and JSON scope](#parser-document-and-json-scope)
  - [string_view](#string_view)
  - [Avoiding pitfalls: enable development checks](#avoiding-pitfalls-enable-development-checks)
  - [Using the parsed JSON](#using-the-parsed-json)
    - [Using the parsed JSON: additional examples](#using-the-parsed-json-additional-examples)
  - [Adding support for custom types](#adding-support-for-custom-types)
    - [1. Specialize `simdjson::ondemand::value::get` to get custom types (pre-C++20)](#1-specialize-simdjsonondemandvalueget-to-get-custom-types-pre-c20)
    - [2. Use `tag_invoke` for custom types (C++20)](#2-use-tag_invoke-for-custom-types-c20)
  - [Minifying JSON strings without parsing](#minifying-json-strings-without-parsing)
  - [UTF-8 validation (alone)](#utf-8-validation-alone)
  - [JSON Pointer](#json-pointer)
  - [JSONPath](#jsonpath)
  - [Error handling](#error-handling)
    - [Error handling examples without exceptions](#error-handling-examples-without-exceptions)
    - [Disabling exceptions](#disabling-exceptions)
    - [Exceptions](#exceptions)
    - [Current location in document](#current-location-in-document)
    - [Checking for trailing content](#checking-for-trailing-content)
  - [Rewinding](#rewinding)
  - [Newline-Delimited JSON (ndjson) and JSON lines](#newline-delimited-json-ndjson-and-json-lines)
  - [Parsing numbers inside strings](#parsing-numbers-inside-strings)
  - [Dynamic Number Types](#dynamic-number-types)
  - [Raw strings from keys](#raw-strings-from-keys)
  - [General direct access to the raw JSON string](#general-direct-access-to-the-raw-json-string)
  - [Storing directly into an existing string instance](#storing-directly-into-an-existing-string-instance)
  - [Thread safety](#thread-safety)
  - [Standard compliance](#standard-compliance)
  - [Backwards compatibility](#backwards-compatibility)
  - [Examples](#examples)
  - [Performance tips](#performance-tips)
  - [Further reading](#further-reading)


Requirements
------------------

- A recent compiler (LLVM clang 6 or better, GNU GCC 7.4 or better, Xcode 11 or better) on a 64-bit (PPC, ARM or x64 Intel/AMD) POSIX systems such as macOS, freeBSD or Linux. We require that the compiler supports the C++11 standard or better.
- Visual Studio 2017 or better under 64-bit Windows. Users should target a 64-bit build (x64 or ARM64) instead of a 32-bit build (x86). We support the LLVM clang compiler under Visual Studio (clangcl) as well as as the regular Visual Studio compiler. We also support MinGW 64-bit under Windows.


Support for AVX-512 require a processor with AVX512-VBMI2 support (Ice Lake or better, AMD Zen 4 or better) under a 64-bit system and a recent compiler (LLVM clang 6 or better, GCC 8 or better, Visual Studio 2019 or better). You need a correspondingly recent assembler such as gas (2.30+) or nasm (2.14+): recent compilers usually come with recent assemblers. If you mix a recent compiler with an incompatible/old assembler (e.g., when using a recent compiler with an old Linux distribution), you may get errors at build time because the compiler produces instructions that the assembler does not recognize: you should update your assembler to match your compiler (e.g., upgrade binutils to version 2.30 or better under Linux) or use an older compiler matching the capabilities of your assembler.

We test the library on a big-endian system (IBM s390x with Linux).

Including simdjson
------------------

To include simdjson, copy [simdjson.h](/singleheader/simdjson.h) and [simdjson.cpp](/singleheader/simdjson.cpp)
into your project. Then include it in your project with:

```c++
#include "simdjson.h"
using namespace simdjson; // optional
```

Under most systems, you can compile with:

```
c++ myproject.cpp simdjson.cpp
```

Note:
- We recommend that you use simdjson by copying the single-header `simdjson.h` file along with the source file `simdjson.cpp` directly in your project, as they are part of [every release](https://github.com/simdjson/simdjson/releases) as assets. In this manner, you only have to compile `simdjson.cpp` as any other source file: it works well in every development environment. However, you may also use simdjson as a git submodule ([example](https://github.com/simdjson/cmakedemo)), using FetchContent ([example](https://github.com/simdjson/cmake_demo_single_file)), with ExternalProject_Add ([example](https://github.com/simdjson/cmakedemo_externalproject)) or with CPM ([example](https://github.com/cpm-cmake/CPM.cmake/tree/master/examples/simdjson)).
- Users on macOS and other platforms where default compilers do not provide C++11 compliant by default should request it with the appropriate flag (e.g., `c++ -std=c++11 myproject.cpp simdjson.cpp`).
- The library relies on [runtime CPU detection](implementation-selection.md): avoid specifying an architecture at compile time (e.g., `-march-native`) if you want your binaries to run everywhere.

Using simdjson with package managers
------------------

You can install the simdjson library on your system or in your project using multiple package managers such as MSYS2, the conan package manager, vcpkg, brew, the apt package manager (debian-based Linux systems), the FreeBSD package manager (FreeBSD), and so on. E.g., [we provide an complete example with vcpkg](https://github.com/simdjson/simdjson-vcpkg) that works under Windows. [Visit our wiki for more details](https://github.com/simdjson/simdjson/wiki/Installing-simdjson-with-a-package-manager).



The following Linux distributions provide simdjson packages: Alpine, RedHat, Rocky Linux, Debian, Fedora, and Ubuntu.

Using simdjson as a CMake dependency
------------------

You can include the simdjson library as a CMake dependency by including the following lines in your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.6.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(simdjson)
```

You should provide `GIT_TAG` with the release you need. If you omit `GIT_TAG  ...`, you will work from the main branch of simdjson: we recommend that if you are working on production code, you always work from a release.

Elsewhere in your project, you can declare dependencies on simdjson with lines such as these:

```cmake
add_executable(myprogram myprogram.cpp)
target_link_libraries(myprogram simdjson)
```

We recommend CMake version 3.15 or better.

See [our CMake demonstration](https://github.com/simdjson/cmake_demo_single_file). It works under Linux, FreeBSD, macOS and Windows (including Visual Studio).

The CMake build in simdjson can be tailored with a few variables. You can see the available variables and their default values by entering the `cmake -LA` command.


Versions
------------------

Users are discouraged from building production code from the
project's main branch. The main branch is used for development:
it may contain new features but also additional bugs.

Users should pick a release. They should also access the
documentation matching the release that they have chosen.
Note that new features may be added over time.

Our releases are tagged using semantic versioning: the tags
are made of three numbers prefixed by the letter `v` and separated by periods.

You can always find the latest release at the following hyperlink:

https://github.com/simdjson/simdjson/releases/latest/

The archive you download at this location contains its own corresponding
documentation.

You can also choose to browse a specific version
of the documentation and the code using GitHub,
by appending the version number to the hyperlink, like so:

https://github.com/simdjson/simdjson/blob/vx.y.z/doc/basics.md

where `x.y.z` should correspond to the version number you have
chosen.

The basics: loading and parsing JSON documents
----------------------------------------------

The simdjson library allows you to navigate and validate JSON documents ([RFC 8259](https://www.tbray.org/ongoing/When/201x/2017/12/14/rfc8259.html)).
As required by the standard, your JSON document should be in a Unicode (UTF-8) string. The whole
string, from the beginning to the end, needs to be valid: we do not attempt to tolerate bad
inputs before or after a document.

For efficiency reasons, simdjson requires a string with a few bytes (`simdjson::SIMDJSON_PADDING`)
at the end, these bytes may be read but their content does not affect the parsing. In practice,
it means that the JSON inputs should be stored in a memory region with `simdjson::SIMDJSON_PADDING`
extra bytes at the end. You do not have to set these bytes to specific values though you may
want to if you want to avoid runtime warnings with some sanitizers. Advanced users may want to
read the section Free Padding in [our performance notes](performance.md).

The simdjson library offers a tree-like [API](https://en.wikipedia.org/wiki/API), which you can
access by creating a `ondemand::parser` and calling the `iterate()` method. The iterate method
quickly indexes the input string and may detect some errors. The following example illustrates
how to get started with an input JSON file (`"twitter.json"`):

```c++
ondemand::parser parser;
auto json = padded_string::load("twitter.json"); // load JSON file 'twitter.json'.
ondemand::document doc = parser.iterate(json); // position a pointer at the beginning of the JSON data
```

You can also create a padded string---and call `iterate()`:

```c++
ondemand::parser parser;
auto json = "[1,2,3]"_padded; // The _padded suffix creates a simdjson::padded_string instance
ondemand::document doc = parser.iterate(json); // parse a string
```

If you have a buffer of your own with enough padding already (SIMDJSON_PADDING extra bytes allocated), you can use `padded_string_view` to pass it in:

```c++
ondemand::parser parser;
char json[3+SIMDJSON_PADDING];
strcpy(json, "[1]");
ondemand::document doc = parser.iterate(json, strlen(json), sizeof(json));
```

The simdjson library will also accept `std::string` instances. If the provided
reference is non-const, it will allocate padding as needed.

You can copy your data directly on a `simdjson::padded_string` as follows:

```c++
const char * data = "my data"; // 7 bytes
simdjson::padded_string my_padded_data(data, 7); // copies to a padded buffer
```

Or as follows...

```c++
std::string data = "my data";
simdjson::padded_string my_padded_data(data); // copies to a padded buffer
```


We recommend against creating many `std::string` or many `std::padding_string` instances in your application to store your JSON data.
Consider reusing the same buffers and limiting memory allocations.

By default, the simdjson library throws exceptions (`simdjson_error`) on errors. We omit `try`-`catch` clauses from our illustrating examples: if you omit `try`-`catch` in your code, an uncaught exception will halt your program. It is also possible to use simdjson without generating exceptions, and you may even build the library without exception support at all. See [Error handling](#error-handling) for details.


Some users may want to browse code along with the compiled assembly. You want to check out the following lists of examples:

* [simdjson examples with errors handled through exceptions](https://godbolt.org/z/98Kx9Kqjn)
* [simdjson examples with errors without exceptions](https://godbolt.org/z/PKG7GdbPo)

*Windows-specific*:  Windows users who need to read files with
non-ANSI characters in the name should set their code page to
UTF-8 (65001). This should be the default with Windows 11 and better.
Further, they may use the AreFileApisANSI function to determine whether
the filename is interpreted using the ANSI or the system default OEM
codepage, and they may call SetFileApisToOEM accordingly.

Documents are iterators
-----------------------

The simdjson library relies on an approach to parsing JSON that we call "On-Demand".
A `document` is *not* a fully-parsed JSON value; rather, it is an **iterator** over the JSON text.
This means that while you iterate an array, or search for a field in an object, it is actually
walking through the original JSON text, merrily reading commas and colons and brackets to make sure
you get where you are going. This is the key to On-Demand's performance: since it's just an iterator,
it lets you parse values as you use them. And particularly, it lets you *skip* values you do not want
to use. On-Demand is also ideally suited when you want to capture part of the document without parsing it
immediately (e.g., see [General direct access to the raw JSON string](#general-direct-access-to-the-raw-json-string)).

We refer to "On-Demand" as a front-end component since it is an interface between the
low-level parsing functions and the user. It hides much of the complexity of parsing JSON
documents.

### Parser, document and JSON scope

For code safety, you should keep (1) the `parser` instance, (2) the input string and (3) the document instance alive throughout your parsing. Additionally, you should follow the following rules:

- A `parser` may have at most one document open at a time, since it holds allocated memory used for the parsing.
- By design, you should only have one `document` instance per JSON document. Thus, if you must pass a document instance to a function, you should avoid passing it by value: choose to pass it by reference instance to avoid the copy. In any case, the `document` class does not have a copy constructor.

During the `iterate` call, the original JSON text is never modified--only read. After you are done
with the document, the source (whether file or string) can be safely discarded.

For best performance, a `parser` instance should be reused over several files: otherwise you will
needlessly reallocate memory, an expensive process. It is also possible to avoid entirely memory
allocations during parsing when using simdjson. [See our performance notes for details](performance.md).

If you need to have several documents active at once, you should have several parser instances.

string_view
-------------

The simdjson library builds on compilers supporting the [C++11 standard](https://en.wikipedia.org/wiki/C%2B%2B11).
It is also a strict requirement: we have no plan to support older C++ compilers.

We represent parsed Unicode (UTF-8) strings in simdjson using the `std::string_view` class. It avoids
the need to copy the data, as would be necessary with the `std::string` class. It also
avoids the pitfalls of null-terminated C strings. It makes it easier for our users to
copy the data into their own favorite class instances (e.g., alternatives to `std::string`).

A `std::string_view` instance is effectively just a pointer to a region in memory representing
a string. In simdjson, we return `std::string_view` instances that either point within the
input string you parsed (see [General direct access to the raw JSON string](#general-direct-access-to-the-raw-json-string)), or to a temporary string buffer inside
our parser class instances that is valid until the parser object is destroyed or you use it to parse another document.
When using `std::string_view` instances, it is your responsibility to ensure that
`std::string_view` instance does not outlive the pointed-to memory (e.g., either the input
buffer or the parser instance). Furthermore, some operations reset the string buffer
inside our parser instances: e.g., when we parse a new document. Thus a `std::string_view` instance
is often best viewed as a temporary string value that is tied to the document you are parsing.
At the cost of some memory allocation, you may convert your `std::string_view` instances for long-term storage into `std::string` instances:
`std::string mycopy(view)` (C++17) or  `std::string mycopy(view.begin(), view.end())` (prior to C++17).
For convenience, we also allow [storing an escaped string directly into an existing string instance](#storing-directly-into-an-existing-string-instance).

The `std::string_view` class has become standard as part of C++17 but it is not always available
on compilers which only supports C++11. When we detect that `string_view` is natively
available, we define the macro `SIMDJSON_HAS_STRING_VIEW`.

When we detect that it is unavailable,
we use [string-view-lite](https://github.com/martinmoene/string-view-lite) as a
substitute. In such cases, we use the type alias `using string_view = nonstd::string_view;` to
offer the same API, irrespective of the compiler and standard library. The macro
`SIMDJSON_HAS_STRING_VIEW` will be *undefined* to indicate that we emulate `string_view`.

Some users prefer to use non-JSON native encoding formats such as UTF-16 or UTF-32. Users may
transcode the UTF-8 strings produced by the simdjson library to other formats. See the
[simdutf library](https://github.com/simdutf/simdutf), for example.

Avoiding pitfalls: enable development checks
--------------------

We recommend that you first compile and run your code in Debug mode:

- under Visual Studio, it means having the `_DEBUG` macro defined,
-  for other compilers, it means leaving the `__OPTIMIZE__` macro undefined.

The simdjson code will set `SIMDJSON_DEVELOPMENT_CHECKS=1` in debug mode.
Alternatively, you can set the macro `SIMDJSON_DEVELOPMENT_CHECKS` to 1 prior to including
the `simdjson.h` header to enable these additional checks: just make sure you remove the
definition once your code has been tested. When `SIMDJSON_DEVELOPMENT_CHECKS` is set to 1, the
simdjson library runs additional (expensive) tests on your code to help ensure that you are
using the library in a safe manner.

Once your code has been tested, you can then run it in
Release mode: under Visual Studio, it means having the `_DEBUG` macro undefined, and, for other
compilers, it means setting `__OPTIMIZE__` to a positive integer. You can also forcefully
disable these checks by setting `SIMDJSON_DEVELOPMENT_CHECKS` to 0. Once your code is tested, we
further encourage you to define `NDEBUG` in your Release builds to disable additional runtime
testing and get the best performance.

Using the parsed JSON
---------------------

Once you have a document (`simdjson::ondemand::document`), you can navigate it with
idiomatic C++ iterators, operators and casts. Besides the document instances and
native types (`double`, `uint64_t`, `int64_t`, `bool`), we also access
Unicode (UTF-8) strings (`std::string_view`), objects (`simdjson::ondemand::object`)
and arrays (`simdjson::ondemand::array`).
We also have a generic ephemeral type (`simdjson::ondemand::value`) which represents a potential
array or object, or scalar type (`double`, `uint64_t`, `int64_t`, `bool`, `null`, string) inside
an array or an object. Both generic types (`simdjson::ondemand::document` and
`simdjson::ondemand::value`) have a `type()` method returning a `json_type` value describing the
value (`json_type::array`, `json_type::object`, `json_type::number`, `json_type::string`,
`json_type::boolean`, `json_type::null`). A generic value (`simdjson::ondemand::value`)
is only valid temporarily, as soon as you access other values, other keys in objects, etc.
it becomes invalid: you should therefore consume the value immediately by converting it to a
scalar type, an array or an object.

Advanced users who need to determine the number types (integer or float) dynamically,
should review our section [dynamic number types](#dynamic-number-types). Indeed,
we have an additional `ondemand::number` type which may represent either integers
or floating-point values, depending on how the numbers are formatted.
floating-point values followed by an integer.

We invite you to keep the following rules in mind:
1. While you are accessing the document, the `document` instance should remain in scope: it is your "iterator" which keeps track of where you are in the JSON document. By design, there is one and only one `document` instance per JSON document.
2. Because On-Demand is really just an iterator, you must fully consume the current object or array before accessing a sibling object or array.
3. Values can only be consumed once, you should get the values and store them if you plan to need them multiple times. You are expected to access the keys of an object just once. You are expected to go through the values of an array just once.

The simdjson library makes generous use of `std::string_view` instances. If you are unfamiliar
with `std::string_view` in C++, make sure to [read the section on std::string_view](#string_view).
They behave much like an immutable `std::string` but they require no memory allocation. You can
create a `std::string` instance from a `std::string_view` when you need it.

The following specific instructions indicate how to use the JSON when exceptions are enabled, but simdjson has full, idiomatic
support for users who avoid exceptions. See [the simdjson error handling documentation](basics.md#error-handling) for more.

* **Validate What You Use:** When calling `iterate`, the document is quickly indexed. If it is
  not a valid Unicode (UTF-8) string or if there is an unclosed string, an error may be reported right away.
  However, it is not fully validated. On-Demand only fully validates the values you use and the
  structure leading to it. It means that at every step as you traverse the document, you may encounter an error. You can handle errors either with exceptions or with error codes.
* **Extracting Values:** You can cast a JSON element to a native type:
  `double(element)`. This works for `std::string_view`, double, uint64_t, int64_t, bool,
  ondemand::object and ondemand::array. We also have explicit methods such as `get_string()`, `get_double()`,
  `get_uint64()`, `get_int64()`, `get_bool()`, `get_object()` and `get_array()`. After a cast or an explicit method,
  the number, string or boolean will be parsed, or the initial `{` or `[` will be verified for `ondemand::object` and `ondemand::array`. An exception may be thrown if
  the cast is not possible: there error code is `simdjson::INCORRECT_TYPE` (see [Error handling](#error-handling)). Importantly, when getting an ondemand::object or ondemand::array instance, its content is
  not validated: you are only guaranteed that the corresponding initial character (`{` or `[`) is present. Thus,
  for example, you could have an ondemand::object instance pointing at the invalid JSON `{ "this is not a valid object" }`: the validation occurs as you access the content.
  The `get_string()` returns a valid UTF-8 string, after
  unescaping characters as needed: unmatched surrogate pairs are treated as an error unless you
  pass `true` (`get_string(true)`) as a parameter to get replacement characters where errors
  occur. If you somehow need to access non-UTF-8 strings in a lossless manner
  (e.g., if you strings contain unpaired surrogates), you may use the `get_wobbly_string()` function to get a string in the [WTF-8 format](https://simonsapin.github.io/wtf-8).
  When calling `get_uint64()` and `get_int64()`, if the number does not fit in a corresponding
  64-bit integer type, it is also considered an error. When parsing numbers or other scalar values, the library checks
  that the value is followed by an expected character, thus you *may* get a number parsing error when accessing the digits
  as an integer in the following strings: `{"number":12332a`, `{"number":12332\0`, `{"number":12332` (the digits appear at the end). We always abide by the [RFC 8259](https://www.tbray.org/ongoing/When/201x/2017/12/14/rfc8259.html) JSON specification so that, for example, numbers prefixed by the `+` sign are in error.

  > IMPORTANT NOTE: values can only be parsed once. Since documents are *iterators*, once you have
  > parsed a value (such as by casting to double), you cannot get at it again. It is an error to call
  > `get_string()` twice on an object (or to cast an object twice to `std::string_view`).
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. This will
  step through each value in the JSON array.

  To iterate through an array, you should be at the beginning
  of the array: to warn you, an OUT_OF_ORDER_ITERATION error is generated [when development checks](#avoiding-pitfalls-enable-development-checks) are active. If you need to access an array more
  than once, you may call `reset()` on it although we discourage this practice. Keep in mind that
  you should consume each value at most once.

  If you know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`.

  You may also use explicit iterators: `for(auto i = array.begin(); i != array.end(); i++) {}`. You can check that an array is empty with the condition `auto i = array.begin(); if (i == array.end()) {...}`.
* **Object Iteration:** You can iterate through an object's fields, as well: `for (auto field : object) { ... }`. You may also use explicit iterators : `for(auto i = object.begin(); i != object.end(); i++) { auto field = *i; .... }`. You can check that an object is empty with the condition `auto i = object.begin(); if (i == object.end()) {...}`.
  - `field.unescaped_key()` will get you the unescaped key string as a `std::string_view` instance. E.g., the JSON string `"\u00e1"` becomes the Unicode string `á`. Optionally,  you pass `true` as a parameter to the `unescaped_key` method if you want invalid escape sequences to be replaced by a default replacement character (e.g., `\ud800\ud801\ud811`): otherwise bad escape sequences lead to an immediate error.
  - `field.escaped_key()` will get you the key string as  as a `std::string_view` instance, but unlike `unescaped_key()`, the key is not processed, so no unescaping is done. E.g., the JSON string `"\u00e1"` becomes the Unicode string `\u00e1`. We expect that `escaped_key()` is faster than `field.unescaped_key()`.
  - `field.value()` will get you the value, which you can then use all these other methods on.


  To iterate through an object, you should be at the beginning
  of the object: to warn you, an OUT_OF_ORDER_ITERATION error is generated [when development checks](#avoiding-pitfalls-enable-development-checks) are active. If you need to access an object more
  than once, you may call `reset()` on it although we discourage this practice. Keep in mind that
  you should consume each value at most once.
* **Array Index:** Because it is forward-only, you cannot look up an array element by index by index. Instead,
  you should iterate through the array and keep an index yourself. Exceptionally, if need a single value
  out of the array, you may use an array access (e.g., `array[1]`).
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`. This will
  scan through the object looking for the field with the matching string, doing a character-by-character
  comparison. It may generate the error `simdjson::NO_SUCH_FIELD` if there is no such key in the object, it may throw an exception (see [Error handling](#error-handling)). For efficiency reason, you should avoid looking up the same field repeatedly: e.g., do
  not do `object["foo"]` followed by `object["foo"]` with the same `object` instance. For best performance, you should try to query the keys in the same order they appear in the document. If you need several keys and you cannot predict the order they will appear in, it is recommended to iterate through all keys `for(auto field : object) {...}`. Generally, you should not mix and match iterating through an object (`for(auto field : object) {...}`) and key accesses (`object["foo"]`): if you need to iterate through an object after a key access, you need to call `reset()` on the object. Whenever you call `reset()`, you need to keep in mind that though you can iterate over the array repeatedly, values should be consumedonly once (e.g., repeatedly calling `unescaped_key()` on the same  key is forbidden). Keep in mind that On-Demand does not buffer or save the result of the parsing: if you repeatedly access `object["foo"]`, then it must repeatedly seek the key and parse the content. The library does not provide a distinct function to check if a key is present, instead we recommend you attempt to access the key: e.g., by doing `ondemand::value val{}; if (!object["foo"].get(val)) {...}`, you have that `val` contains the requested value inside the if clause.  It is your responsibility as a user to temporarily keep a reference to the value (`auto v = object["foo"]`), or to consume the content and store it in your own data structures. If you consume an
  object twice: `std::string_view(object["foo"]` followed by `std::string_view(object["foo"]` then your code
  is in error. Furthermore, you can only consume one field at a time, on the same object. The
  value instance you get from  `content["bids"]` becomes invalid when you call `content["asks"]`.
  If you have retrieved `content["bids"].get_array()` and you later call
  `content["asks"].get_array()`, then the first array should no longer be accessed: it would be
  unsafe to do so. You can detect such mistakes by first compiling and running the code [with development checks](#avoiding-pitfalls-enable-development-checks): an OUT_OF_ORDER_ITERATION error is generated.

  > NOTE: JSON allows you to escape characters in keys. E.g., the key `"date"` may be written as
  > `"\u0064\u0061\u0074\u0065"`. By default, simdjson does *not* unescape keys when matching.
  > Thus if you search for the key `"date"` and the JSON document uses `"\u0064\u0061\u0074\u0065"`
  > as a key, it will not be recognized. This is not generally a problem.  Nevertheless, if you do need
  > to support escaped keys, the method `unescaped_key()` provides the desired unescaped keys by
  > parsing and writing out the unescaped keys to a string buffer and returning a `std::string_view`
  > instance. The `unescaped_key` takes an optional Boolean value: passing it true will decode invalid
  > Unicode sequences with replacement, meaning that the decoding always succeeds but bogus Unicode
  > replacement characters are inserted. In general, you should expect a performance penalty
  > when using `unescaped_key()` compared to `key()` because of the string processing: the `key()`
  > function just points inside the source JSON document. As a compromise, you may use `escaped_key()``
  > which returns a `std::string_view` instance pointing directly in the document, like `key()`, although,
  > unlike `key()`, it has to determine the location of the final quote character.
  >
  > ```c++
  > auto json = R"({"k\u0065y": 1})"_padded;
  > ondemand::parser parser;
  > auto doc = parser.iterate(json);
  > ondemand::object object = doc.get_object();
  > for(auto field : object) {
  >    // parses and writes out the key, after unescaping it,
  >    // to a string buffer. It causes a performance penalty.
  >    // If you do not expect that unescaping is useful, you
  >    // may replace field.unescaped_key() with
  >    // field.escaped_key().
  >    std::string_view keyv = field.unescaped_key();
  >    if (keyv == "key") { std::cout << uint64_t(field.value()); }
  >  }
  > ```
  >
  > By default, field lookup is order-insensitive, so you can look up values in any order. However,
  > we still encourage you to look up fields in the order you expect them in the JSON, as it is still
  > faster.
  >
  > If you want to enforce finding fields in order, you can use `object.find_field("foo")` instead.
  > This will only look forward, and will fail to find fields in the wrong order: for example, this
  > will fail:
  >
  > ```c++
  > ondemand::parser parser;
  > auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
  > auto doc = parser.iterate(json);
  > double y = doc.find_field("y"); // The cursor is now after the 2 (at })
  > double x = doc.find_field("x"); // This fails, because there are no more fields after "y"
  > ```
  >
  > By contrast, using the default (order-insensitive) lookup succeeds:
  >
  > ```c++
  > ondemand::parser parser;
  > auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
  > auto doc = parser.iterate(json);
  > double y = doc["y"]; // The cursor is now after the 2 (at })
  > double x = doc["x"]; // Success: [] loops back around to find "x"
  > ```
* **Output to strings:** Given a document, a value, an array or an object in a JSON document, you can output a JSON string version suitable to be parsed again as JSON content: `simdjson::to_json_string(element)`. A call to `to_json_string` consumes fully the element: if you apply it on a document, the JSON pointer is advanced to the end of the document. The `simdjson::to_json_string` does not allocate memory. The `to_json_string` function should not be confused with retrieving the value of a string instance which are escaped and represented using a lightweight `std::string_view` instance pointing at an internal string buffer inside the parser instance. To illustrate, the first of the following two code segments will print the unescaped string `"test"` complete with the quote whereas the second one will print the escaped content of the string (without the quotes).
  > ```C++
  > // serialize a JSON to an escaped std::string instance so that it can be parsed again as JSON
  > auto silly_json = R"( { "test": "result"  }  )"_padded;
  > ondemand::document doc = parser.iterate(silly_json);
  > std::cout << simdjson::to_json_string(doc["test"]) << std::endl; // Requires simdjson 1.0 or better
  >````
  > ```C++
  > // retrieves an unescaped string value as a string_view instance
  > auto silly_json = R"( { "test": "result"  }  )"_padded;
  > ondemand::document doc = parser.iterate(silly_json);
  > std::cout << std::string_view(doc["test"]) << std::endl;
  >````
  You can use `to_json_string` to efficiently extract components of a JSON document to reconstruct a new JSON document, as in the following example:
  > ```C++
  > auto cars_json = R"( [
  >   { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  >   { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  >   { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  > ] )"_padded;
  > std::vector<std::string_view> arrays;
  > // We are going to collect string_view instances which point inside the `cars_json` string
  > // and are therefore valid as long as `cars_json` remains in scope.
  > {
  >   ondemand::parser parser;
  >   for (ondemand::object car : parser.iterate(cars_json)) {
  >     if (uint64_t(car["year"]) > 2000) {
  >       arrays.push_back(simdjson::to_json_string(car["tire_pressure"]));
  >     }
  >   }
  > }
  > // We can now convert to a JSON string:
  > std::ostringstream oss;
  > oss << "[";
  > for(size_t i = 0; i < arrays.size(); i++) {
  >   if (i>0) { oss << ","; }
  >   oss << arrays[i];
  > }
  > oss << "]";
  > auto json_string = oss.str();
  > // json_string == "[[ 40.1, 39.9, 37.7, 40.4 ],[ 30.1, 31.0, 28.6, 28.7 ]]"
  >````
* **Extracting Values (without exceptions):** You can use a variant usage of `get()` with error
  codes to avoid exceptions. You first declare the variable of the appropriate type (`double`,
  `uint64_t`, `int64_t`, `bool`, `ondemand::object` and `ondemand::array`) and pass it by reference
  to `get()` which gives you back an error code: e.g.,

  ```c++
  auto abstract_json = R"(
    { "str" : { "123" : {"abc" : 3.14 } } }
  )"_padded;
  ondemand::parser parser;

  double value;
  auto doc = parser.iterate(abstract_json);
  auto error = doc["str"]["123"]["abc"].get(value);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  cout << value << endl; // Prints 3.14
  ```
  This examples also show how we can string several operations and only check for the error once, a strategy we call  *error chaining*.
  Though error chaining makes the code very compact, it also makes error reporting less precise: in this instance, you may get the
  same error whether the field "str", "123" or "abc" is missing. If you need to break down error handling per operation, avoid error chaining. Furthermore, you should be mindful that chaining that harm performance by encouraging redundancies: writing both `doc["str"]["123"]["abc"].get(value)` and `doc["str"]["123"]["zyw"].get(value)` in the same program may force multiple accesses to the same keys (`"str"` and `"123"`).
* **Counting elements in arrays:** Sometimes it is useful to scan an array to determine its length prior to parsing it.
  For this purpose, `array` instances have a `count_elements` method. Users should be
  aware that the `count_elements` method can be costly since it requires scanning the
  whole array. You should only call `count_elements` as a last resort as it may
  require scanning the document twice or more. You may use it as follows if your document is itself an array:

  ```C++
  auto cars_json = R"( [ 40.1, 39.9, 37.7, 40.4 ] )"_padded;
  auto doc = parser.iterate(cars_json);
  size_t count = doc.count_elements(); // requires simdjson 1.0 or better
  std::vector<double> values(count);
  size_t index = 0;
  for(double x : doc) { values[index++] = x; }
  ```
  If you access an array inside a document, you can use the `count_elements` method as follow.
  You should not let the array instance go out of scope before consuming it after calling the `count_elements` method:
  ``` C++
  ondemand::parser parser;
  auto cars_json = R"( { "test":[ { "val1":1, "val2":2 }, { "val1":1, "val2":2 } ] }   )"_padded;
  auto doc = parser.iterate(cars_json);
  auto test_array = doc.find_field("test").get_array();
  size_t count = test_array.count_elements(); // requires simdjson 1.0 or better
  std::cout << "Number of elements: " <<  count << std::endl;
  for(ondemand::object elem: test_array) {
     std::cout << simdjson::to_json_string(elem);
  }
  ```
* **Counting fields in objects:** Other times, it is useful to scan an object to determine the number of fields prior to
  parsing it.
  For this purpose, `object` instances have a `count_fields` method. Again, users should be
  aware that the `count_fields` method can be costly since it requires scanning the
  whole objects. You should only call `count_fields` as a last resort as it may
  require scanning the document twice or more.  You may use it as follows if your document is itself an object:

  ```C++
  ondemand::parser parser;
  auto json = R"( { "test":{ "val1":1, "val2":2 } }   )"_padded;
  auto doc = parser.iterate(json);
  size_t count = doc.count_fields(); // requires simdjson 1.0 or better
  std::cout << "Number of fields: " <<  count << std::endl; // Prints "Number of fields: 1"
  ```
  Similarly to `count_elements`, you should not let an object instance go out of scope before consuming it after calling
  the `count_fields` method. If you access an object inside a document, you can use the `count_fields` method as follow.
  ``` C++
  ondemand::parser parser;
  auto json = R"( { "test":{ "val1":1, "val2":2 } }   )"_padded;
  auto doc = parser.iterate(json);
  auto test_object = doc.find_field("test").get_object();
  size_t count = test_object.count_fields(); // requires simdjson 1.0 or better
  std::cout << "Number of fields: " <<  count << std::endl; // Prints "Number of fields: 2"
  ```
* **Tree Walking and JSON Element Types:** Sometimes you don't necessarily have a document
  with a known type, and are trying to generically inspect or walk over JSON elements.
  You can also represent arbitrary JSON values with
  `ondemand::value` instances: it can represent anything except a scalar document (lone number, string, null or Boolean). You can check for scalar documents with the method `scalar()`. You can cast a document that is either an array or an object to an `ondemand::value` instance immediately after you create the document instance: you cannot create an `ondemand::value` instance from a document that has already been accessed as it would mean that you would have two instances of the object or array simultaneously (see [rewinding](#rewinding)). You can query the type of a document or a value with the `type()` method. The `type()` method does not consume or validate documents and values, but it tells you whether they are
  - arrays (`json_type::array`),
  - objects (`json_type::object`)
  - numbers (`json_type::number`),
  - strings (`json_type::string`),
  - Booleans (`json_type::boolean`),
  - null (`json_type::null`).

  You must still validate and consume the values (e.g., call `is_null()`) after calling `type()`.
   You may also access [the raw JSON string](#general-direct-access-to-the-raw-json-string).
  For example, the following is a quick and dirty recursive function that verbosely prints the JSON document as JSON. This example also illustrates lifecycle requirements: the `document` instance holds the iterator. The document must remain in scope while you are accessing instances of `value`, `object` and `array`.
  ```c++
  void recursive_print_json(ondemand::value element) {
    bool add_comma;
    switch (element.type()) {
    case ondemand::json_type::array:
      cout << "[";
      add_comma = false;
      for (auto child : element.get_array()) {
        if (add_comma) {
          cout << ",";
        }
        // We need the call to value() to get
        // an ondemand::value type.
        recursive_print_json(child.value());
        add_comma = true;
      }
      cout << "]";
      break;
    case ondemand::json_type::object:
      cout << "{";
      add_comma = false;
      for (auto field : element.get_object()) {
        if (add_comma) {
          cout << ",";
        }
        // key() returns the key as it appears in the raw
        // JSON document, if we want the unescaped key,
        // we should do field.unescaped_key().
        // We could also use field.escaped_key() if we want
        // a std::string_view instance, but we do not need
        // escaping.
        cout << "\"" << field.key() << "\": ";
        recursive_print_json(field.value());
        add_comma = true;
      }
      cout << "}\n";
      break;
    case ondemand::json_type::number:
      // assume it fits in a double
      cout << element.get_double();
      break;
    case ondemand::json_type::string:
      // get_string() would return escaped string, but
      // we are happy with unescaped string.
      cout << "\"" << element.get_raw_json_string() << "\"";
      break;
    case ondemand::json_type::boolean:
      cout << element.get_bool();
      break;
    case ondemand::json_type::null:
      // We check that the value is indeed null
      // otherwise: an error is thrown.
      if (element.is_null()) {
        cout << "null";
      }
      break;
    }
  }
  void basics_treewalk() {
    padded_string json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::value val = doc;
    recursive_print_json(val);
    std::cout << std::endl;
  }
  ```

### Using the parsed JSON: additional examples


Let us review these concepts with some additional examples. For simplicity, we omit the include clauses (`#include "simdjson.h"`) as well as namespace-using clauses (`using namespace simdjson;`).

The first example illustrates how we can chain operations. In this instance, we repeatedly select keys using the bracket operator (`doc["str"]`) and then finally request a number (using `get_double()`). It is safe to write code in this manner: if any step causes an error, the error status propagates and an exception is thrown at the end. You do not need to constantly check for errors.

```C++
auto abstract_json = R"(
  { "str" : { "123" : {"abc" : 3.14 } } }
)"_padded;
ondemand::parser parser;
auto doc = parser.iterate(abstract_json);
cout << doc["str"]["123"]["abc"].get_double() << endl; // Prints 3.14
```

In the following example, we start with a JSON document that contains
an array of objects. We iterate through the objects using a for-loop. Within each object, we use
the bracket operator (e.g., `car["make"]`) to select values. We also show how we can iterate through an
array, corresponding to the key `tire_pressure`,  that is contained inside each object.

```c++
ondemand::parser parser;
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;

// Iterating through an array of objects
for (ondemand::object car : parser.iterate(cars_json)) {
  // Accessing a field by name
  cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;

  // Casting a JSON element to an integer
  uint64_t year = car["year"];
  cout << "- This car is " << 2020 - year << "years old." << endl;

  // Iterating through an array of floats
  double total_tire_pressure = 0;
  for (double tire_pressure : car["tire_pressure"]) {
    total_tire_pressure += tire_pressure;
  }
  cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
}
```


The previous example had an array of objects, but we can use essentially the same
approach with an object of objects.

```c++
ondemand::parser parser;
auto cars_json = R"( {
  "identifier1":{ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  "identifier2":{ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  "identifier3":{ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
} )"_padded;
// Iterating through an array of objects
ondemand::document doc = parser.iterate(cars_json);
for (ondemand::field key_car : doc.get_object()) {
  // If I need a string_view and/or, I can use key_car.unescaped_key() instead, but
  // key_car.key() will be more performant otherwise.
  // If we want a std::string_view instance but we do not care about escaping, we
  // can also use key_car.escaped_key().
  cout << "identifier : " << key_car.key() << std::endl;
  // I can now access the subobject:
  ondemand::object car = key_car.value();
  // Accessing a field by name
  cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;

  // Casting a JSON element to an integer
  uint64_t year = car["year"];
  cout << "- This car is " << 2020 - year << "years old." << endl;

  // Iterating through an array of floats
  double total_tire_pressure = 0;
  for (double tire_pressure : car["tire_pressure"]) {
    total_tire_pressure += tire_pressure;
  }
  cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
}
```

The following example illustrates how you may also iterate through object values, effectively visiting all key-value pairs in the object.

```C++
#include <iostream>
#include "simdjson.h"
using namespace std;
using namespace simdjson;

// ...

ondemand::parser parser;
auto points_json = R"( [
      {  "12345" : {"x":12.34, "y":56.78, "z": 9998877}   },
      {  "12545" : {"x":11.44, "y":12.78, "z": 11111111}  }
    ] )"_padded;

// Parse and iterate through an array of objects
for (ondemand::object points : parser.iterate(points_json)) {
  // Iterating through an object, you iterate through key-value pairs (a 'field').
  for (auto point : points) {
    // Get the key corresponding the the field 'point'.
    cout << "id: " << std::string_view(point.unescaped_key()) << ": (";
    // Get the value corresponding the the field 'point'.
    ondemand::object xyz = point.value();
    cout << xyz["x"].get_double() << ", ";
    cout << xyz["y"].get_double() << ", ";
    cout << xyz["z"].get_int64() << endl;
  }
}
```


Adding support for custom types
----------------------

There are 2 main ways provided by simdjson to deserialize a value into a custom type:

1. Provide a [**template specialization** for member functions](https://en.cppreference.com/w/cpp/language/template_specialization#Members_of_specializations)
   1. Specialize `simdjson::ondemand::document::get` for the whole document
   2. Specialize `simdjson::ondemand::value::get` for each value
2. Using `tag_invoke` *(the recommended way if your system supports C++20 or better)*

We describe both of them in the following sections. Most users who have systems compatible with
C++20 or better should skip ahead to [using `tag_invoke` for custom types (C++20)](#2-use-tag_invoke-for-custom-types-c20) as it is more powerful and simpler.

### 1. Specialize `simdjson::ondemand::value::get` to get custom types (pre-C++20)

Suppose you have your own types, such as a `Car` struct:

```C++
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};
```

You might want to write code that automatically parses the JSON content to your custom
type:

```C++
  padded_string json = R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;


  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c(val);
    std::cout << c.make << std::endl;
  }
```

We may do so by providing additional template definitions to the `ondemand::value` type.
We may start by providing a definition for `std::vector<double>` as follows. Observe
how we guard the code with `#if !SIMDJSON_SUPPORTS_DESERIALIZATION`: that is because the necessary code
is automatically provided by simdjson if C++20 (and concepts) are available.
See [Use `tag_invoke` for custom types](#2-use-tag_invoke-for-custom-types-c20) if you have
C++20 support.

```c++
#if !SIMDJSON_SUPPORTS_DESERIALIZATION
// The code is unnecessary with C++20:
template <>
simdjson_inline simdjson_result<std::vector<double>>
simdjson::ondemand::value::get() noexcept {
  ondemand::array array;
  auto error = get_array().get(array);
  if (error) { return error; }
  std::vector<double> vec;
  for (auto v : array) {
    double val;
    error = v.get_double().get(val);
    if (error) { return error; }
    vec.push_back(val);
  }
  return vec;
}
#endif
```

We may then provide support for our `Car` struct:

```C++
template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::value::get() noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) { return error; }
  Car car;
  if((error = obj["make"].get_string(car.make))) { return error; }
  if((error = obj["model"].get_string(car.model))) { return error; }
  if((error = obj["year"].get_int64().get(car.year))) { return error; }
  if((error = obj["tire_pressure"].get<std::vector<double>>().get(car.tire_pressure))) { return error; }
  return car;
}
```

And that is all that is needed! The following code is a complete example:

```c++
#include "simdjson.h"
#include <iostream>
#include <vector>

using namespace simdjson;

/**
 * A custom type that we want to parse.
 */
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

#if !SIMDJSON_SUPPORTS_DESERIALIZATION
// This code is not necessary if you have a C++20 compliant system:
template <>
simdjson_inline simdjson_result<std::vector<double>>
simdjson::ondemand::value::get() noexcept {
  ondemand::array array;
  auto error = get_array().get(array);
  if (error) { return error; }
  std::vector<double> vec;
  for (auto v : array) {
    double val;
    error = v.get_double().get(val);
    if (error) { return error; }
    vec.push_back(val);
  }
  return vec;
}
#endif

template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::value::get() noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) { return error; }
  Car car;
  if((error = obj["make"].get_string(car.make))) { return error; }
  if((error = obj["model"].get_string(car.model))) { return error; }
  if((error = obj["year"].get_int64().get(car.year))) { return error; }
  if((error = obj["tire_pressure"].get<std::vector<double>>().get(car.tire_pressure))) { return error; }
  return car;
}

int main(void) {
  padded_string json = R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c(val); // an exception may be thrown
    std::cout << c.make << std::endl;
  }
  return EXIT_SUCCESS;
}
```

Observe that we require an explicit cast (`Car c(val)` instead of `for (Car c : doc) {`): it is by design. We require explicit casting.

If you prefer to avoid exceptions, you may modify the `main` function as follows:

```c++
int main(void) {
  padded_string json = R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  auto error = parser.iterate(json).get(doc);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  for (auto val : doc) {
    Car c;
    error = val.get<Car>().get(c);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
    std::cout << c.make << std::endl;
  }
  return EXIT_SUCCESS;
}
```

Our example is limited to `ondemand::value` instances. If you wish to also be able to map
directly the document instance itself to a custom type, you need to provide the definitions to
the `ondemand::document` type. In this instance, we must replace the function with signature
`simdjson_result<Car> simdjson::ondemand::value::get()` with a function having signature
`simdjson_result<Car> simdjson::ondemand::document::get() &`. The following is a complete
example:

```C++
#include "simdjson.h"
#include <iostream>
#include <vector>

using namespace simdjson;

/**
 * A custom type that we want to parse.
 */
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

#if !SIMDJSON_SUPPORTS_DESERIALIZATION
// This code is not necessary if you have a C++20 compliant system:
template <>
simdjson_inline simdjson_result<std::vector<double>>
simdjson::ondemand::value::get() noexcept {
  ondemand::array array;
  auto error = get_array().get(array);
  if (error) { return error; }
  std::vector<double> vec;
  for (auto v : array) {
    double val;
    error = v.get_double().get(val);
    if (error) { return error; }
    vec.push_back(val);
  }
  return vec;
}
#endif

template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::document::get() & noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) { return error; }
  Car car;
  if((error = obj["make"].get_string(car.make))) { return error; }
  if((error = obj["model"].get_string(car.model))) { return error; }
  if((error = obj["year"].get_int64().get(car.year))) { return error; }
  if((error = obj["tire_pressure"].get<std::vector<double>>().get(car.tire_pressure))) { return error; }
  return car;
}


template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::document::get() noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) { return error; }
  Car car;
  if((error = obj["make"].get_string(car.make))) { return error; }
  if((error = obj["model"].get_string(car.model))) { return error; }
  if((error = obj["year"].get_int64().get(car.year))) { return error; }
  if((error = obj["tire_pressure"].get<std::vector<double>>().get(car.tire_pressure))) { return error; }
  return car;
}

int main(void) {
  padded_string json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] } )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  Car c(doc);
  std::cout << c.make << std::endl;
  return EXIT_SUCCESS;
}
```

### 2. Use `tag_invoke` for custom types (C++20)

In C++20, the standard introduced the notion of *customization point*.
A customization point is a function or function object that can be customized for different types. It allows library authors to provide default behavior while giving users the ability to override this behavior for specific types.

A tag_invoke function serves as a mechanism for customization points. It is not directly part of the C++ standard library but is often used in libraries that implement customization points.
The tag_invoke function is typically a generic function that takes a tag type and additional arguments.
The first argument is usually a tag type (often an empty struct) that uniquely identifies the customization point (e.g., deserialization of custom types in simdjson). Users or library providers can specialize tag_invoke for their types by defining it in the appropriate namespace, often inline namespace.


If your system supports C++20, we recommend that you adopt the `tag_invoke` approach
instead to deserialize custom types. It may prove to be considerably simpler. When
simdjson detects the necessary support, it sets the `SIMDJSON_SUPPORTS_DESERIALIZATION` macro
to 1, otherwise it is set to 0.

Consider a custom class `Car`:

```C++
struct Car {
  std::string make;
  std::string model;
  int year;
  std::vector<float> tire_pressure;
};
```

Observe how we defined the class to use types that simdjson does not directly support (`float`, `int`).
With C++20 support, the library grabs from the JSON the generic type (`double`, `int`) and then it
casts it automatically.

You may support deserializing directly from a JSON value or document to your own `Car` instance
by defining a single `tag_invoke` function:


```C++
namespace simdjson {
// This tag_invoke MUST be inside simdjson namespace
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, Car& car) {
  ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) {
    return error;
  }
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<float>>().get(
           car.tire_pressure))) {
    return error;
  }
  return simdjson::SUCCESS;
}
} // namespace simdjson
```

Observe how we call `get<std::vector<float>>()` even though we never defined support
for `std::vector<float>` in the simdjson library: it is all automated thanks to C++20 concepts.

Importantly, the `tag_invoke` function must be inside the `simdjson` namespace.
Let us explain each argument of `tag_invoke` function.

- `simdjson::deserialize_tag`: it is the tag for Customization Point Object (CPO). You may often ignore this parameter. It is used to indicate that you mean to provide a deserialization function for simdjson.
- `var`: It receives automatically a `simdjson` value type (document, value, document_reference).
- The third parameter is an instance of the type that you want to support.

You can use it like so:

```cpp
  simdjson::padded_string json =
      R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Car c(doc);
  std::cout << c.make << std::endl;
```

Observe how we first get an instance of `document` and then we cast.

You can also handle errors explicitly:

```cpp
  Car c;
  auto error = doc.get(c);
  if(error) { std::cerr << simdjson::error_message(error); return false; }
  std::cout << c.make << std::endl;
```

You can also read instances of `Car` from an array or an object:
```cpp
  simdjson::padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c(val); // an exception may be thrown
    std::cout << c.year << std::endl;
  }
```

Observe how we first get a generic (`val`) which we cast to `Car`. It is by design: we require
explicit casting. The cast may throw an exception.

Once more, you can handle errors explicitly:

```cpp
  for (auto val : doc) {
    Car c;
    auto error = val.get(c);
    if(error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
  }
```

You can also use the custom `Car` type as part of a template such as `std::vector`:

```cpp
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<Car> cars(doc);
  // visual studio users need an explicit call:
  //   std::vector<Car> cars = doc.get<std::vector<Car>>();
  // because the compiler does not know whether to convert
  // doc to an unsigned int or to a vector.
  for(Car& c : cars) {
    std::cout << c.year << std::endl;
  }
```

By default, we support a wide range of standard templates such as
`std::vector`,  `std::list`, `std::set`, `std::stack`, `std:queue`,
`std:deque`, `std::priority_queue`, `std::unordered_set`, `std::multiset`,
`std::unordered_multiset`, `std::unique_ptr`, `std::shared_ptr`, `std::optional`,
etc. They are handled automatically.

E.g., you can recover an `std::unique_ptr<Car>` like so:
```C++
int main() {
  auto const json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::unique_ptr<Car> c(doc);
  std::cout << c->make << std::endl;
  return EXIT_SUCCESS;
}
```

You may also conditionally fill in `std::optional` values.

```C++

  padded_string json =
      R"( { "car1": { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::optional<Car> car;
  error = doc["key not found"].get<std::optional<Car>>().get(car);
  // car has no value, error != simdjson::SUCCESS
  error = doc["car1"].get<std::optional<Car>>().get(car);
  // car has value Car{"Toyota", "Camry", 2018, {40.1f, 39.9f}}
  // error is simdjson::SUCCESS
```

And so forth.

Advanced users may want to overwrite the defaults provided by the simdjson library.
Suppose for example that you want to construct an instance of `std::list<Car>`, but
you also want to filter out any car made by Toyota. You may provide your own
`tag_invoke` function:

```c++
namespace simdjson {
// suppose we want to filter out all Toyotas
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, std::list<Car>& car) {
  ondemand::array arr;
  auto error = val.get_array().get(arr);
  if (error) {
    return error;
  }
  for (auto v : arr) {
    Car c;
    if ((error = v.get<Car>().get(c))) {
      return error;
    }
    if(c.make != "Toyota") {
      car.push_back(c);
    }
  }
  return simdjson::SUCCESS;
}
}
```

With this code, deserializing an `std::list<Car>` instance would capture only the cars
that are not made by Toyota.


Minifying JSON strings without parsing
----------------------

In some cases, you may have valid JSON strings that you do not wish to parse but that you wish to minify. That is, you wish to remove all unnecessary spaces. We have a fast function for this purpose (`simdjson::minify(const char * input, size_t length, const char * output, size_t& new_length)`). This function does not validate your content, and it does not parse it.  It is much faster than parsing the string and re-serializing it in minified form (`simdjson::minify(parser.parse())`). Usage is relatively simple. You must pass an input pointer with a length parameter, as well as an output pointer and an output length parameter (by reference). The output length parameter is not read, but written to. The output pointer should point to a valid memory region that is as large as the original string length. The input pointer and input length are read, but not written to.

```C++
  // Starts with a valid JSON document as a string.
  // It does not have to be null-terminated.
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  // Create a buffer to receive the minified string. Make sure that there is enough room (length bytes).
  std::unique_ptr<char[]> buffer{new char[length]};
  size_t new_length{}; // It will receive the minified length.
  auto error = simdjson::minify(some_string, length, buffer.get(), new_length);
  if(error) { std::cerr << simdjson::error_message(error); }
  // The buffer variable now has "[1,2,3,4]" and new_length has value 9.
```

Though it does not validate the JSON input, it will detect when the document ends with an unterminated string. E.g., it would refuse to minify the string `"this string is not terminated` because of the missing final quote.


UTF-8 validation (alone)
----------------------

The simdjson library has fast functions to validate UTF-8 strings. They are many times faster than most functions commonly found in libraries. You can use our fast functions, even if you do not care about JSON.

```C++
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  bool is_ok = simdjson::validate_utf8(some_string, length);
```

The UTF-8 validation function merely checks that the input is valid UTF-8: it works with strings in general, not just JSON strings.

Your input string does not need any padding. Any string will do. The `validate_utf8` function does not do any memory allocation on the heap, and it does not throw exceptions.

If you find yourself needing only fast Unicode functions, consider using the simdutf library instead: https://github.com/simdutf/simdutf

JSON Pointer
------------

The simdjson library also supports [JSON pointer](https://tools.ietf.org/html/rfc6901) through the `at_pointer()` method, letting you reach further down into the document in a single call. JSON pointer is supported by both the [DOM approach](https://github.com/simdjson/simdjson/blob/master/doc/dom.md#json-pointer) as well as the On-Demand approach.

**Note:** The On-Demand implementation of JSON pointer relies on `find_field` which implies that it does not unescape keys when matching.

Consider the following example:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
ondemand::parser parser;
auto cars = parser.iterate(cars_json);
cout << cars.at_pointer("/0/tire_pressure/1") << endl; // Prints 39.9
```

A JSON Pointer path is a sequence of segments each starting with the '/' character. Within arrays, a zero-based integer
index allows you to select the indexed node. Within objects, the string value of the key allows you to
select the value. If your keys contain the characters '/' or '~', they must be escaped as '~1' and
'~0' respectively. An empty JSON Pointer Path refers to the whole document.

For multiple JSON pointer queries on a document, one can call `at_pointer` multiple times.

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
ondemand::parser parser;
auto cars = parser.iterate(cars_json);
size_t size = cars.count_elements();

for (size_t i = 0; i < size; i++) {
    std::string json_pointer = "/" + std::to_string(i) + "/tire_pressure/1";
    double x = cars.at_pointer(json_pointer);
    std::cout << x << std::endl; // Prints 39.9, 31 and 30
}
```

In most instances, a JSON Pointer is an ASCII string and the keys in a JSON document
are ASCII strings. We support UTF-8 in JSON Pointer, but key values are matched exactly, without unescaping or Unicode normalization. We do a byte-by-byte comparison. The e acute character is
considered distinct from its escaped version `\u00E9`. E.g.,

```c++
const padded_string json = "{\"\\u00E9\":123}"_padded;
auto doc = parser.iterate(json);
doc.at_pointer("/\\u00E9") == 123; // true
doc.at_pointer((const char*)u8"/\u00E9") // returns an error (NO_SUCH_FIELD)
```

Note that `at_pointer` calls [`rewind`](#rewind) to reset the parser at the beginning of the document. Hence, it invalidates all previously parsed values, objects and arrays: make sure to consume the values between each call to  `at_pointer`. Consider the following example where one wants to store each object from the JSON into a vector of `struct car_type`:

```c++
struct car_type {
    std::string make;
    std::string model;
    uint64_t year;
    std::vector<double> tire_pressure;
    car_type(std::string_view _make, std::string_view _model, uint64_t _year,
      std::vector<double>&& _tire_pressure) :
      make{_make}, model{_model}, year(_year), tire_pressure(_tire_pressure) {}
};

auto cars_json = R"( [
{ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
{ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
{ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;

ondemand::parser parser;
std::vector<double> measured;
ondemand::document cars = parser.iterate(cars_json);
std::vector<car_type> content;
for (int i = 0; i < 3; i++) {
    std::string json_pointer = "/" + std::to_string(i);
    // Each successive at_pointer call invalidates
    // previously parsed values, strings, objects and array.
    ondemand::object obj(cars.at_pointer(json_pointer).get_object());
    // We materialize the object.
    std::string_view make = obj["make"];
    std::string_view model = obj["model"];
    uint64_t year(obj["year"]);
    // We materialize the array.
    ondemand::array arr(obj["tire_pressure"].get_array());
    std::vector<double> values;
    for(auto x : arr) {
        double value_double(x.get_double());
        values.push_back(value_double);
    }
    content.emplace_back(make, model, year, std::move(values));
}
```

Furthermore, `at_pointer` calls `rewind` at the beginning of the call (i.e. the document is not reset after `at_pointer`). Consider the following example,

```c++
auto json = R"( {
  "k0": 27,
  "k1": [13,26],
  "k2": true
} )"_padded;
ondemand::parser parser;
auto doc = parser.iterate(json);
std::cout << doc.at_pointer("/k1/1") << std::endl; // Prints 26
std::cout << doc.at_pointer("/k2") << std::endl; // Prints true
doc.rewind();	// Need to manually rewind to be able to use find_field properly from start of document
std::cout << doc.find_field("k0") << std::endl; // Prints 27
```

When the JSON Pointer Path is the empty string (`""`) applied to a scalar document (lone string, number, Boolean or null), a SCALAR_DOCUMENT_AS_VALUE error is returned because scalar document cannot
be represented as `value` instances. You can check that a document is a scalar with the method `scalar()`.

JSONPath
------------

The simdjson library supports a subset of [JSONPath](https://datatracker.ietf.org/doc/html/draft-normington-jsonpath-00) through the `at_path()` method, allowing you to reach further into the document in a single call. The subset of JSONPath that is implemented is the subset that is trivially convertible into the JSON Pointer format, using `.` to access a field and `[]` to access a specific index.

This implementation relies on `at_path()` converting its argument to JSON Pointer and then calling `at_pointer`, which makes use of
[`rewind`](#rewind) to reset the parser at the beginning of the document. Hence, it invalidates all previously parsed values, objects
 and arrays: make sure to consume the values between each call to `at_path`.

Consider the following example:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
ondemand::parser parser;
auto cars = parser.iterate(cars_json);
cout << cars.at_path("[0].tire_pressure[1]") << endl; // Prints 39.9
```

A call to `at_path(json_path)` can result in any of the errors that are returned by the `at_pointer` method and if the conversion of `json_path` to JSON Pointer fails, it will lead to an `simdjson::INVALID_JSON_POINTER`error.

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
ondemand::parser parser;
auto cars = parser.iterate(cars_json);
ASSERT_ERROR(cars.at_path("[0].tire_presure[1").get(x), INVALID_JSON_POINTER); // Fails on conversion to JSON Pointer, since last square bracket was not properly closed.
ASSERT_ERROR(cars.at_path("[0].incorrect_field[1]").get(x), NO_SUCH_FIELD); // Conversion to JSON Pointer succeeds, but fails on at_pointer() since the path is invalid.
```

In most instances, a JSONPath is an ASCII string and the keys in a JSON document
are ASCII strings. We support UTF-8 within a JSONPath expression, but key values are
matched exactly, without unescaping or Unicode normalization. We do a byte-by-byte comparison.
The e acute character is considered distinct from its escaped version `\u00E9`. E.g.,

```c++
const padded_string json = "{\"\\u00E9\":123}"_padded;
auto doc = parser.iterate(json);
doc.at_path(".\\u00E9") == 123; // true
doc.at_path((const char*)u8".\u00E9") // returns an error (NO_SUCH_FIELD)
```


We also support the `$` prefix. When you start a JSONPath expression with $, you are indicating that the path starts from the root of the JSON document. E.g.,

```c++
auto json = R"( { "c" :{ "foo": { "a": [ 10, 20, 30 ] }}, "d": { "foo2": { "a": [ 10, 20, 30 ] }} , "e": 120 })"_padded;
ondemand::parser parser;
ondemand::document doc = parser.iterate(json);
ondemand::object obj = doc.get_object();
int64_t x = obj.at_path("$.c.foo.a[1]"); // 20
x = obj.at_path("$.d.foo2.a.2"); // 30
```

Error handling
--------------

Error handing with exception and a single try/catch clause makes the code simple, but it gives you little control over errors. For easier debugging or more robust error handling, you may want to consider our exception-free approach.

The entire simdjson API is usable with and without exceptions. All simdjson APIs that can fail return `simdjson_result<T>`, which is a &lt;value, error_code&gt;
pair. You can retrieve the value with .get() without generating an exception, like so:

```c++
ondemand::document doc;
auto error = parser.iterate(json).get(doc);
if(error) { std::cerr << simdjson::error_message(error); exit(1); }
```

When there is no error, the error code `simdjson::SUCCESS`is returned: it evaluates as false as a Boolean.
We have several error codes to indicate errors, they all evaluate to true as a Boolean: your software should not generally not depend on exact
error codes. We may change the error codes in future releases and the exact error codes could vary depending on your system.

Some errors are recoverable:

* You may get the error `simdjson::INCORRECT_TYPE` after trying to convert a value to an incorrect type: e.g., you expected a number and try to convert the value to a number, but it is an array.
* You may query a key from an object, but the key is missing in which case you get the error `simdjson::NO_SUCH_FIELD`: e.g., you call `obj["myname"]` and the object does not have a key `"myname"`.

Other errors (e.g., `simdjson::INCOMPLETE_ARRAY_OR_OBJECT`) may indicate a fatal error and often follow from the fact that the document is not valid JSON. In which case, it is no longer possible to continue accessing the document: calling the method `is_alive()` on the document instance returns false. All following accesses will keep returning the same fatal error (e.g., `simdjson::INCOMPLETE_ARRAY_OR_OBJECT`).

When you use the code without exceptions, it is your responsibility to check for error before using the
result: if there is an error, the result value will not be valid and using it will caused undefined behavior. Most compilers should be able to help you if you activate the right
set of warnings: they can identify variables that are written to but never otherwise accessed.

Let us illustrate with an example where we try to access a number that is not valid (`3.14.1`).
If we want to proceed without throwing and catching exceptions, we can do so as follows:

```C++
bool simple_error_example() {
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    ondemand::document doc;
    if (parser.iterate(json).get(doc) != SUCCESS) { return false; }
    double x;
    auto error = doc["bad number"].get_double().get(x);
    // returns "simdjson::NUMBER_ERROR"
    if (error != SUCCESS) {
      std::cerr << simdjson::error_message(error) << std::endl;
      return false;
    }
    std::cout << "Got " << x << std::endl;
    return true;
}
```

Observe how we verify the error variable before accessing the retrieved number (variable `x`).

The equivalent with exception handling might look as follows.

```C++
  bool simple_error_example_except() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    try {
      ondemand::document doc = parser.iterate(json);
      double x = doc["bad number"].get_double();
      std::cout << "Got " << x << std::endl;
      return true;
    } catch(simdjson_error& e) {
      // e.error() == NUMBER_ERROR
      std::cout << e.error() << std::endl;
      return false;
    }
  }
```

Notice how we can retrieve the exact error condition (in this instance `simdjson::NUMBER_ERROR`)
from the exception.

We can write a "quick start" example where we attempt to parse the following JSON file and access some data, without triggering exceptions:
```JSON
{
  "statuses": [
    {
      "id": 505874924095815700
    },
    {
      "id": 505874922023837700
    }
  ],
  "search_metadata": {
    "count": 100
  }
}
```

Our program loads the file, selects value corresponding to key `"search_metadata"` which expected to be an object, and then
it selects the key `"count"` within that object.

```C++
#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::ondemand::parser parser;
  auto error = padded_string::load("twitter.json").get(json);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  simdjson::ondemand::document tweets;
  error = parser.iterate(json).get(tweets);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  simdjson::ondemand::value res;
  error = tweets["search_metadata"]["count"].get(res);
  if (error != SUCCESS) {
    std::cerr << "could not access keys : " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
  return EXIT_SUCCESS;
}
```

The following is a similar example where one wants to get the id of the first tweet without
triggering exceptions. To do this, we use `["statuses"].at(0)["id"]`. We break that expression down:

- Get the list of tweets (the `"statuses"` key of the document) using `["statuses"]`). The result is expected to be an array.
- Get the first tweet using `.at(0)`. The result is expected to be an object.
- Get the id of the tweet using ["id"]. We expect the value to be a non-negative integer.

Observe how we use the `at` method when querying an index into an array, and not the bracket operator.


```C++
#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document tweets;
  padded_string json;
  auto error = padded_string::load("twitter.json").get(json);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  error = parser.iterate(json).get(tweets);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  uint64_t identifier;
  error = tweets["statuses"].at(0)["id"].get(identifier);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return EXIT_FAILURE; }
  std::cout << identifier << std::endl;
}
```

The `at` method can only be called once on an array. It cannot be used
to iterate through the values of an array.

### Error handling examples without exceptions

This is how the example in "Using the parsed JSON" could be written using only error code checking (without exceptions):

```c++
bool parse() {
  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  ondemand::document doc;

  // Iterating through an array of objects
  auto error = parser.iterate(cars_json).get(doc);
  if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
  ondemand::array cars; // invalid until the get() succeeds
  error = doc.get_array().get(cars);

  for (auto car_value : cars) {
    ondemand::object car; // invalid until the get() succeeds
    error = car_value.get_object().get(car);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }

    // Accessing a field by name
    std::string_view make;
    std::string_view model;
    error = car["make"].get(make);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
    error = car["model"].get(model);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }

    cout << "Make/Model: " << make << "/" << model << endl;

    // Casting a JSON element to an integer
    uint64_t year{};
    error = car["year"].get(year);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
    cout << "- This car is " << 2020 - year << " years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    ondemand::array pressures;
    error = car["tire_pressure"].get_array().get(pressures);
    if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
    for (auto tire_pressure_value : pressures) {
      double tire_pressure;
      error = tire_pressure_value.get_double().get(tire_pressure);
      if (error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
  }
  return true;
}
```

For safety, you should only use our ondemand instances (e.g., `ondemand::object`)
after you have initialized them and checked that there is no error:

```c++
    ondemand::object car; // invalid until the get() succeeds
    // the `car` instance should not use used before it is initialized
    error = car_value.get_object().get(car);
    if (error) {
      // the `car` instance should not use used
    } else {
      // the `car` instance can be safely used
    }
```

The following examples illustrates how to iterate through the content of an object without
having to handle exceptions.
```c++
  auto json = R"({"k\u0065y": 1})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  auto error = parser.iterate(json).get(doc);
  if (error) { return false; }
  ondemand::object object; // invalid until the get() succeeds
  error = doc.get_object().get(object);
  if (error) { return false; }
  for(auto field : object) {
    // We could replace 'field.key() with field.unescaped_key() or field.escaped_key(),
    // and ondemand::raw_json_string by std::string_view.
    ondemand::raw_json_string keyv;
    error = field.key().get(keyv);
    if (error) { return false; }
    if (keyv == "key") {
      uint64_t intvalue;
      error = field.value().get(intvalue);
      if (error) { return false; }
      std::cout << intvalue;
    }
  }
```

### Disabling exceptions

The simdjson can be build with exceptions entirely disabled. It checks the `__cpp_exceptions` macro at compile time. Even if exceptions are enabled in your compiler, you may still disable exceptions specifically for simdjson, by setting `SIMDJSON_EXCEPTIONS` to `0` (false) at compile-time when building the simdjson library. If you are building with CMake,  to ensure you don't write any code that uses exceptions, you compile with `SIMDJSON_EXCEPTIONS=OFF`. For example, if including the project via cmake:

```cmake
target_compile_definitions(simdjson PUBLIC SIMDJSON_EXCEPTIONS=OFF)
```

### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```c++
simdjson::ondemand::document doc = parser.iterate(json); // Throws an exception if there was an error!
```

When used this way, a `simdjson_error` exception will be thrown if an error occurs, preventing the
program from continuing if there was an error.


If one is willing to trigger exceptions, it is possible to write simpler code:

```C++
#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::ondemand::parser parser;
  padded_string json = padded_string::load("twitter.json");
  simdjson::ondemand::document tweets = parser.iterate(json);
  uint64_t identifier = tweets["statuses"].at(0)["id"];
  std::cout << identifier << std::endl;
  return EXIT_SUCCESS;
}
```


You can do handle errors gracefully as well...

```C++
#include <iostream>
#include "simdjson.h"
int main(void) {
  simdjson::ondemand::parser parser;
  simdjson::padded_string json_string;
  simdjson::ondemand::document doc;
  try {
    json_string = padded_string::load("twitter.json");
    doc = parser.iterate(json_string);
    uint64_t identifier = doc["statuses"].at(0)["id"];
    std::cout << identifier << std::endl;
  } catch (simdjson::simdjson_error &error) {
    std::cerr << "JSON error: " << error.what() << " near "
              << doc.current_location() << " in " << json_string << std::endl;
  }
}
```

### Current location in document

Sometimes, it might be helpful to know the current location in the document during iteration. This is especially useful when encountering errors. The `current_location()` method on a
`document` instances makes it easy to identify common JSON errors. Users can call the `current_location()` method on a valid document instance to retrieve a `const char *` pointer to the current location in the document. This method also works even after an error has invalidated the document and the parser (e.g. `TAPE_ERROR`, `INCOMPLETE_ARRAY_OR_OBJECT`).
When the input was a `padding_string` or another null-terminated source, then you may
use the `const char *` pointer as a C string. As an example, consider the following
example where we used the exception-free simdjson interface:

```c++
auto broken_json = R"( {"double": 13.06, false, "integer": -343} )"_padded;    // Missing key
ondemand::parser parser;
auto doc = parser.iterate(broken_json);
int64_t i;
auto error = doc["integer"].get_int64().get(i);    // Expect to get integer from "integer" key, but get TAPE_ERROR
if (error) {
  std::cerr << simdjson::error_message(error) << std::endl;    // Prints TAPE_ERROR error message
  // Recover a pointer to the location of the first error:
  const char * ptr;
  doc.current_location().get(ptr);
  // ptr points at 'false, "integer": -343} " which is the location of the error
  //
  // Because we pad simdjson::padded_string instances with null characters, you may also do the following:
  std::cout<< doc.current_location() << std::endl;  // Prints "false, "integer": -343} " (location of TAPE_ERROR)
}
```

You may also use `current_location()` with exceptions as follows:

```c++
auto broken_json = R"( {"double": 13.06, false, "integer": -343} )"_padded;
ondemand::parser parser;
ondemand::document doc = parser.iterate(broken_json);
try {
  return int64_t(doc["integer"]);
} catch(simdjson_error& err) {
  std::cerr << doc.current_location() << std::endl;
  return -1;
}
```

In these examples, we tried to access the `"integer"` key, but since the parser
had to go through a value without a key before (`false`), a `TAPE_ERROR` error is thrown.
The pointer returned by the `current_location()` method then points at the location of the error. The `current_location()` may also be used when the error is triggered
by a user action, even if the JSON input is valid. Consider the following example:

```c++
auto json = R"( [1,2,3] )"_padded;
ondemand::parser parser;
auto doc = parser.iterate(json);
int64_t i;
auto error = doc["integer"].get_int64().get(i);    // Incorrect call on array, INCORRECT_TYPE error
if (error) {
  std::cerr << simdjson::error_message(error) << std::endl;     // Prints INCORRECT_TYPE error message
  std::cout<< doc.current_location() << std::endl;  // Prints "[1,2,3] " (location of INCORRECT_TYPE error)
}
```

If the location is invalid (i.e. at the end of a document), the `current_location()`
methods returns an `OUT_OF_BOUNDS` error. For example:

```c++
auto json = R"( [1,2,3] )"_padded;
ondemand::parser parser;
auto doc = parser.iterate(json);
for (auto val : doc) {
  // Do something with val
}
std::cout << doc.current_location() << std::endl;   // Throws OUT_OF_BOUNDS
```

Conversely, if `doc.current_location().error() == simdjson::SUCCESS`,
then the document has more content.

Finally, the `current_location()` method may also be used even when no exceptions/errors
are thrown. This can be helpful for users that want to know the current state of iteration during parsing. For example:

```c++
auto json = R"( [[1,2,3], -23.4, {"key": "value"}, true] )"_padded;
ondemand::parser parser;
auto doc = parser.iterate(json);
for (auto val : doc) {
  ondemand::object obj; // invalid until the get() succeeds
  auto error = val.get_object().get(obj);     // Only get objects
  if (!error) {
    std::cout << doc.current_location() << std::endl;   // Prints ""key": "value"}, true] "
  }
}
```

The `current_location()` method requires a valid `document` instance. If the
`iterate` function fails to return a valid document, then you cannot use
`current_location()` to identify the location of an error in the input string.
The errors reported by `iterate` function include EMPTY if no JSON document is detected,
UTF8_ERROR if the string is not a valid UTF-8 string, UNESCAPED_CHARS if a string
contains control characters that must be escaped and UNCLOSED_STRING if there
is an unclosed string in the document. We do not provide location information for these
errors.

### Checking for trailing content

The parser validates all parsed content, but your code may exhaust the content while
not having processed the entire document. Thus, as a final optional step, you may
call `at_end()` on the document instance. If it returns `false`, then you may
conclude that you have trailing content and that your document is not valid JSON.
You may then use `doc.current_location()` to obtain a pointer to the start of the trailing
content.

```C++
  auto json = R"([1, 2] foo ])"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::array array = doc.get_array();
  for (uint64_t values : array) {
    std::cout << values << std::endl;
  }
  if (!doc.at_end()) {
    // In this instance, we will be left pointing at 'foo' since we have consumed the array [1,2].
    std::cerr << "trailing content at byte index " << doc.current_location() - json.data() << std::endl;
  }
```

The `at_end()` method is equivalent to `doc.current_location().error() == simdjson::SUCCESS` but
more convenient.

Rewinding
----------

In some instances, you may need to go through a document more than once. For that purpose, you may
call the `rewind()` method on the document instance. It allows you to restart processing from the
beginning without rescanning all of the input data again. It invalidates all values, objects and arrays
that you have created so far (including unescaped strings).

In the following example, we print on the screen the number of cars in the JSON input file
before printout the data.

```C++
  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;

  auto doc = parser.iterate(cars_json);
  for (simdjson_unused ondemand::object car : doc) {
    if (car["make"] == "Toyota") { count++; }
  }
  std::cout << "We have " << count << " Toyota cars.\n";
  doc.rewind(); // requires simdjson 1.0 or better
  for (ondemand::object car : doc) {
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;
  }
```

Performance note: the On-Demand front-end does not materialize the parsed numbers and other values. If you are accessing everything twice, you may need to parse them twice. Thus the rewind functionality is best suited for cases where the first pass only scans the structure of the document.

Both arrays and objects have a similar method `reset()`. It is similar
to the document `rewind()` method, except that it does not rewind the
internal string buffer. Thus you should consume values only once
even if you can iterate through the array or object more than once.
If you unescape a string within an array more than once, you have unsafe code.


Newline-Delimited JSON (ndjson) and JSON lines
----------------------------------------------

When processing large inputs (e.g., in the context of data engineering), engineers commonly
serialize data into streams of multiple JSON documents. That is, instead of one large
(e.g., 2 GB) JSON document containing multiple records, it is often preferable to
write out multiple records as independent JSON documents, to be read one-by-one.

The simdjson library also supports multithreaded JSON streaming through a large file
containing many smaller JSON documents in either [ndjson](http://ndjson.org)
or [JSON lines](http://jsonlines.org) format. If your JSON documents all contain arrays
or objects, we even support direct file concatenation without whitespace. However, if there
is content between your JSON documents, it should be exclusively ASCII white-space characters.

The concatenated file has no size restrictions (including larger than 4GB), though each
individual document must be no larger than 4 GB.

Here is an example:

```c++
auto json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )"_padded;
ondemand::parser parser;
ondemand::document_stream docs = parser.iterate_many(json);
for (auto doc : docs) {
  std::cout << doc["foo"] << std::endl;
}
// Prints 1 2 3
```


Unlike `parser.iterate`, `parser.iterate_many` may parse "On-Demand" (lazily). That is, no parsing may have been done before you enter the loop
`for (auto doc : docs) {` and you should expect the parser to only ever fully parse one JSON document at a time.

As with `parser.iterate`, when calling  `parser.iterate_many(string)`, no copy is made of the provided string input. The provided memory buffer may be accessed each time a JSON document is parsed.  Calling `parser.iterate_many(string)` on a  temporary string buffer (e.g., `docs = parser.parse_many("[1,2,3]"_padded)`) is unsafe (and will not compile) because the  `document_stream` instance needs access to the buffer to return the JSON documents.


The `iterate_many` function can also take an optional parameter `size_t batch_size` which defines the window processing size. It is set by default to a large value (`1000000` corresponding to 1 MB). None of your JSON documents should exceed this window size, or else you will get  the error `simdjson::CAPACITY`. You cannot set this window size larger than 4 GB: you will get  the error `simdjson::CAPACITY`. The smaller the window size is, the less memory the function will use. Setting the window size too small (e.g., less than 100 kB) may also impact performance negatively. Leaving it to 1 MB is expected to be a good choice, unless you have some larger documents.


The following toy examples illustrates how to get capacity errors. It is an artificial example since you should never use a `batch_size` of 50 bytes (it is far too small).

```c++
// We are going to set the capacity to 50 bytes which means that we cannot
// loading a document longer than 50 bytes. The first few documents are small,
// but the last one is large. We will get an error at the last document.
auto json = R"([1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
size_t counter{0};
auto error = parser.iterate_many(json, 50).get(stream);
if (error) { /* handle the error */ }
for (auto doc: stream) {
  if (counter < 6) {
    int64_t val;
    error = doc.at_pointer("/4").get(val);
    if (error) { /* handle the error */ }
    std::cout << "5 = " << val << std::endl;
  } else {
    ondemand::value val;
    error = doc.at_pointer("/4").get(val);
    // error == simdjson::CAPACITY
    if (error) {
      std::cerr << simdjson::error_message(error) << std::endl;
      // We left 293 bytes unprocessed at the tail end of the input.
      std::cout << " unprocessed bytes at the end: " << stream.truncated_bytes() << std::endl;
      break;
    }
  }
  counter++;
}
```

This example should print out:

```
5 = 5
5 = 5
5 = 5
5 = 5
5 = 5
5 = 5
This parser can't support a document that big
 unprocessed bytes at the end: 293
```

If your documents are large (e.g., larger than a megabyte), then the `iterate_many` function is maybe ill-suited. It is really meant to support reading efficiently streams of relatively small documents (e.g., a few kilobytes each). If you have larger documents, you should use other functions like `iterate`.

We also provide some support for comma-separated documents and other advanced features.
See [iterate_many.md](iterate_many.md) for detailed information and design.

Parsing numbers inside strings
------------------------------

Though the JSON specification allows for numbers and string values, many engineers choose to integrate the numbers inside strings, e.g., they prefer `{"a":"1.9"}` to`{"a":1.9}`.
The simdjson library supports parsing valid numbers inside strings which makes it more convenient for people working with those types of documents. This feature is supported through
three methods: `get_double_in_string`, `get_int64_in_string` and  `get_uint64_in_string`. However, it is important to note that these methods are not substitute to the regular
`get_double`, `get_int64` and `get_uint64`. The usage of the `get_*_in_string` methods is solely to parse valid JSON numbers inside strings, and so we expect users to call these
methods appropriately. In particular, a valid JSON number has no leading and no trailing whitespaces, and the strings `"nan"`, `"1e"` and `"infinity"` will not be accepted as valid
numbers. As an example, suppose we have the following JSON text:

```c++
auto json =
{
   "ticker":{
      "base":"BTC",
      "target":"USD",
      "price":"443.7807865468",
      "volume":"31720.1493969300",
      "change":"Infinity",
      "markets":[
         {
            "market":"bitfinex",
            "price":"447.5000000000",
            "volume":"10559.5293639000"
         },
         {
            "market":"bitstamp",
            "price":"448.5400000000",
            "volume":"11628.2880079300"
         },
         {
            "market":"btce",
            "price":"432.8900000000",
            "volume":"8561.0563600000"
         }
      ]
   },
   "timestamp":1399490941,
   "timestampstr":"1399490941"
}
```

Now, suppose that a user wants to get the time stamp from the `timestampstr` key. One could do the following:

```c++
ondemand::parser parser;
auto doc = parser.iterate(json);
uint64_t time = doc.at_pointer("/timestampstr").get_uint64_in_string();
std::cout << time << std::endl;   // Prints 1399490941
```

Another thing a user might want to do is extract the `markets` array and get the market name, price and volume. Here is one way to do so:

```c++
ondemand::parser parser;
auto doc = parser.iterate(json);

// Getting markets array
ondemand::array markets = doc.find_field("ticker").find_field("markets").get_array();
// Iterating through markets array
for (auto value : markets) {
    std::cout << "Market: " << value.find_field("market").get_string();
    std::cout << "\tPrice: " << value.find_field("price").get_double_in_string();
    std::cout << "\tVolume: " << value.find_field("volume").get_double_in_string() << std::endl;
}

/* The above prints
Market: bitfinex        Price: 447.5    Volume: 10559.5
Market: bitstamp        Price: 448.54   Volume: 11628.3
Market: btce    Price: 432.89   Volume: 8561.06
*/
```

Finally, here is an example dealing with errors where the user wants to convert the string `"Infinity"`(`"change"` key) to a float with infinity value.

```c++
ondemand::parser parser;
auto doc = parser.iterate(json);
// Get "change"/"Infinity" key/value pair
ondemand::value value = doc.find_field("ticker").find_field("change");
double d;
std::string_view view;
auto error = value.get_double_in_string().get(d);
// Check if parsed value into double successfully
if (error) {
  error = value.get_string().get(view);
  if (error) { /* Handle error */ }
  else if (view == "Infinity") {
    d = std::numeric_limits::infinity();
  }
  else { /* Handle wrong value */ }
}
```
It is also important to note that when dealing an invalid number inside a string, simdjson will report a `NUMBER_ERROR` error if the string begins with a number whereas simdjson
will report an `INCORRECT_TYPE` error otherwise.

The `*_in_string` methods can also be called on a single document instance:
e.g., when your document consist solely of a quoted number.

Dynamic Number Types
------------------------------

The JSON standard does not offer strongly typed numbers. It suggests that using
the binary64 type (`double` in C++) is a safe choice, but little else.
Given the JSON array `[1.0,1]`,  it is not specified whether it is an array
of two floating-point numbers, two integers, or one floating-point number
followed by an integer.

Given an `ondemand::value` instance, you may ask whether it is a negative value
with the `is_negative()` method. The function is inexpensive.

To occasionally distinguish between floating-point values and integers given
an `ondemand::value` instance, you may call the `is_integer()` method. We recognize
an integer number by the lack decimal point and the lack of exponential suffix. E.g.,
`1e1` is always considered to be a floating-point number. The `is_integer()` method
does not consume the value, but it scans the number string. You should avoid calling
it repeatedly.

If you need to determine both the type of the number (integer or floating-point) and
its value efficiently, you may call the `get_number()` method on the `ondemand::value`
instance. Upon success, it returns an `ondemand::number` instance.


An `ondemand::number` instance may contain an integer value or a floating-point value.
Thus it is a dynamically typed number. Before accessing the value, you must determine the detected type:

* `number.get_number_type()` has value `number_type::signed_integer` if we have a integer in [-9223372036854775808,9223372036854775808). You can recover the value by the `get_int64()` method applied on the `ondemand::number` instance. When `number.get_number_type()` has value `number_type::signed_integer`, you also have that `number.is_int64()` is true. Calling `get_int64()` on the `ondemand::number` instance when `number.get_number_type()` is not `number_type::signed_integer` is unsafe. You may replace `get_int64()` by a cast to a `int64_t` value.
* `number.get_number_type()` has value `number_type::unsigned_integer` if we have a integer in `[9223372036854775808,18446744073709551616)`. You can recover the value by the `get_uint64()` method applied on the `ondemand::number` instance.  When `number.get_number_type()` has value `number_type::unsigned_integer`, you also have that `number.is_uint64()` is true.  Calling `get_uint64()` on the `ondemand::number` instance when `number.get_number_type()` is not `number_type::unsigned_integer` is unsafe. You may replace `get_uint64()` by a cast to a `uint64_t` value.
* `number.get_number_type()` has value `number_type::floating_point_number` if we have and we have a floating-point (binary64) number. You can recover the value by the `get_double()` method applied on the `ondemand::number` instance.  When `number.get_number_type()` has value `number_type::floating_point_number`, you also have that `number.is_double()` is true.  Calling `get_double()` on the `ondemand::number` instance when `number.get_number_type()` is not `number_type::floating_point_number` is unsafe. You may replace `get_double()` by a cast to a `double` value.
* When the value is an integer outside of the valid ranges for a 64-bit integers, e.g., when it is smaller than -9223372036854775808 or larger than 18446744073709551615, then `number.get_number_type()` has value `number_type::big_integer`. If you try to parse
such a number of `get_number()`, you get the error `BIGINT_ERROR`. You can access the underlying string of digits with the function `raw_json_token()` which returns a `std::string_view` instance starting at the beginning of the digit. You can also call `get_double()` to get a floating-point approximation.


You must check the type before accessing the value: it is an error to call `get_int64()` when `number.get_number_type()` is not `number_type::signed_integer` and when `number.is_int64()` is false. You are responsible for this check as the user of the library.

The `get_number()` function is designed with performance in mind. When calling `get_number()`, you scan the number string only once, determining efficiently the type and storing it in an efficient manner.


Consider the following example:
```C++
    ondemand::parser parser;
    padded_string docdata = R"([1.0, 3, 1, 3.1415,-13231232,9999999999999999999])"_padded;
    ondemand::document doc = parser.iterate(docdata);
    ondemand::array arr = doc.get_array();
    for(ondemand::value val : arr) {
      std::cout << val << " ";
      std::cout << "negative: " << val.is_negative() << " ";
      std::cout << "is_integer: " << val.is_integer() << " ";
      ondemand::number num = val.get_number();
      ondemand::number_type t = num.get_number_type();
      switch(t) {
        case ondemand::number_type::signed_integer:
          std::cout  << "integer: " << int64_t(num) << " ";
          std::cout  << "integer: " << num.get_int64() << std::endl;
          break;
        case ondemand::number_type::unsigned_integer:
          std::cout  << "large 64-bit integer: " << uint64_t(num) << " ";
          std::cout << "large 64-bit integer: " << num.get_uint64() << std::endl;
          break;
        case ondemand::number_type::floating_point_number:
          std::cout  << "float: " << double(num) << " ";
          std::cout << "float: " << num.get_double() << std::endl;
          break;
        case ondemand::number_type::big_integer:
          std::cout  << "big-integer: " << val.raw_json_token() << std::endl;
          break;
      }
    }
```

It will output:

```
1.0 negative: 0 is_integer: 0 float: 1 float: 1
3 negative: 0 is_integer: 1 integer: 3 integer: 3
1 negative: 0 is_integer: 1 integer: 1 integer: 1
3.1415 negative: 0 is_integer: 0 float: 3.1415 float: 3.1415
-13231232 negative: 1 is_integer: 1 integer: -13231232 integer: -13231232
9999999999999999999 negative: 0 is_integer: 1 large 64-bit integer: 9999999999999999999 large 64-bit integer: 9999999999999999999
```

In the following example, we have an array of integers that are outside the valid range of 64-bit signed or
unsigned integers. Calling `get_number_type()` on the values returns `ondemand::number_type::big_integer`.
You can try to represent these big integers as 64-bit floating-point numbers, though you typically lose
precision in the process (as illustrated in the example).

```C++
  ondemand::parser parser;
  padded_string docdata = R"([-9223372036854775809, 18446744073709551617, 99999999999999999999999 ])"_padded;
  double dexpected[] = {-9223372036854775808.0, 18446744073709551616.0, 1e23};
  ondemand::document doc = parser.iterate(docdata);
  ondemand::array arr = doc.get_array();
  for(ondemand::value val : arr) {
    if(val.get_number_type() == ondemand::number_type::big_integer) {
      std::cout << val.get_double() << std::endl;
      // might print -9.22337e+18, 1.84467e+19, 1e+23
    }
  }
```
This program might print:
```
-9.22337e+18
1.84467e+19
1e+23
```

You may get access to the underlying string representing the big integer with
`raw_json_token()` and you may parse the resulting number strings using your own parser.

```c++
  ondemand::parser parser;
  padded_string docdata = R"([-9223372036854775809, 18446744073709551617, 99999999999999999999999 ])"_padded;
  ondemand::document doc = parser.iterate(docdata);
  ondemand::array arr = doc.get_array();
  for(ondemand::value val : arr) {
    // val.get_number_type() == ondemand::number_type::big_integer
    if(val.get_number_type() == ondemand::number_type::big_integer) {
      std::string_view token = val.raw_json_token();
      // token = "-9223372036854775809", "18446744073709551617", "99999999999999999999999 "
      std::cout << "'" << token << "'" << std::endl;
    }
  }
```
This code prints the following:
```
'-9223372036854775809'
'18446744073709551617'
'99999999999999999999999 '
```

Raw strings from keys
-----------

It is sometimes useful to have access to a raw (unescaped) string: we make available a
minimalist `raw_json_string` data type which contains a pointer inside the string in the
original document, right after the quote. It is accessible via `get_raw_json_string()` on a
string instance and returned by the `key()` method on an object's field instance. It is always
optional: replacing `get_raw_json_string()` with `get_string()` and `key()` by
`unescaped_key()` or `escaped_key()` returns an `string_view` instance of the unescaped/unprocessed string.

You can quickly compare a `raw_json_string` instance with a target string. You may also
unescape  the `raw_json_string` on your own string buffer: `parser.unescape(mystr, ptr)`
advances the provided pointer `ptr` and returns a string_view instance on the newly serialized
string upon success, otherwise it returns an error. When unescaping to your own string buffer,
you should ensure that you have sufficient memory space: the total size of the strings plus
`simdjson::SIMDJSON_PADDING` bytes. The following example illustrates how we can unescape
JSON string to a user-provided buffer:

```C++
    auto json = R"( {"name": "Jack The Ripper \u0033"} )"_padded;
    // We create a buffer large enough to store all strings we need:
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[json.size() + simdjson::SIMDJSON_PADDING]);
    uint8_t * ptr = buffer.get();
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    // We store our strings as 'string_view' instances in a vector:
    std::vector<std::string_view> mystrings;
    for (auto key_value : doc.get_object()) {
      std::string_view keysv = parser.unescape(key_value.key(), ptr);// writes 'name'
      mystrings.push_back(keysv);
      std::string_view valuesv = parser.unescape(key_value.value().get_raw_json_string(), ptr);
      // writes 'Jack The Ripper 3', escaping the \u0033
      mystrings.push_back(valuesv);
    }
```

Some users might prefer to have a direct access to a `std::string_view` instance
pointing inside the source document. The `key_raw_json_token()` method serves this
purpose. It provides a view on the key, including the starting quote character,
and everything up to the next `:` character after the final quote character. E.g.,
if the key is `"name"` then `key_raw_json_token()` returns a `std::string_view`  which
begins with `"name"` and may containing trailing white-space characters.
```C++
  auto json = R"( {"name" : "Jack The Ripper \u0033"} )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  for (auto key_value : doc.get_object()) {
    std::string_view keysv = key_value.key_raw_json_token(); // keysv is "\"name\" "
  }
```


General direct access to the raw JSON string
--------------------------------
If your value is a string, the `raw_json_string` you get with `get_raw_json_string()` gives you direct access to the unprocessed
string. But the simdjson library allows you to have access to the raw underlying JSON
more generally, not just for strings.

The simdjson library makes explicit assumptions about types. For examples, numbers
must be integers (up to 64-bit integers) or binary64 floating-point numbers. Some users
have different needs. For example, some users might want to support big integers.
The library makes this possible by providing a `raw_json_token` method which returns
a `std::string_view` instance containing the value as a string which you may then
parse as you see fit.

```C++
simdjson::ondemand::parser parser;
simdjson::padded_string docdata =  R"({"value":12321323213213213213213213213211223})"_padded;
simdjson::ondemand::document doc = parser.iterate(docdata);
simdjson::ondemand::object obj = doc.get_object();
std::string_view token = obj["value"].raw_json_token();
// token has value 12321323213213213213213213213211223, it points inside the input string
```

The `raw_json_token` method even works when the JSON value is a string. In such cases, it
will return the complete string with the quotes and with eventual escaped sequences as in the
source document.

```C++
simdjson::ondemand::parser parser;
simdjson::padded_string docdata =  R"({"value":"12321323213213213213213213213211223"})"_padded;
simdjson::ondemand::document doc = parser.iterate(docdata);
simdjson::ondemand::object obj = doc.get_object();
string_view token = obj["value"].raw_json_token();
// token has value "12321323213213213213213213213211223", it points inside the input string
```

The `raw_json_token()` should be fast and free of allocation.

Given a quote-deliminated string, you find the string sequence inside the quote with a
single line of code:

```C++
std::string_view noquote(std::string_view v) { return {v.data()+1, v.find_last_of('"')-1}; }
```

If your value is an array or an object, `raw_json_token()` returns effectively a single
character (`[`) or (`}`) which is not very useful. For arrays and objects, we have another
method called `raw_json()` which consumes (traverses) the array or the object.

```C++
simdjson::ondemand::parser parser;
simdjson::padded_string docdata =  R"({"value":123})"_padded;
simdjson::ondemand::document doc = parser.iterate(docdata);
simdjson::ondemand::object obj = doc.get_object();
string_view token = obj.raw_json(); // gives you `{"value":123}`
```


```C++
simdjson::ondemand::parser parser;
simdjson::padded_string docdata =  R"([1,2,3])"_padded;
simdjson::ondemand::document doc = parser.iterate(docdata);
simdjson::ondemand::array arr = doc.get_array();
string_view token = arr.raw_json(); // gives you `[1,2,3]`
```

Because `raw_json()` consumes to object or the array, if you want to both have
access to the raw string, and also use the array or object, you should call `reset()`.

```C++
simdjson::ondemand::parser parser;
simdjson::padded_string docdata =  R"({"value":123})"_padded;
simdjson::ondemand::document doc = parser.iterate(docdata);
simdjson::ondemand::object obj = doc.get_object();
string_view token = obj.raw_json(); // gives you `{"value":123}`
obj.reset(); // revise the object
uint64_t x = obj["value"]; // gives me 123
```

You can use `raw_json()` with the values inside an array and object. When
calling `raw_json()` on an untyped value, it acts as `raw_json()` when the
value is an array or an object. Otherwise, it acts as `raw_json_token()`.
It is useful if you do not care for the type of the value and just wants a
string representation.

```C++
  auto json = R"( [1,2,"fds", {"a":1}, [1,344]] )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  size_t counter = 0;
  for(auto array: doc) {
    std::string_view raw = array.raw_json();
    // will capture "1", "2", "\"fds\"", "{\"a\":1}", "[1,344]"
  }
```

```C++
  auto json = R"( {"key1":1,"key2":2,"key3":"fds", "key4":{"a":1}, "key5":[1,344]} )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  size_t counter = 0;
  for(auto key_value: doc.get_object()) {
    std::string_view raw = key_value.value().raw_json();
    // will capture "1", "2", "\"fds\"", "{\"a\":1}", "[1,344]"
  }
```


You can use `raw_json()` to capture the content of some JSON values as `std::string_view`
instances which can be safely used later. The `std::string_view` instances point inside
the original document and do not depend in any way on simdjson. In the following example,
we store the `std::string_view` instances inside a `std::vector<std::string_view>` instance
and print the out after the parsing is concluded:

```cpp
  padded_string json_padded = "{\"a\":[1,2,3], \"b\": 2, \"c\": \"hello\"}"_padded;
  std::vector<std::string_view> fields;

  ondemand::parser parser;
  auto doc = parser.iterate(json_padded);
  auto object = doc.get_object();
  for (auto field : object) {
    fields.push_back(field.value().raw_json());
  }
  // Output the fields
  // Expected output:
  // [1,2,3]
  // 2
  // "hello"
  for (std::string_view field_ref : fields) {
    std::cout << field_ref << std::endl;
  }
  ```


Storing directly into an existing string instance
-----------------------------------------------------

The simdjson library favours  the use of `std::string_view` instances because
it tends to lead to better performance due to causing fewer memory allocations.
However, they are cases where you need to store a string result in a `std::string`
instance. You can do so with a templated version of the `to_string()` method which takes as
a parameter a reference to a `std::string`.

```C++
  auto json = R"({
  "name": "Daniel",
  "age": 42
})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::string name;
  doc["name"].get_string(name);
```

The same routine can be written without exceptions handling:

```C++
  std::string name;
  auto err = doc["name"].get_string(name);
  if (err) { /* handle error */ }
```

The `std::string` instance, once created, is independent. Unlike our `std::string_view` instances,
it does not point at data that is within our `parser` instance. The same caveat applies: you should
only consume a JSON string once.

Because `get_string()` is a template that requires a type that can be assigned a `std::string`, you
can use it with features such as `std::optional`:

```C++
  auto json = R"({ "foo1": "3.1416" } )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::optional<std::string> value;
  if (doc["foo1"].get_string(value)) { /* error */ }
  // value was populated with "3.1416"
```

You should be mindful of the trade-off: allocating multiple
`std::string` instances can become expensive.

Thread safety
-------------

We built simdjson with thread safety in mind.

The simdjson library is single-threaded except for [`iterate_many`](iterate_many.md) and [`parse_many`](parse_many.md) which may use secondary threads under their control when the library is compiled with thread support.


We recommend using one `parser` object per thread. When using the On-Demand front-end (our default), you should access the `document` instances in a single-threaded manner since it
acts as an iterator (and is therefore not thread safe).

The CPU detection, which runs the first time parsing is attempted and switches to the fastest
parser for your CPU, is transparent and thread-safe.
Our runtime dispatching is based on global objects that are instantiated at the beginning of the
main thread and may be discarded at the end of the main thread. If you have multiple threads running
and some threads use the library while the main thread is cleaning up ressources, you may encounter
issues. If you expect such problems, you may consider using [std::quick_exit](https://en.cppreference.com/w/cpp/utility/program/quick_exit).

In a threaded environment, stack space is often limited. Running code like simdjson in debug mode may require hundreds of kilobytes of stack memory. Thus stack overflows are a possibility. We recommend you turn on optimization when working in an environment where stack space is limited. If you must run your code in debug mode, we recommend you configure your system to have more stack space. We discourage you from running production code based on a debug build.


Standard compliance
--------------------

The simdjson library is fully compliant with  the [RFC 8259](https://www.tbray.org/ongoing/When/201x/2017/12/14/rfc8259.html) JSON specification.

- The only insignificant whitespace characters allowed are the space, the horizontal tab, the line feed and the carriage return. In particular, a JSON document may not contain an unescaped null character.
- A single string or a single number is considered to be a valid JSON document.
- We fully validate the numbers according to the JSON specification. For example,  the string `01` is not valid JSON document since the specification states that *leading zeros are not allowed*.
- The specification allows implementations to set limits on the range and precision of numbers accepted.  We support 64-bit floating-point numbers as well as integer values.
  - We parse integers and floating-point numbers as separate types which allows us to support all signed (two's complement) 64-bit integers, like a Java `long` or a C/C++ `long long` and all 64-bit unsigned integers. When we cannot represent exactly an integer as a signed or unsigned 64-bit value, we reject the JSON document.
  - We support the full range of 64-bit floating-point numbers (binary64). The values range from `std::numeric_limits<double>::lowest()`  to `std::numeric_limits<double>::max()`, so from -1.7976e308 all the way to 1.7975e308. Extreme values (less or equal to -1e308, greater or equal to 1e308) are rejected: we refuse to parse the input document. Numbers are parsed with a perfect accuracy (ULP 0): the nearest floating-point value is chosen, rounding to even when needed. If you serialized your floating-point numbers with 17 significant digits in a standard compliant manner, the simdjson library is guaranteed to recover the same numbers, exactly.
- The specification states that JSON text exchanged between systems that are not part of a closed ecosystem MUST be encoded using UTF-8. The simdjson library does full UTF-8 validation as part of the parsing. The specification states that implementations MUST NOT add a byte order mark: the simdjson library rejects documents starting with a  byte order mark.
- The simdjson library validates string content for unescaped characters. Unescaped line breaks and tabs in strings are not allowed.
- The simdjson library accepts objects with repeated keys: all of the name/value pairs, including duplicates, are reported. We do not enforce key uniqueness.
- The specification states that an implementation may set limits on the size of texts that it accepts. The simdjson library limits single JSON documents to 4 GiB. It will refuse to parse a JSON document larger than 4294967295 bytes. (This limitation does not apply to streams of JSON documents, only to single JSON documents.)
- The specification states that an implementation may set limits on the maximum depth of nesting. By default, the simdjson will refuse to parse documents with a depth exceeding 1024.


Backwards compatibility
-----------------------

The only header file supported by simdjson is `simdjson.h`. Older versions of simdjson published a
number of other include files such as `document.h` or `ParsedJson.h` alongside `simdjson.h`; these headers
may be moved or removed in future versions.

Examples
--------

Some users like to have example. The following code samples illustrate how to process specific JSON inputs.
For simplicity, we do not include full error support: this code would throw exceptions on error.


* Example 1: ZuluBBox

```C++
struct ZuluBBox {
  double xmin;
  double ymin;
  double width;
  double height;

  void print() {
    std::cout << xmin << ", " << ymin << ", " << width << ", " << height
              << std::endl;
  }
};

bool example() {

  auto json = R"+( {
  "ZuluROI": {
    "ZuluBBox": {
      "xmin": 0,
      "ymin": 0,
      "width": 1,
      "height": 1
    },
    "SubObjects": [
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.7587034106254578,
          "ZuluBBox": {
            "xmin": 0.3843536376953125,
            "ymin": 0.4532909393310547,
            "width": 0.09115534275770187,
            "height": 0.04127710685133934
          },
          "SubObjects": []
        }
      },
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.6718865633010864,
          "ZuluBBox": {
            "xmin": 0.7500002980232239,
            "ymin": 0.5212296843528748,
            "width": 0.07592231780290604,
            "height": 0.038947589695453644
          },
          "SubObjects": []
        }
      },
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.5806200504302979,
          "ZuluBBox": {
            "xmin": 0.9025363922119141,
            "ymin": 0.5925348401069641,
            "width": 0.05478987470269203,
            "height": 0.046337299048900604
          },
          "SubObjects": []
        }
      }
    ]
  },
  "timestamp (ms)": 1677085594421,
  "buffer_offset": 35673
} )+"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::object root_object = doc.get_object();
  ondemand::object roi_object = root_object["ZuluROI"];

  ondemand::object box_roi_object = roi_object["ZuluBBox"];
  ZuluBBox box = {
      double(box_roi_object["xmin"]), double(box_roi_object["ymin"]),
      double(box_roi_object["width"]), double(box_roi_object["height"])};
  box.print();

  for (ondemand::object value : roi_object["SubObjects"]) {
    ondemand::object detect = value["ZuluDetection"];
    std::cout << detect["label"].get_string() << std::endl;
    std::cout << detect["class_id"].get_uint64() << std::endl;
    std::cout << detect["confidence"].get_double() << std::endl;

    ondemand::object vbox_roi_object = detect["ZuluBBox"];
    ZuluBBox vbox = {
        double(vbox_roi_object["xmin"]), double(vbox_roi_object["ymin"]),
        double(vbox_roi_object["width"]), double(vbox_roi_object["height"])};
    vbox.print();
  }

  std::cout << root_object["timestamp (ms)"].get_uint64() << std::endl;
  std::cout << root_object["buffer_offset"].get_uint64() << std::endl;
  return true;
}
```


* Example 2: Demos

```C++
bool example() {
  auto json = R"+( {
    "5f08a730b280e54fd1e75a7046b93fdc": {
        "file": "/DEMOS/0-9/10_Orbyte.sid",
        "len": [
            "1:17"
        ],
        "loud": [
            "-22.8"
        ],
        "name": "10 Orbyte",
        "author": "Michael Becker (Premium)",
        "release": "2014 Tristar & Red Sector Inc.",
        "bits": 20
    },
    "2727236ead44a62f0c6e01f6dd4dc484": {
        "file": "/DEMOS/0-9/12345.sid",
        "len": [
            "0:56"
        ],
        "loud": [
            "-33.3"
        ],
        "name": "12345",
        "author": "Beal",
        "release": "1988 Beal",
        "bits": 20
    },
    "7ea765fce6c0f92570b18adc7bf52f54": {
        "file": "/DEMOS/0-9/128_Byte_Blues_BASIC.sid",
        "len": [
            "0:18"
        ],
        "loud": [
            "-27.1"
        ],
        "name": "128 Byte Blues",
        "author": "Leonard J. Paul (Freaky DNA)",
        "release": "2005 Freaky DNA",
        "bits": 62
    }
} )+"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::object root_object = doc.get_object();
  for(auto key_value : root_object) {
    // could get std::string_view with 'unescaped_key()' or 'escaped_key()':
    std::cout << "key: " << key_value.key() << std::endl;
    ondemand::object obj = key_value.value();

    std::cout << "file: " << std::string_view(obj["file"]) << std::endl;

    std::cout << "len: ";
    for(std::string_view values : obj["len"]) {
      std::cout << values << std::endl;
    }
    std::cout << std::endl;

    std::cout << "loud: ";
    for(std::string_view values : obj["loud"]) {
      std::cout << values << std::endl;
    }
    std::cout << std::endl;

    std::cout << "name: " << std::string_view(obj["name"]) << std::endl;
    std::cout << "author: " << std::string_view(obj["author"]) << std::endl;
    std::cout << "release: " << std::string_view(obj["release"]) << std::endl;
    std::cout << "bits: " << uint64_t(obj["bits"]) << std::endl;
  }
  return true;
}
```

* Example 3: CRT

```C++

bool example() {
  padded_string padded_input_json = R"([
	{ "monitor": [
		{ "id": "monitor",		"type": "toggle",		"label": "monitor"			},
		{ "id": "profile",		"type": "selector",		"label": "collection"		},
		{ "id": "overlay",		"type": "selector",		"label": "overlay"			},
		{ "id": "zoom",			"type": "toggleSlider",	"label": "zoom"				}
	] },

	{ "crt": [
		{ "id": "system",		"type": "multi",		"label": "system",		"choices": "PAL, NTSC"	},
		{ "type": "spacer" },
		{ "id": "brightness",	"type": "slider",		"icon": "brightness"		},
		{ "id": "contrast",		"type": "slider",		"icon": "contrast"			},
		{ "id": "saturation",	"type": "slider",		"icon": "saturation"		},
		{ "type": "spacer" },
		{ "id": "overscan",		"type": "toggleSlider",	"label": "overscan"			},
		{ "type": "spacer" },
		{ "id": "emulation",	"type": "toggle",		"label": "CRT emulation"	},
		{ "type": "spacer" },
		{ "id": "curve",		"type": "toggleSlider",	"label": "curve"			},
		{ "id": "bleed",		"type": "toggleSlider",	"label": "bleed"			},
		{ "id": "vignette",		"type": "toggleSlider",	"label": "vignette"			},
		{ "id": "scanlines",	"type": "toggleSlider",	"label": "scanlines"		},
		{ "id": "gridlines",	"type": "toggleSlider",	"label": "gridlines"		},
		{ "id": "glow",			"type": "toggleSlider",	"label": "glow"				},
		{ "id": "flicker",		"type": "toggleSlider",	"label": "flicker"			},
		{ "id": "noise",		"type": "toggleSlider",	"label": "noise"			},
    {}
	] }
])"_padded;
  auto parser = ondemand::parser{};
  auto doc = parser.iterate(padded_input_json);
  auto root_array = doc.get_array();
  // the root should be an object, not an array, but that's the JSON we are
  // given.
  for (ondemand::object node : root_array) {
    // We know that we are going to have just one element in the object.
    for (auto field : node) {
      std::cout << "\n\ntop level:" << field.key() << std::endl;
      // You can get a proper std::string_view for the key with:
      // std::string_view key = field.unescaped_key();
      // or
      // std::string_view key = field.escaped_key();
      // and second for-range loop to get child-elements here
      for (ondemand::object inner_object : field.value()) {
        auto i = inner_object.begin();
        if (i == inner_object.end()) {
          std::cout << "empty object" << std::endl;
          continue;
        } else {
          for (; i != inner_object.end(); ++i) {
            auto inner_field = *i;
            std::cout << '"' << inner_field.key()
                      << "\" : " << inner_field.value() << ", ";
            // You can get proper std::string_view for the key and value with:
            // std::string_view inner_key = field.unescaped_key();
            // or
            // std::string_view inner_key = field.escaped_key();
            // and
            // std::string_view value_str = field.value();
          }
        }
        std::cout << std::endl;
      }
      // You can break here if you only want just the first element.
      // break;
    }
  }
  return true;
}
```

* Example 4: Passing an array to a function

```C++

#include "simdjson.h"
#include <iostream>

// prints the content of the array as hexadecimal 64-bit integers
void f(simdjson::ondemand::array v) {
  for(uint64_t val : v) {
    std::cout << "0x" << std::hex << val << std::endl;
  }
}


int main(void) {
  simdjson::padded_string json = R"( [ 897314173811950000, 3122321 ])"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  f(doc.get_array());
  return EXIT_SUCCESS;
}
```


* Example 4: Value capture with `std::string_view` instances

```cpp
void example() {
  ondemand::parser parser;
  const padded_string json = R"({ "parent": {"child1": {"name": "John"} , "child2": {"name": "Daniel"}} })"_padded;
  auto doc = parser.iterate(json);
  ondemand::object parent = doc["parent"];
  // parent owns the focus
  ondemand::object c1 = parent["child1"];
  // c1 owns the focus
  //
  std::string_view as1 = c1["name"];
  // We have that as1 == "John", as long as 'parser' and 'json' live
  // c2 attempts to grab the focus from parent but fails
  ondemand::object c2 = parent["child2"];
  // c2 owns the focus, at this point c1 is invalid
  std::string_view as2 = c2["name"];
  // We have that as2 == "Daniel", as long as 'parser' and 'json' live
  std::cout << as1 << " " << as2 << std::endl; // prints John Daniel
}
```

Performance tips
--------


- Read [our performance notes](performance.md) for advanced topics.
- To better understand the operation of your On-Demand parser, and whether it is performing as well as you think it should be, there is a  logger feature built in to simdjson! To use it, define the pre-processor directive `SIMDJSON_VERBOSE_LOGGING` prior to including the `simdjson.h` header, which enables logging in simdjson. Run your code. It may generate a lot of logging output; adding printouts from your application that show each section may be helpful. The log's output will show step-by-step information on state, buffer pointer position, depth, and key retrieval status. Importantly, unless `SIMDJSON_VERBOSE_LOGGING` is defined, logging is entirely disabled and thus carries no overhead.
- The On-Demand front-end works best when doing a single pass over the input: avoid calling `count_elements`, `rewind`, `reset` and similar methods.
- If you are familiar with assembly language, you may use the online tool godbolt to explore the compiled code. The following example may work: [https://godbolt.org/z/xE4GWs573](https://godbolt.org/z/xE4GWs573).
- Given a field `field` in an object, calling `field.key()` is often faster than `field.unescaped_key()` so if you do not need an unescaped `std::string_view` instance, prefer `field.key()`. Similarly, we expect `field.escaped_key()` to be faster than `field.unescaped_key()` even though both return a `std::string_view` instance.
- For release builds, we recommend setting `NDEBUG` pre-processor directive when compiling the `simdjson` library. Importantly, using the optimization flags `-O2` or `-O3` under GCC and LLVM clang does not set the `NDEBUG` directive, you must set it manually (e.g., `-DNDEBUG`).
- For long streams of JSON documents, consider [`iterate_many`](iterate_many.md) and [`parse_many`](parse_many.md) for better performance.
- Never seek to access a field twice (e.g., o["data"] and later again o["data"]). Instead capture once an ondemand::value and reuse it.
- If you must access several different keys in an object, it might be preferable to iterate through all the fields in the object instead, and branch on the field keys.
- If possible, refer to each object and array in your code once. For example, the following code repeatedly refers to the `"data"` key to create an object...
	```C++
	std::string_view make = o["data"]["make"];
	std::string_view model = o["data"]["model"];
	std::string_view year = o["data"]["year"];
  ```
  We expect that it is more efficient to access the `"data"` key once:
  ```C++
	simdjson::ondemand::object data = o["data"];
	std::string_view model = data["model"];
	std::string_view year = data["year"];
	std::string_view rating = data["rating"];
  ```
- You will get better performance if you seek the keys in the order in which they appear in the document. So if processing `{"a":1, "b":2, "c":3}`, do `value1 = data["a"]; value2 = data["b"]; value3 data["c"];` and not `value2 = data["b"]; value1 = data["a"]; value3 data["c"];`. Of course, it is not always possible to know for sure in which order the keys appear.



Further reading
--------


- John Keiser, Daniel Lemire, [On-Demand JSON: A Better Way to Parse Documents?](http://arxiv.org/abs/2312.17149), Software: Practice and Experience 54 (6), 2024
