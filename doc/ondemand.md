On Demand Basics
================

On Demand is a new, faster simdjson API with all the ease-of-use you are used to. While it provides a
familiar DOM interface, under the hood it is different: it is parsing values *as you use them.*
With On Demand, you do not waste time parsing JSON you do not use, and you do not pay the cost of generating
an intermediate DOM tree.

We provide an overview of what you need to know to use the simdjson On Demand API, with examples.

* [Including ondemand](#including-on-demand)
* [The Basics: Loading and Parsing JSON Documents](#the-basics-loading-and-parsing-json-documents)
* [Using the Parsed JSON](#using-the-parsed-json)

The On Demand API supports the same JSON standards and C++ compilers as simdjson's DOM API. Refer to the DOM docs for more information:

* [Requirements](basics.md##requirements)
* [Using simdjson as a CMake dependency](#using-simdjson-as-a-cmake-dependency)
* [Error Handling](basics.md#error-handling)
  * [Error Handling Example](basics.md#error-handling-example)
  * [Exceptions](basics.md#exceptions)
* [Thread Safety](basics.md#thread-safety)
* [Standard Compliance](basics.md#standard-compliance)
* [C++11 Support and string_view](basics.md#c11-support-and-string_view)
* [C++17 Support](basics.md#c17-support)
* [Backwards Compatibility](basics.md#backwards-compatibility)

For deeper information about the design and implementation of the simdjson On Demand API, refer to
the [design document](ondemand.md).

Including On Demand
------------------

To include simdjson, copy [simdjson.h](/singleheader/simdjson.h) and [simdjson.cpp](/singleheader/simdjson.cpp)
into your project. Then include it in your project with:

```c++
#include "simdjson.h"
using namespace simdjson; // optional
```

You can generally compile with:

```
c++ -O3 myproject.cpp simdjson.cpp
```


Note:
- Users on macOS and other platforms where compilers do not provide C++11 compliant by default
should request it with the appropriate flag (e.g., `c++ -march=native -std=c++17 myproject.cpp simdjson.cpp`).

The Basics: Loading and Parsing JSON Documents
----------------------------------------------

The simdjson library offers a simple DOM tree API, which you can access by creating a
`ondemand::parser` and calling the `iterate()` method:

```c++
ondemand::parser parser;
auto json = padded_string::load("twitter.json");
ondemand::document doc = parser.iterate(json); // load and parse a file
```

Or by creating a padded string (for efficiency reasons, simdjson requires a string with
SIMDJSON_PADDING bytes at the end) and calling `iterate()`:

```c++
ondemand::parser parser;
auto json = "[1,2,3]"_padded; // The _padded suffix creates a simdjson::padded_string instance
ondemand::document doc = parser.iterate(json); // parse a string
```

If you have a buffer of your own with enough padding already (SIMDJSON_PADDING extra bytes allocated), you can use `promise_padded` to pass it in:

```c++
ondemand::parser parser;
char json[3+SIMDJSON_PADDING];
strcpy(json, "[1]");
ondemand::document doc = parser.iterate(json, strlen(json), sizeof(json));
```

Documents Are Iterators
-----------------------

A `document` is *not* a fully-parsed JSON value; rather, it is an **iterator** over the JSON text.
This means that while you iterate an array, or search for a field in an object, it is actually
walking through the original JSON text, merrily reading commas and colons and brackets to make sure
you get where you are going. This is the key to On Demand's performance: since it's just an iterator,
it lets you parse values as you use them. And particularly, it lets you *skip* values you do not want
to use.

### Parser, Document and JSON Scope

Because a document is an iterator over the JSON text, both the JSON text and the parser must
remain alive (in scope) while you are using it. Further, a `parser` may have at most
one document open at a time, since it holds allocated memory used for the parsing.

During the `iterate` call, the original JSON text is never modified--only read. After you are done
with the document, the source (whether file or string) can be safely discarded.

For best performance, a `parser` instance should be reused over several files: otherwise you will
needlessly reallocate memory, an expensive process. It is also possible to avoid entirely memory
allocations during parsing when using simdjson. [See our performance notes for details](performance.md).

Using the Parsed JSON
---------------------

Once you have a document, you can navigate it with idiomatic C++ iterators, operators and casts.
The following show how to use the JSON when exceptions are enabled, but simdjson has full, idiomatic
support for users who avoid exceptions. See [the simdjson DOM API's error handling documentation](basics.md#error-handling) for more.

* **Extracting Values:** You can cast a JSON element to a native type:
  `double(element)` or `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  ondemand::object and ondemand::array. At this point, the number, string or boolean will be parsed,
  or the initial `[` or `{` will be verified. An exception is thrown if the cast is not possible.

  > IMPORTANT NOTE: values can only be parsed once. Since documents are *iterators*, once you have
  > parsed a value (such as by casting to double), you cannot get at it again.
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`. This will
  scan through the object looking for the field with the matching string.

  > NOTE: simdjson does *not* unescape keys when matching. This is not generally a problem for
  > applications with well-defined key names (which generally do not use escapes). If you do need this
  > support, it's best to iterate through the object fields to find the field you are looking for.
  >
  > By default, field lookup is order-insensitive, so you can look up values in any order. However,
  > we still encourage you to look up fields in the order you expect them in the JSON, as it is still
  > much faster.
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
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. This will
  step through each value in the JSON array.

  If you know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`.
* **Object Iteration:** You can iterate through an object's fields, as well: `for (auto field : object) { ... }`
  - `field.unescaped_key()` will get you the key string.
  - `field.value()` will get you the value, which you can then use all these other methods on.
* **Array Index:** Because it is forward-only, you cannot look up an array element by index. Instead,
  you will need to iterate through the array and keep an index yourself.

### Examples

The following code illustrates many of the above concepts:

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

Here is a different example illustrating the same ideas:

```C++
ondemand::parser parser;
auto points_json = R"( [
    {  "12345" : {"x":12.34, "y":56.78, "z": 9998877}   },
    {  "12545" : {"x":11.44, "y":12.78, "z": 11111111}  }
  ] )"_padded;

// Parse and iterate through an array of objects
for (ondemand::object points : parser.iterate(points_json)) {
  for (auto point : points) {
    cout << "id: " << std::string_view(point.unescaped_key()) << ": (";
    cout << point.value()["x"].get_double() << ", ";
    cout << point.value()["y"].get_double() << ", ";
    cout << point.value()["z"].get_int64() << endl;
  }
}
```

And another one:

```C++
auto abstract_json = R"(
  { "str" : { "123" : {"abc" : 3.14 } } }
)"_padded;
ondemand::parser parser;
auto doc = parser.iterate(abstract_json);
cout << doc["str"]["123"]["abc"].get_double() << endl; // Prints 3.14
```

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
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  cout << value << endl; // Prints 3.14
  ```
