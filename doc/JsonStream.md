# JsonStream
An interface providing features to work with files or streams containing multiple JSON documents. 
As fast and convenient as possible.
## Contents
- [Motivations](#Motivations)
- [Performance](#Performance)
- [How it works](#how-it-works)
- [Support](#Support)
- [API](#API)
- [Concurrency mode](#concurrency-mode)
- [Example](#Example)
- [Use cases](#use-cases)


## Motivations
The main motivation for this piece of software is to achieve maximum speed and offer a
better quality of life in parsing files containing multiple JSON documents.

The JavaScript Object Notation (JSON) [RFC7159](https://tools.ietf.org/html/rfc7159) is a very handy
serialization format.  However, when serializing a large sequence of
values as an array, or a possibly indeterminate-length or never-
ending sequence of values, JSON becomes difficult to work with.

Consider a sequence of one million values, each possibly one kilobyte
when encoded -- roughly one gigabyte.  It is often desirable to process such a dataset incrementally 
without having to first read all of it before beginning to produce results.
## Performance
Here is a chart comparing the speed of the different alternatives to parse a multiline JSON.
JsonStream provides a threaded and non-threaded implementation.  As the figure below shows, 
if you can, use threads, else it's still pretty fast!
[![Chart.png](/doc/Multiline_JSON_Parse_Competition.png)](/doc/Multiline_JSON_Parse_Competition.png)
## How it works
- **Context:** 
    The parsing in simdjson is divided into 2 stages.  First, in stage 1, we parse the document and find all 
    the structural indexes (`{`, `}`, `]`, `[`, `,`, `"`, ...) and validate UTF8.  Then, in stage 2, we go through the 
    document again and build the tape using structural indexes found during stage 1.  Although stage 1 finds the structural 
    indexes, it has no knowledge of the structure of the document nor does it know whether it parsed a valid document, 
    multiple documents, or even if the document is complete.

    Prior to JsonStream, most people who had to parse a multiline JSON file would proceed by reading the file
    line by line, using a utility function like `std::getline` or equivalent, and would then use the function 
    `simdjson::json_parse` on each of those lines.  From a performance point of view, this process is highly inefficient, 
    in that it requires a lot of unnecessary memory allocation and makes use of the `getline` function,
    which is fundamentally slow, slower than the act of parsing with simdjson [(more on this here)](https://lemire.me/blog/2019/06/18/how-fast-is-getline-in-c/).

    Unlike the popular parser RapidJson, our DOM does not require the buffer once the parsing job is completed, 
    the DOM and the buffer are completely independent. The drawback of this architecture is that we need to allocate
    some additional memory to store our ParsedJson data, for every document inside a given file.  Memory allocation can be
    slow and become a bottleneck, therefore, we want to minimize it as much as possible.
- **Design:**
    To achieve a minimum amount of allocations, we opted for a design where we create only one ParsedJson object and 
    therefore allocate its memory once, and then recycle it for every document in a given file.  But, knowing 
    that they often have largely varying size, we need to make sure that we allocate enough memory so that all the 
    documents can fit. This value is what we call the batch size.  As of right now, we need to manually specify a value
    for this batch size, it has to be at least as big as the biggest document in your file, but not too big so that it 
    submerges the cached memory. The bigger the batch size, the fewer we need to make allocations. We found that 1MB is 
    somewhat a sweet spot for now.
    
    We then proceed by running the stage 1 on all the data that we can fit into this batch size. Finally, every time the
    user calls `JsonStream::json_parse`, we run the stage 2 on this data, until we detect that we parsed a valid 
    document, keep a reference to this point and start at this point the next time `JsonStream::json_parse` is called. 
    We do so until we consumed all the documents in the current batch, then we load another batch, and repeat the process
    until we reach the end of the file. 
    
    But how can we make use of threads?  We found a pretty cool algorithm that allows us to quickly identify the 
    position of the last JSON document in a given batch. Knowing exactly where the end of the batch is, we no longer 
    need for stage 2 to finish in order to load a new batch. We already know where to start the next batch. Therefore,
    we can run stage 1 on the next batch concurrently while the main thread is going through stage 2. Now, running 
    stage 1 in a different thread can, in best cases, remove almost entirely it's cost and replaces it by the overhead
    of a thread, which is orders of magnitude cheaper. Ain't that awesome!

## Support
Since we want to offer flexibility and not restrict ourselves to a specific file
format, we support any file that contains any amount of valid JSON document, **separated by one
or more character that is considered whitespace** by the JSON spec. Anything that is
 not whitespace will be parsed as a JSON document and could lead to failure.

Whitespace Characters: 
- **Space**
- **Linefeed**
- **Carriage return**
- **Horizontal tab**
- **Nothing**




Some official formats **(non-exhaustive list)**:
- [Newline-Delimited JSON (NDJSON)](http://ndjson.org/)
- [JSON lines (JSONL)](http://jsonlines.org/)
- [Record separator-delimited JSON (RFC 7464)](https://tools.ietf.org/html/rfc7464) <- Not supported by JsonStream!
- [More on Wikipedia...](https://en.wikipedia.org/wiki/JSON_streaming)

## API
**Constructor**  
```cpp  
// Create a JsonStream object that can be used to parse sequentially the valid JSON 
// documents found in the buffer "buf".  
//  
// The batch_size must be at least as large as the biggest document in the file, but not 
// too large to submerge the cached memory.  We found that 1MB is somewhat a sweet spot for 
// now.  
//  
// The user is expected to call the following json_parse method to parse the next valid 
// JSON document found in the buffer. This method can and is expected to be called in a 
// loop.  
//  
// Various methods are offered to keep track of the status, like get_current_buffer_loc, 
// get_n_parsed_docs, 
// get_n_bytes_parsed, etc.
 JsonStream(const char *buf, size_t len, size_t batch_size = 1000000);
 JsonStream(const std::string &s, size_t batch_size = 1000000);
 ```  
 **Methods**

 - **Parsing**
	```cpp
	// Parse the next document found in the buffer previously given to JsonStream.  
	//  
	// You do NOT need to pre-allocate ParsedJson.  This function takes care of pre
	// allocating a capacity defined by the 
	// batch_size defined when creating the 
	// JsonStream object.  
	//  
	// The function returns simdjson::SUCCESS_AND_HAS_MORE (an integer = 1) in case of 
	// success and indicates that the buffer 
	// still contains more data to be parsed, 
	// meaning this function can be called again to return the next JSON document 
	// after this one.  
	//  
	// The function returns simdjson::SUCCESS (as integer = 0) in case of success and 
	// indicates that the buffer has 
	// successfully been parsed to the end. Every 
	// document it contained has been parsed without error.  
	//  
	// The function returns an error code from simdjson/simdjson.h in case of failure 
	// such as simdjson::CAPACITY, 
	// simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so 
	// forth; the simdjson::error_message function converts these error 
	// codes into a 
	// string).  
	//  
	// You can also check validity by calling pj.is_valid(). The same ParsedJson can 
	// and should be reused for the other 
	// documents in the buffer.
	 int json_parse(ParsedJson &pj)
	 ```  
 - **Buffer**
	 ```cpp
	// Sets a new buffer for this JsonStream.  Will also reinitialize all the 
	// variables, which acts as a reset.  A new JsonStream without initializing 
	// again.
	void set_new_buffer(const char *buf, size_t len);  
	void set_new_buffer(const std::string &s)
	```

 - **Utility**
	```cpp
	// Returns the location (index) of where the next document should be in the 
	// buffer.  Can be used for debugging, it tells the user the position of the end of 
	// the last valid JSON document parsed
	 size_t get_current_buffer_loc() const;  
	  
	// Returns the total amount of complete documents parsed by the JsonStream,  
	// in the current buffer, at the given time.
	 size_t get_n_parsed_docs() const;  
	  
	// Returns the total amount of data (in bytes) parsed by the JsonStream,  
	// in the current buffer, at the given time.
	 size_t get_n_bytes_parsed() const;
	```
## Concurrency mode
To use concurrency mode, tell the compiler to enable threading mode: 
- **GCC and Clang** : Compile with `-pthread` flag.
- **Visual C++ (MSVC)**: Usually enabled by default. If not, add the flag `/MT` or `/MD` to the compiler.

**Note:** The JsonStream API remains the same whether you are using the threaded version or not.
## Example

Here is a simple exemple, using single header simdjson:
```cpp
#include "simdjson.h"
#include "simdjson.cpp"

int parse_file(const char *filename) {
    simdjson::padded_string p = simdjson::get_corpus(filename);
    simdjson::ParsedJson pj;
    simdjson::JsonStream js{p};
    int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
    
    while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
            parse_res = js.json_parse(pj);

            //Do something with pj...
        }
}
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
