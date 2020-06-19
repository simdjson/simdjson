The Basics
==========

An overview of what you need to know to use simdjson, with examples.

* [Requirements](#requirements)
* [Including simdjson](#including-simdjson)
* [The Basics: Loading and Parsing JSON Documents](#the-basics-loading-and-parsing-json-documents)
* [Using the Parsed JSON](#using-the-parsed-json)
* [JSON Pointer](#json-pointer)
* [Error Handling](#error-handling)
  * [Error Handling Example](#error-handling-example)
  * [Exceptions](#exceptions)
* [Tree Walking and JSON Element Types](#tree-walking-and-json-element-types)
* [Newline-Delimited JSON (ndjson) and JSON lines](#newline-delimited-json-ndjson-and-json-lines)
* [Thread Safety](#thread-safety)


Requirements
------------------

- A recent compiler (LLVM clang6 or better, GNU GCC 7 or better) on a 64-bit (ARM or x64 Intel/AMD) POSIX systems such as macOS, freeBSD or Linux. We require that the compiler supports the C++11 standard or better.
- Visual Studio 2017 or better under 64-bit Windows. Users should target a 64-bit build (x64) instead of a 32-bit build (x86). We support the LLVM clang compiler under Visual Studio (clangcl) as well as as the regular Visual Studio compiler.

Including simdjson
------------------

To include simdjson, copy [simdjson.h](/singleheader/simdjson.h) and [simdjson.cpp](/singleheader/simdjson.cpp)
into your project. Then include it in your project with:

```c++
#include "simdjson.h"
using namespace simdjson; // optional
```

You can compile with:

```
c++ myproject.cpp simdjson.cpp
```

Note:
- Users on macOS and other platforms were default compilers do not provide C++11 compliant by default should request it with the appropriate flag (e.g., `c++ myproject.cpp simdjson.cpp`).
- Visual Studio users should compile with the `_CRT_SECURE_NO_WARNINGS` flag to avoid warnings with respect to our use of standard C functions such as `fopen`.


The Basics: Loading and Parsing JSON Documents
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
dom::element doc = parser.parse("[1,2,3]"_padded); // parse a string
```

The parsed document resulting from the `parser.load` and `parser.parse` calls depends on the `parser` instance. Thus the `parser` instance must remain in scope. Furthermore, you must have at most one parsed document in play per `parser` instance.

During the`load` or `parse` calls, neither the input file nor the input string are ever modified. After calling `load` or `parse`, the source (either a file or a string) can be safely discarded. All of the JSON data is stored in the `parser` instance.  The parsed document is also immutable in simdjson: you do not modify it by accessing it.

For best performance, a `parser` instance should be reused over several files: otherwise you will needlessly reallocate memory, an expensive process. It is also possible to avoid entirely memory allocations during parsing when using simdjson. [See our performance notes for details](https://github.com/simdjson/simdjson/blob/master/doc/performance.md).


Using the Parsed JSON
---------------------

Once you have an element, you can navigate it with idiomatic C++ iterators, operators and casts.

* **Extracting Values:** You can cast a JSON element to a native type: `double(element)` or
  `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  dom::object and dom::array. An exception is thrown if the cast is not possible. You can also use is<*typename*>() to test if it is a
  given type, or use the `type()` method: e.g., `element.type() == dom::element_type::DOUBLE`. Instead of casting, you can use get<*typename*>() to get the value: casts and get<*typename*>() can be used interchangeably. You can use a variant usage of get<*typename*>() with error codes to avoid exceptions: e.g.,  
  ```c++
  simdjson::error_code error;
  simdjson::padded_string numberstring = "1.2"_padded; // our JSON input ("1.2")
  simdjson::dom::parser parser;
  double value; // variable where we store the value to be parsed
  parser.parse(numberstring).get(value,error);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << "I parsed " << value << " from " << numberstring.data() << std::endl;
  ```
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`.
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. If you
  know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`
* **Object Iteration:** You can iterate through an object's fields, too: `for (auto [key, value] : object)`
* **Array Index:** To get at an array value by index, use the at() method: `array.at(0)` gets the
  first element.
  > Note that array[0] does not compile, because implementing [] gives the impression indexing is a
  > O(1) operation, which it is not presently in simdjson. Instead, you should iterate over the elements
  > using a for-loop, as in our examples.
* **Array and Object size** Given an array or an object, you can get its size (number of elements or keys)
  with the `size()` method.
* **Checking an Element Type:** You can check an element's type with `element.type()`. It
  returns an `element_type`.


Here are some examples of all of the above:

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
    for(const auto& key_value : obj) {
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
  double v = parser.parse(abstract_json)["str"]["123"]["abc"].get<double>();
  cout << "number: " << v << endl;
```

Minifying JSON strings without parsing
----------------------

In some cases, you may have valid JSON strings that you do not wish to parse but that you wish to minify. That is, you wish to remove all unnecessary spaces. We have a fast function for this purpose (`minify`). This function does not validate your content, and it does not parse it. Instead, it assumes that your string is valid UTF-8. It is much faster than parsing the string and re-serializing it in minified form. Usage is relatively simple. You must pass an input pointer with a length parameter, as well as an output pointer and an output length parameter (by reference). The output length parameter is not read, but written to. The output pointer should point to a valid memory region that is slightly overallocated (by `simdjson::SIMDJSON_PADDING`) compared to the original string length. The input pointer and input length are read, but not written to.

```C++
  // Starts with a valid JSON document as a string.
  // It does not have to be null-terminated.
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = strlen(some_string);
  // Create a buffer to receive the minified string. Make sure that there is enough room,
  // including some padding (simdjson::SIMDJSON_PADDING).
  std::unique_ptr<char[]> buffer{new(std::nothrow) char[length + simdjson::SIMDJSON_PADDING]};
  size_t new_length{}; // It will receive the minified length.
  auto error = simdjson::minify(some_string, length, buffer.get(), new_length);
  // The buffer variable now has "[1,2,3,4]" and new_length has value 9.
```

Though it does not validate the JSON input, it will detect when the document ends with an unterminated string. E.g., it would refuse to minify the string `"this string is not terminated` because of the missing final quote.

C++17 Support
-------------

While the simdjson library can be used in any project using C++ 11 and above, it has special support
for C++ 17. The APIs for field iteration and error handling in particular are designed to work
nicely with C++17's destructuring syntax. For example:

```c++
dom::parser parser;
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
auto [object, error] = parser.parse(json).get<dom::object>();
if (error) { cerr << error << endl; return; }
for (auto [key, value] : object) {
  cout << key << " = " << value << endl;
}
```

For comparison, here is the C++ 11 version of the same code:

```c++
// C++ 11 version for comparison
dom::parser parser;
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
simdjson::error_code error;
dom::object object;
parser.parse(json).get(object, error);
if (!error) { cerr << error << endl; return; }
for (dom::key_value_pair field : object) {
  cout << field.key << " = " << field.value << endl;
}
```

JSON Pointer
------------

The simdjson library also supports [JSON pointer](https://tools.ietf.org/html/rfc6901) through the
at() method, letting you reach further down into the document in a single call:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::element cars = parser.parse(cars_json);
cout << cars.at("0/tire_pressure/1") << endl; // Prints 39.9
```

Error Handling
--------------

All simdjson APIs that can fail return `simdjson_result<T>`, which is a &lt;value, error_code&gt;
pair. The error codes and values can be accessed directly, reading the error like so:

```c++
auto [doc, error] = parser.parse(json); // doc is a dom::element
if (error) { cerr << error << endl; exit(1); }
// Use document here now that we've checked for the error
```

When you use the code this way, it is your responsibility to check for error before using the
result: if there is an error, the result value will not be valid and using it will caused undefined
behavior.

> Note: because of the way `auto [x, y]` works in C++, you have to define new variables each time you
> use it. If your project treats aliased, this means you can't use the same names in `auto [x, error]`
> without triggering warnings or error (and particularly can't use the word "error" every time). To
> circumvent this, you can use this instead:
>
> ```c++
> dom::element doc;
> simdjson::error_code error;
> parser.parse(json).tie(doc, error); // <-- Assigns to doc and error just like "auto [doc, error]"
> ```


We can write a "quick start" example where we attempt to parse a file and access some data, without triggering exceptions:

```C++
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;
  simdjson::dom::element tweets;
  simdjson::error_code error;
  parser.load("twitter.json").tie(tweets,error);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  simdjson::dom::element res;

  tweets["search_metadata"]["count"].tie(res,error);
  if(error) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
}
```

### Error Handling Example

This is how the example in "Using the Parsed JSON" could be written using only error code checking:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::array cars;
simdjson::error_code error;
if (!parser.parse(cars_json).get(cars, error)) { cerr << error << endl; exit(1); }

// Iterating through an array of objects
for (dom::element car_element : cars) {
    dom::object car;
    if (!car_element.get(car, error)) { cerr << error << endl; exit(1); }

    // Accessing a field by name
    std::string_view make, model;
    if (!car["make"].get(make, error)) { cerr << error << endl; exit(1); }
    if (!car["model"].get(model, error)) { cerr << error << endl; exit(1); }
    cout << "Make/Model: " << make << "/" << model << endl;

    // Casting a JSON element to an integer
    uint64_t year;
    if (!car["year"].get(year, error)) { cerr << error << endl; exit(1); }
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    dom::array tire_pressure_array;
    if (!car["tire_pressure"].get(tire_pressure_array, error)) { cerr << error << endl; exit(1); }
    for (dom::element tire_pressure_element : tire_pressure_array) {
        double tire_pressure;
        if (!tire_pressure_element.get(tire_pressure, error)) { cerr << error << endl; exit(1); }
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
dom::array rootarray;
simdjson::error_code error;
parser.parse(abstract_json).get(rootarray, error);
if (error) { cerr << error << endl; exit(1); }
// Iterate through an array of objects
for (dom::element elem : rootarray) {
    dom::object obj;
    if (!elem.get(obj, error)) { cerr << error << endl; exit(1); }
    for (auto & key_value : obj) {
        cout << "key: " << key_value.key << " : ";
        dom::object innerobj;
        if (!key_value.value.get(innerobj, error)) { cerr << error << endl; exit(1); }

        double va, vb;
        if (!innerobj["a"].get(va, error)) { cerr << error << endl; exit(1); }
        cout << "a: " << va << ", ";
        if (!innerobj["b"].get(vc, error)) { cerr << error << endl; exit(1); }
        cout << "b: " << vb << ", ";

        int64_t vc;
        if (!innerobj["c"].get(vc, error)) { cerr << error << endl; exit(1); }
        cout << "c: " << vc << endl;
    }
}
```

And another one:

```C++
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  simdjson::error_code error;
  double v;
  parser.parse(abstract_json)["str"]["123"]["abc"].get(v, error);
  if (error) { cerr << error << endl; exit(1); }
  cout << "number: " << v << endl;
```

Notice how we can string several operation (`parser.parse(abstract_json)["str"]["123"]["abc"].get<double>()`) and only check for the error once, a strategy we call  *error chaining*.


The next two functions will take as input a JSON document containing an array with a single element, either a string or a number. They return true upon success.

```C++
simdjson::dom::parser parser{};

bool parse_double(const char *j, double &d) {
  simdjson::error_code error;
  parser.parse(j, std::strlen(j))
        .at(0)
        .get(d, error);
  if (error) { return false; }
  return true;
}

bool parse_string(const char *j, std::string &s) {
  simdjson::error_code error;
  std::string_view answer;
  parser.parse(j,strlen(j))
        .at(0)
        .get(answer, error);
  if (error) { return false; }
  s.assign(answer.data(), answer.size());
  return true;
}
```

### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```c++
dom::element doc = parser.parse(json); // Throws an exception if there was an error!
```

When used this way, a `simdjson_error` exception will be thrown if an error occurs, preventing the
program from continuing if there was an error.

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

Newline-Delimited JSON (ndjson) and JSON lines
----------------------------------------------

The simdjson library also support multithreaded JSON streaming through a large file containing many
smaller JSON documents in either [ndjson](http://ndjson.org) or [JSON lines](http://jsonlines.org)
format. If your JSON documents all contain arrays or objects, we even support direct file
concatenation without whitespace. The concatenated file has no size restrictions (including larger
than 4GB), though each individual document must be no larger than 4 GB.

Here is a simple example, given "x.json" with this content:

```json
{ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 }
```

```c++
dom::parser parser;
for (dom::element doc : parser.load_many(filename)) {
  cout << doc["foo"] << endl;
}
// Prints 1 2 3
```

In-memory ndjson strings can be parsed as well, with `parser.parse_many(string)`.

Both `load_many` and `parse_many` take an optional parameter `size_t batch_size` which defines the window processing size. It is set by default to a large value (`1000000` corresponding to 1 MB). None of your JSON documents should exceed this window size, or else you will get  the error `simdjson::CAPACITY`. You cannot set this window size larger than 4 GB: you will get  the error `simdjson::CAPACITY`. The smaller the window size is, the less memory the function will use. Setting the window size too small (e.g., less than 100 kB) may also impact performance negatively. Leaving it to 1 MB is expected to be a good choice, unless you have some larger documents.

See [parse_many.md](parse_many.md) for detailed information and design.

Thread Safety
-------------

We built simdjson with thread safety in mind.

The simdjson library is single-threaded except for  [`parse_many`](https://github.com/simdjson/simdjson/blob/master/doc/parse_many.md) which may use secondary threads under its control when the library is compiled with thread support.


We recommend using one `dom::parser` object per thread in which case the library is thread-safe.
It is unsafe to reuse a `dom::parser` object between different threads.
The parsed results (`dom::document`, `dom::element`, `array`, `object`) depend on the `dom::parser`, etc. therefore it is also potentially unsafe to use the result of the parsing between different threads.

The CPU detection, which runs the first time parsing is attempted and switches to the fastest
parser for your CPU, is transparent and thread-safe.

Backwards Compatibility
-----------------------

The only header file supported by simdjson is `simdjson.h`. Older versions of simdjson published a
number of other include files such as `document.h` or `ParsedJson.h` alongside `simdjson.h`; these headers
may be moved or removed in future versions.



Further Reading
-------------

* [Performance](doc/performance.md) shows some more advanced scenarios and how to tune for them.
* [Implementation Selection](doc/implementation-selection.md) describes runtime CPU detection and
  how you can work with it.
