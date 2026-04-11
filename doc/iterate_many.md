iterate_many
==========

When serializing large databases, it is often better to write out many independent JSON
documents, instead of one large monolithic document containing many records. The simdjson
library provides high-speed access to files or streams containing multiple small JSON documents separated by ASCII white-space characters. Given an input such as
```JSON
{"text":"a"}
{"text":"b"}
{"text":"c"}
"..."
```
... you want to read the entries (individual JSON documents) as quickly and as conveniently as possible. Importantly, the input might span several gigabytes, but you want to use a small (fixed) amount of memory. Ideally, you'd also like the parallelize the processing (using more than one core) to speed up the process.

Contents
--------

- [Motivations](#motivations)
- [How it works](#how-it-works)
  - [Context](#context)
  - [Design](#design)
  - [Threads](#threads)
- [Support](#support)
- [API](#api)
- [Streaming directly from a memory-mapped file](#streaming-directly-from-a-memory-mapped-file)
- [Use cases](#use-cases)
- [Tracking your position](#tracking-your-position)
- [Incomplete streams](#incomplete-streams)
- [C++20 features](#c20-features)
- [C++26 features (static reflection)](#c26-features-static-reflection)

Motivation
-----------

The main motivation for this piece of software is to achieve maximum speed and offer a
better quality of life in parsing files containing multiple small JSON documents.

The JavaScript Object Notation (JSON) [RFC7159](https://tools.ietf.org/html/rfc7159) is a handy
serialization format.  However, when serializing a large sequence of
values as an array, or a possibly indeterminate-length or never-
ending sequence of values, JSON may be inconvenient.

Consider a sequence of one million values, each possibly one kilobyte
when encoded -- roughly one gigabyte.  It is often desirable to process such a dataset incrementally
without having to first read all of it before beginning to produce results.


How it works
------------

### Context

Before parsing anything, simdjson first preprocesses the JSON text by identifying all structural indexes
(i.e. the starting position of any JSON value, as well as any important operators like `,`, `:`, `]` or
`}`) and validating UTF8. This stage is referred to stage 1. However, during this process, simdjson has
no knowledge of whether parsed a valid document, multiple documents, or even if the document is complete.
Then, to iterate through the JSON text during parsing, we use what we call a JSON iterator that will navigate
through the text using these structural indexes. This JSON iterator is not visible though, but it is the
key component to make parsing work.

Prior to iterate_many, most people who had to parse a multiline JSON file would proceed by reading the
file line by line, using a utility function like `std::getline` or equivalent, and would then use
the `parse` on each of those lines.  From a performance point of view, this process is highly
inefficient,  in that it requires a lot of unnecessary memory allocation and makes use of the
`getline` function, which is fundamentally slow, slower than the act of parsing with simdjson
[(more on this here)](https://lemire.me/blog/2019/06/18/how-fast-is-getline-in-c/).

Unlike the popular parser RapidJson, our DOM does not require the buffer once the parsing job is
completed,  the DOM and the buffer are completely independent. The drawback of this architecture is
that we need to allocate some additional memory to store our ParsedJson data, for every document
inside a given file.  Memory allocation can be slow and become a bottleneck, therefore, we want to
minimize it as much as possible.

### Design

To achieve a minimum amount of allocations, we opted for a design where we create only one
parser object and therefore allocate its memory once, and then recycle it for every document in a
given file. But, knowing that they often have largely varying size, we need to make sure that we
allocate enough memory so that all the documents can fit. This value is what we call the batch size.
As of right now, we need to manually specify a value for this batch size, it has to be at least as
big as the biggest document in your file, but not too big so that it submerges the cached memory.
The bigger the batch size, the fewer we need to make allocations. We found that 1MB is somewhat a
sweet spot.

1. When the user calls `iterate_many`, we return a `document_stream` which the user can iterate over
   to receive parsed documents.
2. We call stage 1 on the first batch_size bytes of JSON in the buffer, detecting structural
	indexes for all documents in that batch.
3. The `document_stream` owns a `document` instance that keeps track of the current document position
	in the stream using a JSON iterator. To obtain a valid document, the `document_stream` returns a
	**reference** to its document instance.
4. Each time the user calls `++` to read the next document, the JSON iterator moves to the start the
	next document.
5. When we reach the end of the batch, we call stage 1 on the next batch, starting from the end of
   the last document, and go to step 3.

### Threads

But how can we make use of threads if they are available?  We found a pretty cool algorithm that allows
us to quickly identify the  position of the last JSON document in a given batch. Knowing exactly where
the end of the last document in the batch is, we can safely parse through the last document without any
worries that it might be incomplete. Therefore, we can run stage 1 on the next batch concurrently while
parsing the documents in the current batch. Running stage 1 in a different thread can, in best cases,
remove almost entirely its cost and replaces it by the overhead of a thread, which is orders of magnitude
cheaper. Ain't that awesome!

Thread support is only active if thread supported is detected in which case the macro
SIMDJSON_THREADS_ENABLED is set.  You can also manually pass `SIMDJSON_THREADS_ENABLED=1` flag
to the library. Otherwise the library runs in single-thread mode.

You should be consistent. If you link against the simdjson library built for multithreading
(i.e., with `SIMDJSON_THREADS_ENABLED`), then you should build your application with multithreading
system (setting `SIMDJSON_THREADS_ENABLED=1` and linking against a thread library).

A `document_stream` instance uses at most two threads: there is a main thread and a worker thread.

Support
-------

Since we want to offer flexibility and not restrict ourselves to a specific file
format, we support any file that contains any amount of valid JSON document, **separated by one
or more character that is considered whitespace** by the JSON spec. Anything that is
 not whitespace will be parsed as a JSON document and could lead to failure.

Whitespace Characters:
- **Space**
- **Linefeed**
- **Carriage return**
- **Horizontal tab**

If your documents are all objects or arrays, then you may even have nothing between them.
E.g., `[1,2]{"32":1}` is recognized as two documents.

Some official formats **(non-exhaustive list)**:
- [Newline-Delimited JSON (NDJSON)](https://github.com/ndjson/ndjson-spec/)
- [JSON lines (JSONL)](http://jsonlines.org/)
- [Record separator-delimited JSON (RFC 7464)](https://tools.ietf.org/html/rfc7464)
- [More on Wikipedia...](https://en.wikipedia.org/wiki/JSON_streaming)

API
---

Example:

```cpp
//  R"( ... )" is a C++ raw string literal.
auto json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )"_padded;
// _padded returns an simdjson::padded_string instance
ondemand::parser parser;
ondemand::document_stream docs = parser.iterate_many(json);
for (auto doc : docs) {
  std::cout << doc["foo"] << std::endl;
}
// Prints 1 2 3
```

See [basics.md](basics.md#newline-delimited-json-ndjson-and-json-lines) for an overview of the API.


Streaming directly from a memory-mapped file
--------------------------------------------

When your input is a large NDJSON / JSON-lines file on disk, the most efficient
way to feed `iterate_many` is to use `simdjson::padded_memory_map`. It returns
a `padded_string_view` with the right amount of trailing padding, so you can
hand it straight to `iterate_many` without ever copying the file contents into
your own buffer.

`padded_memory_map` is available on POSIX systems (Linux, macOS, BSD, ...) by
default. On Windows it is an **opt-in** feature with the following
requirements:

1. Build simdjson with `-DSIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS=ON`, or — if
   you consume simdjson as a pre-built library — define
   `SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS=1`, raise `NTDDI_VERSION` to at
   least `NTDDI_WIN10_RS4` (`0x0A000005`, Windows 10 version 1803), and
   add `onecore.lib` to your link line yourself. The Windows
   implementation uses the modern memory APIs `CreateFileMapping2` /
   `MapViewOfFile3`, which are available starting with that version of
   Windows and are exported by `onecore.lib`.
2. `#include <windows.h>` before `#include "simdjson.h"` in every
   translation unit where you want to use `padded_memory_map`. simdjson
   deliberately does not pull in `<windows.h>` itself, so the class is
   only declared when the Win32 types are already visible.

If either requirement is not met on Windows, the `padded_memory_map` class is
not declared at all and any code that references it fails to compile with an
"unknown identifier" error. The availability of the class can be tested with
the macro `SIMDJSON_HAS_PADDED_MEMORY_MAP`.

On POSIX, `padded_memory_map` uses `mmap` to map the file directly into
memory with zero copies. On Windows (when enabled), it uses
`CreateFileMapping2` + `MapViewOfFile3` for true zero-copy mapping
whenever the file does not end within `SIMDJSON_PADDING` bytes of a page
boundary; for those rare cases, it transparently falls back to reading
the file into a heap-allocated padded buffer so that the returned view
always has `SIMDJSON_PADDING` accessible zero bytes after the file content.

```cpp
#ifdef _WIN32
#include <windows.h> // Must come BEFORE <simdjson.h> on Windows
#endif
#include "simdjson.h"

// ...

simdjson::padded_memory_map map("huge_stream.ndjson");
if (!map.is_valid()) { /* file missing, unreadable, too large, ... */ return; }

simdjson::ondemand::parser parser;
simdjson::ondemand::document_stream stream;
auto error = parser.iterate_many(map.view()).get(stream);
if (error) { std::cerr << error << std::endl; return; }

for (auto doc : stream) {
  // process each JSON document in the stream
  std::cout << doc << std::endl;
}
```

Important lifetime rule: the `padded_string_view` returned by `map.view()` is
only valid while the `padded_memory_map` instance is alive, so keep `map`
alive for as long as you are iterating the stream.

The file must not be modified while the memory map is in use. If you need a
fully independent copy of the data, use `simdjson::padded_string::load(...)`
instead.

If you prefer single-document parsing on a memory-mapped file, the same
pattern applies to `parser.iterate(...)`:

```cpp
simdjson::padded_memory_map map(myfilename);
if (!map.is_valid()) { /* handle error */ }
simdjson::padded_string_view view = map.view(); // view is usable while padded_memory_map is in scope
ondemand::document doc = parser.iterate(view); // parse the JSON
```

## Use cases

From [jsonlines.org](http://jsonlines.org/examples/):

- **Better than CSV**
    ```json
    ["Name", "Session", "Score", "Completed"]
    ["Gilbert", "2013", 24, true]
    ["Alexa", "2013", 29, true]
    ["May", "2012B", 14, false]
    ["Deloise", "2012A", 19, true]
    ```
    CSV seems so easy that many programmers have written code to generate it themselves, and almost every implementation is
    different. Handling broken CSV files is a common and frustrating task. CSV has no standard encoding, no standard column
    separator and multiple character escaping standards. String is the only type supported for cell values, so some programs
     attempt to guess the correct types.

    JSON Lines handles tabular data cleanly and without ambiguity. Cells may use the standard JSON types.

    The biggest missing piece is an import/export filter for popular spreadsheet programs so that non-programmers can use
    this format.

- **Easy Nested Data**
    ```json
    {"name": "Gilbert", "wins": [["straight", "7♣"], ["one pair", "10♥"]]}
    {"name": "Alexa", "wins": [["two pair", "4♠"], ["two pair", "9♠"]]}
    {"name": "May", "wins": []}
    {"name": "Deloise", "wins": [["three of a kind", "5♣"]]}
    ```
    JSON Lines' biggest strength is in handling lots of similar nested data structures. One .jsonl file is easier to
    work with than a directory full of XML files.


Tracking your position
-----------

Some users would like to know where the document they parsed is in the input array of bytes.
It is possible to do so by accessing directly the iterator and calling its `current_index()`
method which reports the location (in bytes) of the current document in the input stream.
You may also call the `source()` method to get a `std::string_view` instance on the document
and `error()` to check if there were any error.

Let us illustrate the idea with code:


```cpp
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2,3]  )"_padded;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
    auto error = parser.iterate_many(json).get(stream);
    if (error) { /* do something */ }
    auto i = stream.begin();
	 size_t count{0};
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if (!i.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << i.source() << std::endl;
          count++;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
          return false;
        }
    }

```

This code will print:
```
got full document at 0
[1,2,3]
got full document at 9
{"1":1,"2":3,"4":4}
got full document at 29
[1,2,3]
```


Incomplete streams
-----------

Some users may need to work with truncated streams. The simdjson may truncate documents at the very end of the stream that cannot possibly be valid JSON (e.g., they contain unclosed strings, unmatched brackets, unmatched braces). After iterating through the stream, you may query the `truncated_bytes()` method which tells you how many bytes were truncated. If the stream is made of full (whole) documents, then you should expect `truncated_bytes()` to return zero.


Consider the following example where a truncated document (`{"key":"intentionally unclosed string  `) containing 39 bytes has been left within the stream. In such cases, the first two whole documents are parsed and returned, and the `truncated_bytes()` method returns 39.

```cpp
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} {"key":"intentionally unclosed string  )"_padded;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
    auto error = parser.iterate_many(json,json.size()).get(stream);
    if (error) { std::cerr << error << std::endl; return; }
    for(auto i = stream.begin(); i != stream.end(); ++i) {
       std::cout << i.source() << std::endl;
    }
    std::cout << stream.truncated_bytes() << " bytes "<< std::endl; // returns 39 bytes
```

This will print:
```
[1,2,3]
{"1":1,"2":3,"4":4}
39 bytes
```

Importantly, you should only call `truncated_bytes()` after iterating through all of the documents since the stream cannot tell whether there are truncated documents at the very end when it may not have accessed that part of the data yet.

Comma-separated documents
-----------

To parse comma-separated documents like `{"a":1},{"b":2},{"c":3}`, use the `stream_format::comma_delimited` parameter:

```cpp
auto json = R"({"a":1},{"b":2},{"c":3})"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::comma_delimited).get(stream);
if (error) { std::cerr << error << std::endl; return; }
for (auto doc : stream) {
    std::cout << doc << std::endl;
}
// Prints: {"a":1}
//         {"b":2}
//         {"c":3}
```

Whitespace around the commas is allowed:
```cpp
auto json = R"({"a":1} , {"b":2} , {"c":3})"_padded;  // Also works
```

Nested commas inside objects and arrays are preserved:
```cpp
auto json = R"({"arr":[1,2,3]},{"obj":{"x":1,"y":2}})"_padded;
// Correctly parses as 2 documents, not 6
```

Mixed document types are supported:
```cpp
auto json = R"(1, 2, 3, 4, "a", "b", "c", {"hello": "world"}, [1, 2, 3])"_padded;
ondemand::parser parser;
ondemand::document_stream doc_stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::comma_delimited).get(doc_stream);
if (error) { std::cerr << error << std::endl; return; }
for (auto doc : doc_stream) {
    std::cout << doc.type() << std::endl;
}
// Prints: number number number number string string string object array
```

Extra top-level separators are tolerated for compatibility with the legacy
`allow_comma_separated` behavior. For example, leading commas, trailing commas,
and repeated commas are treated as empty separators rather than documents.

### Legacy `allow_comma_separated` parameter (deprecated)

The `allow_comma_separated` boolean parameter is deprecated. When set to `true`, it now internally maps to `stream_format::comma_delimited`.

The old single-batch limitation no longer applies - comma-delimited parsing now supports multi-batch processing and threading for optimal performance on large files.

JSON Text Sequences (RFC 7464)
------------------------------

[RFC 7464](https://tools.ietf.org/html/rfc7464) defines a format for streaming JSON values using ASCII Record Separator (RS, 0x1E) as a delimiter. Each JSON text is preceded by RS and optionally followed by ASCII Line Feed (LF, 0x0A).

Example input:
```
<RS>{"name":"doc1"}<LF>
<RS>{"name":"doc2"}<LF>
<RS>{"name":"doc3"}<LF>
```

To parse JSON text sequences, use the `stream_format::json_sequence` parameter:

```cpp
// Build input with RS (0x1E) and LF (0x0A) delimiters
std::string input_str;
input_str += '\x1e'; input_str += "{\"a\":1}"; input_str += '\x0a';
input_str += '\x1e'; input_str += "{\"b\":2}"; input_str += '\x0a';
input_str += '\x1e'; input_str += "{\"c\":3}"; input_str += '\x0a';
simdjson::padded_string input(input_str);

ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::json_sequence).get(stream);
if (error) { std::cerr << error << std::endl; return; }
for (auto doc : stream) {
    std::cout << doc << std::endl;
}
```

The `stream_format` enum has the following values:
- `stream_format::whitespace_delimited` (default): Standard NDJSON/JSON Lines format
- `stream_format::json_sequence`: RFC 7464 format with RS delimiters
- `stream_format::comma_delimited`: Comma-separated JSON documents
- `stream_format::comma_delimited_array`: A single JSON array whose elements are iterated as comma-delimited documents (see below)

The trailing LF after each JSON text is optional but recommended by the RFC for robustness.

JSON Array As A Document Stream
-------------------------------

Sometimes an input is a single, well-formed JSON array — `[{"a":1},{"b":2},{"c":3}]` — but you want to iterate its elements one at a time without materializing the whole array. Use `stream_format::comma_delimited_array`:

```cpp
auto json = R"([{"a":1},{"b":2},{"c":3}])"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::comma_delimited_array).get(stream);
if (error) { std::cerr << error << std::endl; return; }
for (auto doc : stream) {
    std::cout << doc << std::endl;
}
// Prints: {"a":1}
//         {"b":2}
//         {"c":3}
```

The parser strips the outer `[` and `]` plus any surrounding JSON whitespace (space, tab, LF, CR) and then behaves exactly like `stream_format::comma_delimited` over the remaining bytes. All comma-delimited features are inherited: multi-batch processing, threading, mixed scalar types, and nested commas preserved inside inner objects and arrays.

```cpp
// All of these work:
auto a = R"([1, "x", true, null, {"k":"v"}, [1,2]])"_padded;  // mixed scalars
auto b = R"(  [ 1, 2, 3 ] )"_padded;                          // whitespace
auto c = R"([])"_padded;                                      // empty array → 0 docs
```

If the input is not a well-formed outer array (missing `[`, missing `]`, or empty / all-whitespace), `iterate_many` returns `TAPE_ERROR`. Content **inside** the array is not validated up front — individual document parse errors surface when you iterate, just like `comma_delimited`.

Positions reported via `current_index()` are relative to the **stripped** buffer (the bytes between `[` and `]`), not the original input, for consistency with the existing BOM-stripping behavior.


C++20 features
--------------------

In C++20, the standard introduced the notion of *customization point*.
A customization point is a function or function object that can be customized for different types. It allows library authors to provide default behavior while giving users the ability to override this behavior for specific types.

A tag_invoke function serves as a mechanism for customization points. It is not directly part of the C++ standard library but is often used in libraries that implement customization points.
The tag_invoke function is typically a generic function that takes a tag type and additional arguments.
The first argument is usually a tag type (often an empty struct) that uniquely identifies the customization point (e.g., deserialization of custom types in simdjson). Users or library providers can specialize tag_invoke for their types by defining it in the appropriate namespace, often inline namespace.



You can deserialize you own data structures conveniently if your system supports C++20.
When it is the case, the macro `SIMDJSON_SUPPORTS_CONCEPTS` will be set to 1 by
the simdjson library.

Consider a custom class `Car`:

```cpp
struct Car {
  std::string make;
  std::string model;
  int year;
  std::vector<float> tire_pressure;
};
```


You may support deserializing directly from a JSON value or document to your own `Car` instance
by defining a single `tag_invoke` function:


```cpp
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

Importantly, the `tag_invoke` function must be inside the `simdjson` namespace.
Let us explain each argument of `tag_invoke` function.

- `simdjson::deserialize_tag`: it is the tag for Customization Point Object (CPO). You may often ignore this parameter. It is used to indicate that you mean to provide a deserialization function for simdjson.
- `var`: It receives automatically a `simdjson` value type (document, value, document_reference).
- The third parameter is an instance of the type that you want to support.

Please see our main documentation (`basics.md`) under
"Use `tag_invoke` for custom types (C++20)" for details about
tag_invoke functions.

Given a stream of JSON documents, you can add them to a data structure
such as a `std::vector<Car>` like so if you support exceptions:

```cpp
  padded_string json =
      R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] }
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
)"_padded;
  ondemand::parser parser;
  ondemand::document_stream stream;
  [[maybe_unused]] auto error = parser.iterate_many(json).get(stream);
  std::vector<Car> cars;
  for(auto doc : stream) {
    cars.push_back((Car)doc); // an exception may be thrown
  }
```

Otherwise you may use this longer version for explicit handling of errors:


```cpp
  std::vector<Car> cars;
  for(auto doc : stream) {
    Car c;
    if ((error = doc.get<Car>().get(c))) {
      std::cerr << simdjson::error_message(error); << std::endl;
      return EXIT_FAILURE;
    }
    cars.push_back(c);
  }
```

**Performance tip**: You will get better performance if you order the attributes (make, model)
in the order they appear in the JSON document.

C++26 features (static reflection)
-----------------------------------

If you have a C++26 compatible compiler with [P2996](https://wg21.link/P2996)
static reflection support, you can compile the simdjson library with the
`SIMDJSON_STATIC_REFLECTION` macro set to `1`. When this is the case, simdjson
can deserialize a stream of JSON documents directly into your own structures
**without** writing any `tag_invoke` function. The library inspects the
non-static public members of your type at compile time and produces the
parsing code automatically.

```cpp
#define SIMDJSON_STATIC_REFLECTION 1
#include "simdjson.h"
```

Consider the same `Car` structure used in the C++20 example, but **without**
any `tag_invoke` glue:

```cpp
struct Car {
  std::string make;
  std::string model;
  int year;
  std::vector<double> tire_pressure;
};
```

With C++26 static reflection enabled, you can iterate a stream of cars and
push them into a `std::vector<Car>` directly:

```cpp
auto json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
                  "tire_pressure": [ 40.1, 39.9 ] }
                { "make": "Kia",    "model": "Soul",   "year": 2012,
                  "tire_pressure": [ 30.1, 31.0 ] }
                { "make": "Toyota", "model": "Tercel", "year": 1999,
                  "tire_pressure": [ 29.8, 30.0 ] } )"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json).get(stream);
if (error) { /* handle error */ }
std::vector<Car> cars;
for (auto doc : stream) {
  Car c;
  if ((error = doc.get<Car>().get(c))) { /* handle error */ }
  cars.push_back(c);
}
```

This works for every `stream_format` value supported by `iterate_many`. The
following examples each parse the same three cars, but laid out using a
different streaming convention.

### Whitespace-delimited (default, NDJSON / JSON Lines)

```cpp
auto json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
                  "tire_pressure": [ 40.1, 39.9 ] }
                { "make": "Kia",    "model": "Soul",   "year": 2012,
                  "tire_pressure": [ 30.1, 31.0 ] }
                { "make": "Toyota", "model": "Tercel", "year": 1999,
                  "tire_pressure": [ 29.8, 30.0 ] } )"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::whitespace_delimited).get(stream);
if (error) { /* handle error */ }
std::vector<Car> cars;
for (auto doc : stream) {
  cars.push_back((Car)doc); // throws on error
}
```

### Comma-delimited documents

```cpp
auto json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
                  "tire_pressure": [ 40.1, 39.9 ] },
                { "make": "Kia",    "model": "Soul",   "year": 2012,
                  "tire_pressure": [ 30.1, 31.0 ] },
                { "make": "Toyota", "model": "Tercel", "year": 1999,
                  "tire_pressure": [ 29.8, 30.0 ] } )"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::comma_delimited).get(stream);
if (error) { /* handle error */ }
std::vector<Car> cars;
for (auto doc : stream) {
  Car c;
  if ((error = doc.get<Car>().get(c))) { /* handle error */ }
  cars.push_back(c);
}
```

### A single JSON array as a stream of documents

When the input is a single JSON array, you can stream its elements one at a
time without materializing the entire array as a `std::vector` upfront:

```cpp
auto json = R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
                    "tire_pressure": [ 40.1, 39.9 ] },
                  { "make": "Kia",    "model": "Soul",   "year": 2012,
                    "tire_pressure": [ 30.1, 31.0 ] },
                  { "make": "Toyota", "model": "Tercel", "year": 1999,
                    "tire_pressure": [ 29.8, 30.0 ] } ] )"_padded;
ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(json, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::comma_delimited_array).get(stream);
if (error) { /* handle error */ }
std::vector<Car> cars;
for (auto doc : stream) {
  Car c;
  if ((error = doc.get<Car>().get(c))) { /* handle error */ }
  cars.push_back(c);
}
```

### JSON Text Sequences (RFC 7464)

```cpp
// Build input with RS (0x1E) and LF (0x0A) delimiters
std::string input_str;
auto append = [&](std::string_view doc) {
  input_str += '\x1e'; input_str += doc; input_str += '\x0a';
};
append(R"({ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9 ] })");
append(R"({ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0 ] })");
append(R"({ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0 ] })");
simdjson::padded_string input(input_str);

ondemand::parser parser;
ondemand::document_stream stream;
auto error = parser.iterate_many(input, ondemand::DEFAULT_BATCH_SIZE,
                                 simdjson::stream_format::json_sequence).get(stream);
if (error) { /* handle error */ }
std::vector<Car> cars;
for (auto doc : stream) {
  Car c;
  if ((error = doc.get<Car>().get(c))) { /* handle error */ }
  cars.push_back(c);
}
```

In every case, the user-defined type (`Car` here) does not need a hand-written
`tag_invoke` overload: the library generates the deserialization code from the
type's public data members at compile time.


**Performance tip**: You will get better performance if you order the attributes (make, model)
in the order they appear in the JSON document.