iterate_many
==========

When serializing large databases, it is often better to write out many independent JSON
documents, instead of one large monolithic document containing many records. The simdjson
library provides high-speed access to files or streams containing multiple small JSON documents separated by ASCII white-space characters. Given an input such as
```JSON
{"text":"a"}
{"text":"b"}
{"text":"c"}
...
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
- [Use cases](#use-cases)
- [Tracking your position](#tracking-your-position)
- [Incomplete streams](#incomplete-streams)

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
SIMDJSON_THREADS_ENABLED is set. Otherwise the library runs in  single-thread mode.

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
- [Newline-Delimited JSON (NDJSON)](http://ndjson.org/)
- [JSON lines (JSONL)](http://jsonlines.org/)
- [Record separator-delimited JSON (RFC 7464)](https://tools.ietf.org/html/rfc7464) <- Not supported by JsonStream!
- [More on Wikipedia...](https://en.wikipedia.org/wiki/JSON_streaming)

API
---

Example:

```c++
auto json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )"_padded;
ondemand::parser parser;
ondemand::document_stream docs = parser.iterate_many(json);
for (auto doc : docs) {
  std::cout << doc["foo"] << std::endl;
}
// Prints 1 2 3
```

See [basics.md](basics.md#newline-delimited-json-ndjson-and-json-lines) for an overview of the API.

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


```C++
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

```C++
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

We also support comma-separated documents, but with some performance limitations. The `iterate_many` function  takes in an option to allow parsing of comma separated documents (which defaults on false). In this mode, the entire buffer is processed in one batch. Therefore, the total size of the document should not exceed the maximal capacity of the parser (4 GB). This mode also effectively disallow multithreading. It is therefore mostly suitable for not "very large" inputs. In this mode, the batch_size parameter
is effectively ignored, as it is set to at least the document size.

Example:

```C++
    auto json = R"( 1, 2, 3, 4, "a", "b", "c", {"hello": "world"} , [1, 2, 3])"_padded;
    ondemand::parser parser;
    ondemand::document_stream doc_stream;
    // We pass '32' as the batch size, but it is a bogus parameter because, since
    // we pass 'true' to the allow_comma parameter, the batch size will be set to at least
    // the document size.
    auto error = parser.iterate_many(json, 32, true).get(doc_stream);
    if (error) { std::cerr << error << std::endl; return; }
    for (auto doc : doc_stream) {
        std::cout << doc.type() << std::endl;
    }
 ```

 This will print:

```
number
number
number
number
string
string
string
object
array
```
