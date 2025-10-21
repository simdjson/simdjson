Builder
==========

Sometimes you want to generate JSON string outputs efficiently.
The simdjson library provides high-performance low-level facilities.
When using these low-level functionalities, you are responsible to
define the structure of your JSON document. Our more advanced interface
automates the process using C++26 static reflection: you get both high
speed and high convenience.

- [Builder](#builder)
  * [Overview: string_builder](#overview--string-builder)
  * [Example: string_builder](#example--string-builder)
  * [C++26 static reflection](#c--26-static-reflection)
    + [Without `string_buffer` instance](#without--string-buffer--instance)
    + [Without `string_buffer` instance but with explicit error handling](#without--string-buffer--instance-but-with-explicit-error-handling)

Overview: string_builder
---------------------------

The string_builder class is a low-level utility for constructing JSON strings representing documents. It is optimized for performance, potentially leveraging kernel-specific features like SIMD instructions for tasks such as string escaping. This class supports atomic types (e.g., booleans, numbers, strings) but does not handle composed types directly (like arrays or objects).
Note that JSON strings are always encoded as UTF-8.

An `string_builder` is created with an initial buffer capacity (e.g., 1kB). The memory
is reallocated when needed.
The efficiency of `string_builder` stems from its internal use of a resizable array or buffer. When you append data, it adds the characters to this buffer, resizing it only when necessary, typically in a way that minimizes reallocations. This approach contrasts with regular string concatenation, where each operation creates a new string, copying all previous content, leading to quadratic time complexity for repeated concatenations.


It has the following methods to add content to the string:


- `append(number_type v)`: Appends a number (including booleans) to the JSON buffer. Booleans are converted to the strings "false" or "true". Numbers are formatted according to the JSON standard, with floating-point numbers using the shortest representation that accurately reflects the value.
- `append(char c)`: Appends a single character to the JSON buffer.
- `append_null()`: Appends the string "null" to the JSON buffer.
- `clear()`: Clears the contents of the JSON buffer, resetting the position to 0 while retaining the allocated capacity.
- `escape_and_append(std::string_view input)`: Appends a string view to the JSON buffer after escaping special characters (e.g., quotes, backslashes) as required by JSON.
- `escape_and_append_with_quotes(std::string_view input)` Appends a string view surrounded by double quotes (e.g., "input") to the JSON buffer after escaping special characters. For constant strings, you may also do `escape_and_append_with_quotes<"mystring">()`.
- `escape_and_append_with_quotes(char input)`: Appends a single character surrounded by double quotes (e.g., "c") to the JSON buffer after escaping it if necessary.
- `append_raw(const char *c)`: Appends a null-terminated C string directly to the JSON buffer without escaping.
- `append_raw(std::string_view input)`: Appends a string view directly to the JSON buffer without escaping.
- `append_raw(const char *str, size_t len)`: Appends a specified number of characters from a C string directly to the JSON
- `append_key_value(key,value)`: Appends a key and a value (`"json":somevalue`)
- `append_key_value<"mykey">(value)`: Appends a key and a value (`"json":somevalue`), useful when the key is a compile-time constant (C++20).

After writing the content, if you have reasons to believe that the content might violate UTF-8 conventions, you can check it as follows:

- `validate_unicode()`: Checks if the content in the JSON buffer is valid UTF-8. Returns: true if the content is valid UTF-8, false otherwise.

You might need to do unicode validation if you have strings in your data structures containing
malformed UTF-8. Note that we do not automatically call `validate_unicode()`.

Once you are satisfied, you can recover the string as follows:

- `operator std::string()`: Converts the JSON buffer to an std::string. (Might throw if an error occurred.)
- `operator std::string_view()`: Converts the JSON buffer to an std::string_view.  (Might throw if an error occurred.)
- `view()`: Returns a view of the written JSON buffer as a `simdjson_result<std::string_view>` (C++20).

The later method (`view()`) is recommended.  For performance reasons, we expect you to explicitly call `validate_unicode()` as needed (e.g., prior to calling `view()`).

Example: string_builder
---------------------------

```cpp
struct Car {
    std::string make;
    std::string model;
    int64_t year;
    std::vector<double> tire_pressure;
};

void serialize_car(const Car& car, simdjson::builder::string_builder& builder) {
    // start of JSON
    builder.start_object();

    // "make"
    builder.append_key_value("make", car.make);
    builder.append_comma();

    // "model"
    builder.append_key_value("model", car.model);
    builder.append_comma();

    // "year"
    builder.append_key_value("year", car.year);
    builder.append_comma();

    // "tire_pressure"
    builder.escape_and_append_with_quotes("tire_pressure");
    builder.append_colon();
    builder.start_array();
    // vector tire_pressure
    for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
        builder.append(car.tire_pressure[i]);
        if (i < car.tire_pressure.size() - 1) {
            builder.append_comma();
        }
    }
    builder.end_array();
    builder.end_object();
}

bool car_test() {
    simdjson::builder::string_builder sb;
    Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
    serialize_car(c, sb);
    std::string_view p{sb};
    // p holds the JSON:
    // "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}"
    return true;
}
```




The `string_builder` constructor takes an optional parameter which specifies the initial
memory allocation in byte. If you know approximately the size of your JSON output, you can
pass this value as a parameter (e.g., `simdjson::builder::string_builder sb{1233213}`).

The `string_builder` might throw an exception in case of error when you cast it result to `std::string_view`. If you wish to avoid exceptions, you can use the following programming pattern:

```cpp
    std::string_view p;
    if(sb.view().get(p)) {
        return false; // there was an error
    }
```

In all cases, the `std::string_view` instance depends the corresponding `string_builder` instance.

### C++20



If you have C++20, you can simplify the code, as the `std::vector<double>` is automatically supported. Further, we can pass the keys (which are compile-time
constant) as template parameter (for improved performance).

```cpp
    Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
    simdjson::builder::string_builder sb;
    sb.start_object();
    sb.append_key_value<"make">(c.make);
    sb.append_comma();
    sb.append_key_value<"model">(c.model);
    sb.append_comma();
    sb.append_key_value<"year">(c.year);
    sb.append_comma();
    sb.append_key_value<"tire_pressure">(c.tire_pressure);
    sb.end_object();
    std::string_view p = sb.view();
```

With C++20, you can similarly handle standard containers transparently.
For example, you can serialize `std::map<std::string,T>` types.

```cpp
    std::map<std::string,double> c = {{"key1", 1}, {"key2", 1}};
    simdjson::builder::string_builder sb;
    sb.append(c);
    std::string_view p = sb.view();
```

You can also serialize `std::vector<T>` types.

```cpp
    std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
    simdjson::builder::string_builder sb;
    sb.append(c);
    std::string_view p = sb.view();
```

You can also skip the creation for the `string_builder` instance in such simple cases.

```cpp
std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
std::string json = simdjson::to_json(c);
```

We also have an overload for when you want to reuse the same `std::string` instance:


```cpp
std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
std::string json;
auto error = simdjson::to_json(c, json);
if(error) { /* there was an error */ }
```


We do recommend that you create and reuse the `string_builder` instance for performance
reasons.

You can also add custom serialization functions using a `tag_invoke` function.
For example, the following
function will allow you to serialize instances of the type `Car`.

```cpp
#include <simdjson>

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<float> tire_pressure;
};

namespace simdjson {

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type &builder, const Car& car) {
  builder.start_object();
  builder.append_key_value("make", car.make);
  builder.append_comma();
  builder.append_key_value("model", car.model);
  builder.append_comma();
  builder.append_key_value("year", car.year);
  builder.append_comma();
  builder.append_key_value("tire_pressure", car.tire_pressure);
  builder.end_object();
}

} // namespace simdjson
```

C++26 static reflection
------------------------

Static reflection (or compile-time reflection) in C++26 introduces a powerful compile-time mechanism that allows a program to inspect and manipulate its own structure, such as types, variables, functions, and other program elements, during compilation. Unlike runtime reflection in languages like Java or Python, C++26’s static reflection operates entirely at compile time, aligning with C++’s emphasis on zero-overhead abstractions and high performance. It means
that you can delegate much of the work to the library.
If you have a compiler with support C++26 static reflection, you can compile
your code with the `SIMDJSON_STATIC_REFLECTION` macro set:

```cpp
#define SIMDJSON_STATIC_REFLECTION 1
//...
#include "simdjson.h"
```

And then you can append your data structures to a `string_builder` instance
automatically. In most cases, it should work automatically:

```cpp
    struct Car {
        std::string make;
        std::string model;
        int64_t year;
        std::vector<double> tire_pressure;
    };

    bool car_test() {
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        sb << c;
        std::string_view p{sb};
        // p holds the JSON:
        // "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}"
        return true;
    }
```


### Without `string_buffer` instance

In some instances, you might want to create a string directly from your own data type.
You can create a string directly, without an explicit `string_builder` instance
with the `simdjson::to_json` template function.
(Under the hood a `string_builder` instance may still be created.)

```cpp
    struct Car {
        std::string make;
        std::string model;
        int64_t year;
        std::vector<double> tire_pressure;
    };

    void f() {
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        std::string json = simdjson::to_json(c);
    }
```

If you know the output size, in bytes, of your JSON string, you may
pass it as a second parameter (e.g., `simdjson::to_json(c, 31123)`).

Sometimes you may want to reuse the same `std::string` instance. We
have an overload for this purpose:

```cpp
Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
std::string s;
auto error = simdjson::to_json(c, s);
if(error) { /* there was an error */ }
```

You can then also add a third parameter for the expected output size in bytes.

### Extracting just some fields
In some instances, your class might have many fields that you do not want to serialize.
You can achieve this result with the `simdjson::extract_from` template. In the following
example, we serialize only the `year` and `price` fields on the `Car` instance.
```cpp
struct Car {
    std::string make;
    std::string model;
    int year;
    double price;
    bool electric;
};
Car car{"Ford", "F-150", 2024, 55000.0, false};
// Extract year and price
std::string json_result = simdjson::extract_from<"year", "price">(car);
// Alternatively:
// std::string json_result;
// auto error =  extract_from<"year", "price">(car).get(json_result);
// if(error) { /* error handling */ }
```

### Without `string_buffer` instance but with explicit error handling

If prefer a version without exceptions and explicit error handling, you can use the following
pattern:

```cpp
  std::string json;
  if(simdjson::to_json(c).get(json)) {
    // there was an error
  } else {
    // json contain the serialized JSON
  }
```

### Customization

If you want to serialize a value in a custome way, you can do it with a
`tag_invoke` specialization like the following example which will map
the year attribute to a string.


```cpp
#include <simdjson>

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<float> tire_pressure;
};

namespace simdjson {

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type &builder, const Car& car) {
  builder.start_object();
  builder.append_key_value("make", car.make);
  builder.append_comma();
  builder.append_key_value("model", car.model);
  builder.append_comma();
  builder.append_key_value("year", std::to_string(car.year));
  builder.append_comma();
  builder.append_key_value("tire_pressure", car.tire_pressure);
  builder.end_object();
}

} // namespace simdjson
```