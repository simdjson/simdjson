The Basics
==========

An overview of what you need to know to use simdjson, with examples.


Requirements
------------------

- A recent compiler (LLVM clang6 or better, GNU GCC 7 or better) on a 64-bit (ARM or x64 Intel/AMD) POSIX systems such as macOS, freeBSD or Linux. We require that the compiler supports the C++11 standard or better.
- Visual Studio 2017 or better under 64-bit Windows. Users should target a 64-bit build (x64) instead of a 32-bit build (x86). We support the LLVM clang compiler under Visual Studio (clangcl) as well as as the regular Visual Studio compiler.

Including simdjson
------------------

To include simdjson, copy the simdjson.h and simdjson.cpp files from the singleheader directory
into your project. Then include the header file in your project with:

```
#include "simdjson.h"
using namespace simdjson; // optional
```

You can compile with:

```
c++ myproject.cpp simdjson.cpp
```

Note:
- Users on macOS and other platforms were default compilers do not provide C++11 compliant by default should request it with the appropriate flag (e.g., `c++  -std=c++17 myproject.cpp simdjson.cpp`).
- Visual Studio users should compile with the `_CRT_SECURE_NO_WARNINGS` flag to avoid warnings with respect to our use of standard C functions such as `fopen`.

Using simdjson with package managers
------------------

You can install the simdjson library on your system or in your project using multiple package managers such as  MSYS2, the conan package manager, vcpkg, brew, the apt package manager (debian-based Linux systems), the FreeBSD package manager (FreeBSD), and so on. [Visit our wiki for more details](https://github.com/simdjson/simdjson/wiki/Installing-simdjson-with-a-package-manager).

Using simdjson as a CMake dependency
------------------

You can include the  simdjson as a CMake dependency by including the following lines in your `CMakeLists.txt`:

```
include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  v0.4.7
  GIT_SHALLOW TRUE)

set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")

FetchContent_MakeAvailable(simdjson)
```

You should replace `GIT_TAG  v0.5.0` by the version you need. If you omit `GIT_TAG  v0.5.0`, you will work from the main branch of simdjson: we recommend that if you are working on production code, 

Elsewhere in your project, you can  declare dependencies on simdjson with lines such as these:

```
add_executable(myprogram myprogram.cpp)
target_link_libraries(myprogram simdjson)
```

We recommend CMake version 3.15 or better.

See [our CMake demonstration](https://github.com/simdjson/cmake_demo_single_file). It works under Linux, FreeBSD, macOS and Windows (including Visual Studio).

The CMake build in simdjson can be taylored with a few variables. You can see the available variables and their default values by entering the `cmake -LA` command.



The Basics: Loading and Parsing JSON Documents
----------------------------------------------

The simdjson library offers a simple DOM tree API, which you can access by creating a
`dom::parser` and calling the `load()` method:

```
dom::parser parser;
dom::element doc = parser.load(filename); // load and parse a file
```

Or by creating a padded string (for efficiency reasons, simdjson requires a string with
SIMDJSON_PADDING bytes at the end) and calling `parse()`:

```
dom::parser parser;
dom::element doc = parser.parse("[1,2,3]"_padded); // parse a string, the _padded suffix creates a simdjson::padded_string instance
```

The parsed document resulting from the `parser.load` and `parser.parse` calls depends on the `parser` instance. Thus the `parser` instance must remain in scope. Furthermore, you must have at most one parsed document in play per `parser` instance.

During the`load` or `parse` calls, neither the input file nor the input string are ever modified. After calling `load` or `parse`, the source (either a file or a string) can be safely discarded. All of the JSON data is stored in the `parser` instance.  The parsed document is also immutable in simdjson: you do not modify it by accessing it.

For best performance, a `parser` instance should be reused over several files: otherwise you will needlessly reallocate memory, an expensive process. It is also possible to avoid entirely memory allocations during parsing when using simdjson. 


If you need a lower-level interface, you may call the function `parser.parse(const char * p, size_t l)` on a pointer `p` while specifying the
length of your input `l` in bytes. To see how to get the very best performance from a low-level approach, you way want to read our [performance notes](https://github.com/simdjson/simdjson/blob/master/doc/performance.md#padding-and-temporary-copies) on this topic (see the Padding and Temporary Copies section).

Using the Parsed JSON
---------------------

Once you have an element, you can navigate it with idiomatic C++ iterators, operators and casts.

* **Extracting Values (with exceptions):** You can cast a JSON element to a native type: `double(element)` or
  `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  dom::object and dom::array. An exception is thrown if the cast is not possible.
* **Extracting Values (without exceptions):** You can use a variant usage of `get()` with error codes to avoid exceptions. You first declare the variable of the appropriate type (`double`, `uint64_t`, `int64_t`, `bool`,
  `dom::object` and `dom::array`) and pass it by reference to `get()` which gives you back an error code: e.g.,
  ```
  simdjson::error_code error;
  simdjson::padded_string numberstring = "1.2"_padded; // our JSON input ("1.2")
  simdjson::dom::parser parser;
  double value; // variable where we store the value to be parsed
  error = parser.parse(numberstring).get(value);
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
  returns an `element_type` with values such as `simdjson::dom::element_type::ARRAY`, `simdjson::dom::element_type::OBJECT`, `simdjson::dom::element_type::INT64`,  `simdjson::dom::element_type::UINT64`,`simdjson::dom::element_type::DOUBLE`, `simdjson::dom::element_type::BOOL` or, `simdjson::dom::element_type::NULL_VALUE`.
* **Output to Streams and Strings:** Given a document or an element (or node) out of a JSON document, you can output a minified string version using the C++ stream idiom (`out << element`). You can also request the construction of a minified string version (`simdjson::minify(element)`).


The following code illustrates all of the above:

```
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

```
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


```
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  double v = parser.parse(abstract_json)["str"]["123"]["abc"];
  cout << "number: " << v << endl;
```


C++11 Support and string_view
-------------

The simdjson library builds on compilers supporting the [C++11 standard](https://en.wikipedia.org/wiki/C%2B%2B11). It is also a strict requirement: we have no plan to support older C++ compilers.

We represent parsed strings in simdjson using the `std::string_view` class. It avoids
the need to copy the data, as would be necessary with the `std::string` class. It also
avoids the pitfalls of null-terminated C strings.

The `std::string_view` class has become standard as part of C++17 but it is not always available
on compilers which only supports C++11. When we detect that `string_view` is natively
available, we define the macro `SIMDJSON_HAS_STRING_VIEW`.

When we detect that it is unavailable,
we use [string-view-lite](https://github.com/martinmoene/string-view-lite) as a
substitute. In such cases, we use the type alias `using string_view = nonstd::string_view;` to  
offer the same API, irrespective of the compiler and standard library. The macro
`SIMDJSON_HAS_STRING_VIEW` will be *undefined* to indicate that we emulate `string_view`.


C++17 Support
-------------

While the simdjson library can be used in any project using C++ 11 and above, field iteration has special support C++ 17's destructuring syntax. For example:

```
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
dom::parser parser;
dom::object object;
auto error = parser.parse(json).get(object);
if (error) { cerr << error << endl; return; }
for (auto [key, value] : object) {
  cout << key << " = " << value << endl;
}
```

For comparison, here is the C++ 11 version of the same code:

```
// C++ 11 version for comparison
padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
dom::parser parser;
dom::object object;
auto error = parser.parse(json).get(object);
if (!error) { cerr << error << endl; return; }
for (dom::key_value_pair field : object) {
  cout << field.key << " = " << field.value << endl;
}
```

Minifying JSON strings without parsing
----------------------

In some cases, you may have valid JSON strings that you do not wish to parse but that you wish to minify. That is, you wish to remove all unnecessary spaces. We have a fast function for this purpose (`simdjson::minify(const char * input, size_t length, const char * output, size_t& new_length)`). This function does not validate your content, and it does not parse it.  It is much faster than parsing the string and re-serializing it in minified form (`simdjson::minify(parser.parse())`). Usage is relatively simple. You must pass an input pointer with a length parameter, as well as an output pointer and an output length parameter (by reference). The output length parameter is not read, but written to. The output pointer should point to a valid memory region that is as large as the original string length. The input pointer and input length are read, but not written to.


```
  // Starts with a valid JSON document as a string.
  // It does not have to be null-terminated.
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  // Create a buffer to receive the minified string. Make sure that there is enough room (length bytes).
  std::unique_ptr<char[]> buffer{new char[length]};
  size_t new_length{}; // It will receive the minified length.
  auto error = simdjson::minify(some_string, length, buffer.get(), new_length);
  // The buffer variable now has "[1,2,3,4]" and new_length has value 9.
```

Though it does not validate the JSON input, it will detect when the document ends with an unterminated string. E.g., it would refuse to minify the string `"this string is not terminated` because of the missing final quote.


UTF-8 validation (alone)
----------------------

The simdjson library has fast functions to validate UTF-8 strings. They are many times faster than most functions commonly found in libraries. You can use our fast functions, even if you do not care about JSON.

```
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  bool is_ok = simdjson::validate_utf8(some_string, length);
```

The UTF-8 validation function merely checks that the input is valid UTF-8: it works with strings in general, not just JSON strings.

Your input string does not need any padding. Any string will do. The `validate_utf8` function does not do any memory allocation on the heap, and it does not throw exceptions.

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
You can apply a JSON path to any node and the path gets interpreted relatively, as if the currrent node were a whole JSON document.

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
    dom::object car;
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

```
dom::element doc;
auto error = parser.parse(json).get(doc);
if (error) { cerr << error << endl; exit(1); }
```

When you use the code this way, it is your responsibility to check for error before using the
result: if there is an error, the result value will not be valid and using it will caused undefined
behavior.

We can write a "quick start" example where we attempt to parse a file and access some data, without triggering exceptions:

```
#include "simdjson.h"

int main(void) {
  simdjson::dom::parser parser;

  simdjson::dom::element tweets;
  auto error = parser.load("twitter.json").get(tweets);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }

  simdjson::dom::element res;
  if ((error = tweets["search_metadata"]["count"].get(res))) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
}
```

### Error Handling Example

This is how the example in "Using the Parsed JSON" could be written using only error code checking:

```
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::array cars;
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

```
auto abstract_json = R"( [
    {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
    {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
  ] )"_padded;
dom::parser parser;
dom::array array;
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

```
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

```
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
  auto error = parser.parse(j,strlen(j))
        .at(0)
        .get(answer, error);
  if (error) { return false; }
  s.assign(answer.data(), answer.size());
  return true;
}
```

### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```
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

```
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

```
{ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 }
```

```
dom::parser parser;
dom::document_stream docs = parser.load_many("x.json");
for (dom::element doc : docs) {
  cout << doc["foo"] << endl;
}
// Prints 1 2 3
```


In-memory ndjson strings can be parsed as well, with `parser.parse_many(string)`: 


```
dom::parser parser;
  auto json = R"({ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 })"_padded;
dom::document_stream docs = parser.parse_many(json);
for (dom::element doc : docs) {
  cout << doc["foo"] << endl;
}
// Prints 1 2 3
```


Unlike `parser.parse`, both `parser.load_many(filename)` and `parser.parse_many(string)` may parse
"on demand" (lazily). That is, no parsing may have been done before you enter the loop 
`for (dom::element doc : docs) {` and you should expect the parser to only ever fully parse one JSON
document at a time.

1. When calling `parser.load_many(filename)`, the file's content is loaded up in a memory buffer owned by the `parser`'s instance. Thus the file can be safely deleted after calling `parser.load_many(filename)` as the parser instance owns all of the data.
2. When calling  `parser.parse_many(string)`, no copy is made of the provided string input. The provided memory buffer may be accessed each time a JSON document is parsed.  Calling `parser.parse_many(string)` on a  temporary string buffer (e.g., `docs = parser.parse_many("[1,2,3]"_padded)`) is unsafe  (and will not compile) because the  `document_stream` instance needs access to the buffer to return the JSON documents. In constrast, calling `doc = parser.parse("[1,2,3]"_padded)` is safe because `parser.parse` eagerly parses the input.

Both `load_many` and `parse_many` take an optional parameter `size_t batch_size` which defines the window processing size. It is set by default to a large value (`1000000` corresponding to 1 MB). None of your JSON documents should exceed this window size, or else you will get  the error `simdjson::CAPACITY`. You cannot set this window size larger than 4 GB: you will get  the error `simdjson::CAPACITY`. The smaller the window size is, the less memory the function will use. Setting the window size too small (e.g., less than 100 kB) may also impact performance negatively. Leaving it to 1 MB is expected to be a good choice, unless you have some larger documents.


Thread Safety
-------------

We built simdjson with thread safety in mind.

The simdjson library is single-threaded except for  `parse_many` which may use secondary threads under its control when the library is compiled with thread support.


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
