The Basics
==========

An overview of what you need to know to use simdjson, with examples.

* [Including simdjson](#including-simdjson)
* [The Basics: Loading and Parsing JSON Documents](#the-basics-loading-and-parsing-json-documents)
* [Using the Parsed JSON](#using-the-parsed-json)
* [JSON Pointer](#json-pointer)
* [Error Handling](#error-handling)
    * [Error Handling Example](#error-handling-example)
    * [Exceptions](#exceptions)
* [Newline-Delimited JSON (ndjson) and JSON lines](#newline-delimited-json-ndjson-and-json-lines)
* [Thread Safety](#thread-safety)

Including simdjson
------------------

To include simdjson, copy [simdjson.h](singleheader/simdjson.h) and [simdjson.cpp](singleheader/simdjson.cpp)
into your project. Then include it in your project with:

```c++
#include "simdjson.h"
using namespace simdjson; // optional
```

You can compile with:

```
c++ myproject.cpp simdjson.cpp --std=c++17
```

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

Using the Parsed JSON
---------------------

Once you have an element, you can navigate it with idiomatic C++ iterators, operators and casts.

* **Extracting Values:** You can cast a JSON element to a native type: `double(element)` or
  `double x = json_element`. This works for double, uint64_t, int64_t, bool,
  dom::object and dom::array. You can also use is_*typename*()` to test if it is a
  given type, and as_*typename*() to do the cast and return an error code on failure instead of an
  exception.
* **Field Access:** To get the value of the "foo" field in an object, use `object["foo"]`.
* **Array Iteration:** To iterate through an array, use `for (auto value : array) { ... }`. If you
  know the type of the value, you can cast it right there, too! `for (double value : array) { ... }`
* **Object Iteration:** You can iterate through an object's fields, too: `for (auto [key, value] : object)`
* **Array Index:** To get at an array value by index, use the at() method: `array.at(0)` gets the
  first element.
  > Note that array[0] does not compile, because implementing [] gives the impression indexing is a
  > O(1) operation, which it is not presently in simdjson.

Here are some examples of all of the above:

```c++
auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
dom::array cars = parser.parse(cars_json).get<dom::array>();

// Iterating through an array of objects
for (dom::object car : cars) {
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
  for (auto [key, value] : car) {
    cout << "- " << key << ": " << value << endl;
  }
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

### Error Handling Example

This is how the example in "Using the Parsed JSON" could be written using only error code checking:

```c++
auto cars_json = R"( [
{ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
{ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
{ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
dom::parser parser;
auto [cars, error] = parser.parse(cars_json).get<dom::array>();
if (error) { cerr << error << endl; exit(1); }

// Iterating through an array of objects
for (dom::element car_element : cars) {
dom::object car;
car_element.get<dom::object>().tie(car, error);
if (error) { cerr << error << endl; exit(1); }

// Accessing a field by name
dom::element make, model;
car["make"].tie(make, error);
if (error) { cerr << error << endl; exit(1); }
car["model"].tie(model, error);
if (error) { cerr << error << endl; exit(1); }
cout << "Make/Model: " << make << "/" << model << endl;

// Casting a JSON element to an integer
uint64_t year;
car["year"].get<uint64_t>().tie(year, error);
if (error) { cerr << error << endl; exit(1); }
cout << "- This car is " << 2020 - year << "years old." << endl;

// Iterating through an array of floats
double total_tire_pressure = 0;
dom::array tire_pressure_array;
car["tire_pressure"].get<dom::array>().tie(tire_pressure_array, error);
if (error) { cerr << error << endl; exit(1); }
for (dom::element tire_pressure_element : tire_pressure_array) {
    double tire_pressure;
    tire_pressure_element.get<double>().tie(tire_pressure, error);
    if (error) { cerr << error << endl; exit(1); }
    total_tire_pressure += tire_pressure;
}
cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

// Writing out all the information about the car
for (auto [key, value] : car) {
    cout << "- " << key << ": " << value << endl;
}
```

### Exceptions

Users more comfortable with an exception flow may choose to directly cast the `simdjson_result<T>` to the desired type:

```c++
dom::element doc = parser.parse(json); // Throws an exception if there was an error!
```

When used this way, a `simdjson_error` exception will be thrown if an error occurs, preventing the
program from continuing if there was an error.

Newline-Delimited JSON (ndjson) and JSON lines
----------------------------------------------

The simdjson library also support multithreaded JSON streaming through a large file containing many
smaller JSON documents in either [ndjson](http://ndjson.org) or [JSON lines](http://jsonlines.org)
format. If your JSON documents all contain arrays or objects, we even support direct file
concatenation without whitespace. The concatenated file has no size restrictions (including larger
than 4GB), though each individual document must be less than 4GB.

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

See [parse_many.md](parse_many.md) for detailed information and design.

Thread Safety
-------------

The simdjson library is mostly single-threaded. Thread safety is the responsibility of the caller:
it is unsafe to reuse a dom::parser object between different threads.

simdjson's CPU detection, which runs the first time parsing is attempted and switches to the fastest
parser for your CPU, is transparent and thread-safe.

The json stream parser is threaded, using a second thread under its own control. Like the single
document parser
