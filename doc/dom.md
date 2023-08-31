The Document-Object-Model (DOM) front-end
==========

An overview of what you need to know to use simdjson, with examples.

* [DOM vs On Demand](#dom-vs-on-demand)
* [The Basics: Loading and Parsing JSON Documents](#the-basics-loading-and-parsing-json-documents-using-the-dom-front-end)
* [Using the Parsed JSON](#using-the-parsed-json)
* [C++17 Support](#c17-support)
* [JSON Pointer](#json-pointer)
* [Error Handling](#error-handling)
  * [Error Handling Example](#error-handling-example)
  * [Exceptions](#exceptions)
* [Tree Walking and JSON Element Types](#tree-walking-and-json-element-types)
* [Reusing the parser for maximum efficiency](#reusing-the-parser-for-maximum-efficiency)
* [Server Loops: Long-Running Processes and Memory Capacity](#server-loops-long-running-processes-and-memory-capacity)
* [Best Use of the DOM API](#best-use-of-the-dom-api)
* [Padding and Temporary Copies](#padding-and-temporary-copies)
* [Performance Tips](#performance-tips)

DOM vs On Demand
----------------------------------------------

The simdjson library offers two distinct approaches on how to access a JSON document. We support
a conventional Document-Object-Model (DOM) front-end. In such a scenario, the JSON document is
entirely parsed, validated and materialized in memory as the first step. The programmer may
then access the parsed data using this in-memory model.

The Basics: Loading and Parsing JSON Documents using the DOM front-end
----------------------------------------------

The simdjson library offers a simple DOM tree API, which you can access by creating a
`dom::parser` and calling the `load()` method:

```c++
dom::parser parser;
dom::element doc = parser.load(filename); // load and parse a file
```

Or by creating a padded string (for efficiency reasons, simdjson requires a string with
SIMDJSON_PADDING bytes at the end) and calling `parse()`:

```c++
dom::parser parser;
dom::element doc = parser.parse("[1,2,3]"_padded); // parse a string, the _padded suffix creates a simdjson::padded_string instance
```

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

The parsed document resulting from the `parser.load` and `parser.parse` calls depends on the `parser` instance. Thus the `parser` instance must remain in scope. Furthermore, you must have at most one parsed document in play per `parser` instance.
You cannot copy a `parser` instance, you may only move it.

If you need to keep a document around long term, you can keep or move the parser instance. Note that moving a parser instance, or keeping one in a movable data structure like vector or map, can cause any outstanding `element`, `object` or `array` instances to be invalidated. The `element`, `object` or `array` instances are mere thin wrappers akin to an `std::vector<int>::iterator`: they are invalid when default constructed, they must be tied to a valid document instance. If you need to store a parser in a movable data structure, you should use a `std::unique_ptr` to avoid this invalidation(e.g., `std::unique_ptr<dom::parser> parser(new dom::parser{})`).

During the`load` or `parse` calls, neither the input file nor the input string are ever modified. After calling `load` or `parse`, the source (either a file or a string) can be safely discarded. All of the JSON data is stored in the `parser` instance.  The parsed document is also immutable in simdjson: you do not modify it by accessing it.

For best performance, a `parser` instance should be reused over several files: otherwise you will needlessly reallocate memory, an expensive process. It is also possible to avoid entirely memory allocations during parsing when using simdjson. [See our performance notes for details](performance.md).

If you need a lower-level interface, you may call the function `parser.parse(const char * p, size_t l)` on a pointer `p` while specifying the
length of your input `l` in bytes. To see how to get the very best performance from a low-level approach, you way want to read our [performance notes](https://github.com/simdjson/simdjson/blob/master/doc/performance.md#padding-and-temporary-copies) on this topic (see the Padding and Temporary Copies section).


Using the Parsed JSON
---------------------

Once you have an element, you can navigate it with idiomatic C++ iterators, operators and casts.

* **Extracting Values (with exceptions):** You can cast a JSON element to a native type: `double(element)` or
  `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  dom::object and dom::array. An exception (`simdjson::simdjson_error`) is thrown if the cast is not possible.
* **Extracting Values (without exceptions):** You can use a variant usage of `get()` with error codes to avoid exceptions. You first declare the variable of the appropriate type (`double`, `uint64_t`, `int64_t`, `bool`, `std::string_view`,
  `dom::object` and `dom::array`) and pass it by reference to `get()` which gives you back an error code: e.g.,
  ```c++
  simdjson::error_code error;
  simdjson::padded_string numberstring = "1.2"_padded; // our JSON input ("1.2")
  simdjson::dom::parser parser;
  double value; // variable where we store the value to be parsed
  error = parser.parse(numberstring).get(value);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << "I parsed " << value << " from " << numberstring.data() << std::endl;
  ```
  The strings contain unescaped valid UTF-8 strings: no unmatched surrogate is allowed.
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`.
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. If you
  know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`
* **Object Iteration:** You can iterate through an object's fields, too: `for (auto [key, value] : object)`
* **Array Index:** To get at an array value by index, use the at() method: `array.at(0)` gets the
  first element. The at() method has linear-time complexity so it should not be used to iterate over the values of an array.
  > Note that array[0] does not compile, because implementing [] gives the impression indexing is a
  > O(1) operation, which it is not presently in simdjson. Instead, you should iterate over the elements
  > using a for-loop, as in our examples.
* **Array and Object size** Given an array or an object, you can get its size (number of elements or keys)
  with the `size()` method.
* **Checking an Element Type:** You can check an element's type with `element.type()`. It
  returns an `element_type` with values such as `simdjson::dom::element_type::ARRAY`, `simdjson::dom::element_type::OBJECT`, `simdjson::dom::element_type::INT64`,  `simdjson::dom::element_type::UINT64`,`simdjson::dom::element_type::DOUBLE`, `simdjson::dom::element_type::STRING`, `simdjson::dom::element_type::BOOL` or, `simdjson::dom::element_type::NULL_VALUE`.
* **Output to streams and strings:** Given a document or an element (or node) out of a JSON document, you can output a minified string version using the C++ stream idiom (`out << element`). You can also request the construction of a minified string version (`simdjson::minify(element)`) or a prettified string version (`simdjson::prettify(element)`). Numbers are serialized as 64-bit floating-point numbers (`double`).

### Examples

The following code illustrates all of the above:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;

// Iterating through an array of objects
for (dom::object car : parser.parse(cars_json)) {
  // Accessing a field by name
  cout << "Make/Model: " << car["make"] << "/" << car["model"] << endl;

  // Casting a JSON element to an integer
  uint64_t year = car["year"];
  cout << "- This car is " << 2020 - year << "years old." << endl;

  // Iterating through an array of floats
  double total_tire_pressure = 0;
  for (double tire_pressure : car["tire_pressure"]) {
    total_tire_pressure += tire_pressure;
  }
  cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

  // Writing out all the information about the car
  for (auto field : car) {
    cout << "- " << field.key << ": " << field.value << endl;
  }
}
```

Here is a different example illustrating the same ideas:

```C++
auto abstract_json = R"( [
    {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
    {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
  ] )"_padded;
dom::parser parser;

// Parse and iterate through an array of objects
for (dom::object obj : parser.parse(abstract_json)) {
    for(const auto key_value : obj) {
      cout << "key: " << key_value.key << " : ";
      dom::object innerobj = key_value.value;
      cout << "a: " << double(innerobj["a"]) << ", ";
      cout << "b: " << double(innerobj["b"]) << ", ";
      cout << "c: " << int64_t(innerobj["c"]) << endl;
    }
}
```

And another one:


```C++
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  double v = parser.parse(abstract_json)["str"]["123"]["abc"];
  cout << "number: " << v << endl;
```


C++17 Support
-------------

While the simdjson library can be used in any project using C++ 11 and above, field iteration has special support C++ 17's destructuring syntax. For example:

```c++
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
dom::parser parser;
dom::object object; // invalid until the get() succeeds
auto error = parser.parse(json).get(object);
if (error) { cerr << error << endl; return; }
for (auto [key, value] : object) {
  cout << key << " = " << value << endl;
}
```

For comparison, here is the C++ 11 version of the same code:

```c++
// C++ 11 version for comparison
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
dom::parser parser;
dom::object object; // invalid until the get() succeeds
auto error = parser.parse(json).get(object);
if (error) { cerr << error << endl; return; }
for (dom::key_value_pair field : object) {
  cout << field.key << " = " << field.value << endl;
}
```


JSON Pointer
------------

The simdjson library also supports [JSON pointer](https://tools.ietf.org/html/rfc6901) through the
`at_pointer()` method, letting you reach further down into the document in a single call:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::element cars = parser.parse(cars_json);
cout << cars.at_pointer("/0/tire_pressure/1") << endl; // Prints 39.9
```

A JSON Path is a sequence of segments each starting with the '/' character. Within arrays, an integer
index allows you to select the indexed node. Within objects, the string value of the key allows you to
select the value. If your keys contain the characters '/' or '~', they must be escaped as '~1' and
'~0' respectively. An empty JSON Path refers to the whole document.

We also extend the JSON Pointer support to include *relative* paths.
You can apply a JSON path to any node and the path gets interpreted relatively, as if the current node were a whole JSON document.

Consider the following example:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::element cars = parser.parse(cars_json);
cout << cars.at_pointer("/0/tire_pressure/1") << endl; // Prints 39.9
for (dom::element car_element : cars) {
    dom::object car; // invalid until the get() succeeds
    simdjson::error_code error;
    if ((error = car_element.get(car))) { std::cerr << error << std::endl; return; }
    double x = car.at_pointer("/tire_pressure/1");
    cout << x << endl; // Prints 39.9, 31 and 30
}
```



Error Handling
--------------

All simdjson APIs that can fail return `simdjson_result<T>`, which is a &lt;value, error_code&gt;
pair. You can retrieve the value with .get(), like so:

```c++
dom::element doc;
auto error = parser.parse(json).get(doc);
if (error) { cerr << error << endl; exit(1); }
```

When there is no error, the error code simdjson::SUCCESS is returned: it evaluates as false as a Boolean.
We have several error codes to indicate errors, they all evaluate to true as a Boolean: your software should not generally not depend on exact
error codes. We may change the error codes in future releases and the exact error codes could vary depending on your system.

When you use the code without exceptions, it is your responsibility to check for error before using the
result: if there is an error, the result value will not be valid and using it will caused undefined
behavior.

We can write a "quick start" example where we attempt to parse the following JSON file and access some data, without triggering exceptions:
```JavaScript
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

Our program loads the file, selects value corresponding to key "search_metadata" which expected to be an object, and then
it selects the key "count" within that object.

```C++
#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets; // invalid until the get() succeeds
  auto error = parser.load("twitter.json").get(tweets);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }

  simdjson::dom::element res; // invalid until the get() succeeds
  if ((error = tweets["search_metadata"]["count"].get(res))) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
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
  simdjson::dom::parser parser;
  simdjson::dom::element tweets; // invalid until the get() succeeds
  auto error = parser.load("twitter.json").get(tweets);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  uint64_t identifier;
  error = tweets["statuses"].at(0)["id"].get(identifier);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << identifier << std::endl;
  return EXIT_SUCCESS;
}
```

The `at()` method has linear-time complexity: it should not be used to iterate
over the content of an array.

### Error Handling Example

This is how the example in "Using the Parsed JSON" could be written using only error code checking:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::array cars; // invalid until the get() succeeds
auto error = parser.parse(cars_json).get(cars);
if (error) { cerr << error << endl; exit(1); }

// Iterating through an array of objects
for (dom::element car_element : cars) {
    dom::object car;
    if ((error = car_element.get(car))) { cerr << error << endl; exit(1); }

    // Accessing a field by name
    std::string_view make, model;
    if ((error = car["make"].get(make))) { cerr << error << endl; exit(1); }
    if ((error = car["model"].get(model))) { cerr << error << endl; exit(1); }
    cout << "Make/Model: " << make << "/" << model << endl;

    // Casting a JSON element to an integer
    uint64_t year;
    if ((error = car["year"].get(year))) { cerr << error << endl; exit(1); }
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    dom::array tire_pressure_array;
    if ((error = car["tire_pressure"].get(tire_pressure_array))) { cerr << error << endl; exit(1); }
    for (dom::element tire_pressure_element : tire_pressure_array) {
        double tire_pressure;
        if ((error = tire_pressure_element.get(tire_pressure))) { cerr << error << endl; exit(1); }
        total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

    // Writing out all the information about the car
    for (auto field : car) {
        cout << "- " << field.key << ": " << field.value << endl;
    }
}
```

Here is another example:

```C++
auto abstract_json = R"( [
    {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
    {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
  ] )"_padded;
dom::parser parser;
dom::array array; // invalid until after the next line
auto error = parser.parse(abstract_json).get(array);
if (error) { cerr << error << endl; exit(1); }
// Iterate through an array of objects
for (dom::element elem : array) {
    dom::object obj;
    if ((error = elem.get(obj))) { cerr << error << endl; exit(1); }
    for (auto & key_value : obj) {
        cout << "key: " << key_value.key << " : ";
        dom::object innerobj;
        if ((error = key_value.value.get(innerobj))) { cerr << error << endl; exit(1); }

        double va, vb;
        if ((error = innerobj["a"].get(va))) { cerr << error << endl; exit(1); }
        cout << "a: " << va << ", ";
        if ((error = innerobj["b"].get(vc))) { cerr << error << endl; exit(1); }
        cout << "b: " << vb << ", ";

        int64_t vc;
        if ((error = innerobj["c"].get(vc))) { cerr << error << endl; exit(1); }
        cout << "c: " << vc << endl;
    }
}
```

And another one:

```C++
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  double v;
  auto error = parser.parse(abstract_json)["str"]["123"]["abc"].get(v);
  if (error) { cerr << error << endl; exit(1); }
  cout << "number: " << v << endl;
```

Notice how we can string several operations (`parser.parse(abstract_json)["str"]["123"]["abc"].get(v)`) and only check for the error once, a strategy we call  *error chaining*.

The next two functions will take as input a JSON document containing an array with a single element, either a string or a number. They return true upon success.

```C++
simdjson::dom::parser parser{};

bool parse_double(const char *j, double &d) {
  auto error = parser.parse(j, std::strlen(j))
        .at(0)
        .get(d, error);
  if (error) { return false; }
  return true;
}

bool parse_string(const char *j, std::string &s) {
  std::string_view answer;
  auto error = parser.parse(j, strlen(j))
        .at(0)
        .get(answer, error);
  if (error) { return false; }
  s.assign(answer.data(), answer.size());
  return true;
}
```

To ensure you don't write any code that uses exceptions, compile with `SIMDJSON_EXCEPTIONS=OFF`. For example, if including the project via cmake:

```cmake
target_compile_definitions(simdjson PUBLIC SIMDJSON_EXCEPTIONS=OFF)
```

### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```c++
dom::element doc = parser.parse(json); // Throws an exception if there was an error!
```

When used this way, a `simdjson_error` exception will be thrown if an error occurs, preventing the
program from continuing if there was an error.


If one is willing to trigger exceptions, it is possible to write simpler code:

```C++
#include <iostream>
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load("twitter.json");
  std::cout << "ID: " << tweets["statuses"].at(0)["id"] << std::endl;
  return EXIT_SUCCESS;
}
```


Tree Walking and JSON Element Types
-----------------------------------

Sometimes you don't necessarily have a document with a known type, and are trying to generically
inspect or walk over JSON elements. To do that, you can use iterators and the type() method. For
example, here's a quick and dirty recursive function that verbosely prints the JSON document as JSON
(* ignoring nuances like trailing commas and escaping strings, for brevity's sake):

```c++
void print_json(dom::element element) {
  switch (element.type()) {
    case dom::element_type::ARRAY:
      cout << "[";
      for (dom::element child : dom::array(element)) {
        print_json(child);
        cout << ",";
      }
      cout << "]";
      break;
    case dom::element_type::OBJECT:
      cout << "{";
      for (dom::key_value_pair field : dom::object(element)) {
        cout << "\"" << field.key << "\": ";
        print_json(field.value);
      }
      cout << "}";
      break;
    case dom::element_type::INT64:
      cout << int64_t(element) << endl;
      break;
    case dom::element_type::UINT64:
      cout << uint64_t(element) << endl;
      break;
    case dom::element_type::DOUBLE:
      cout << double(element) << endl;
      break;
    case dom::element_type::STRING:
      cout << std::string_view(element) << endl;
      break;
    case dom::element_type::BOOL:
      cout << bool(element) << endl;
      break;
    case dom::element_type::NULL_VALUE:
      cout << "null" << endl;
      break;
  }
}

void basics_treewalk_1() {
  dom::parser parser;
  print_json(parser.load("twitter.json"));
}
```



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

Performance Tips
---------------------

- For release builds, we recommend setting `NDEBUG` pre-processor directive when compiling the `simdjson` library. Importantly, using the optimization flags `-O2` or `-O3` under GCC and LLVM clang does not set the `NDEBUG` directrive, you must set it manually (e.g., `-DNDEBUG`).
- For long streams of JSON documents, consider [`iterate_many`](iterate_many.md) and [`parse_many`](parse_many.md) for better performance.
