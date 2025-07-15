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

Overview: string_builder
---------------------------

The string_builder class is a low-level utility for constructing JSON strings representing documents. It is optimized for performance, potentially leveraging kernel-specific features like SIMD instructions for tasks such as string escaping. This class supports atomic types (e.g., booleans, numbers, strings) but does not handle composed types directly (like arrays or objects).

An `string_builder` is created with an initial buffer capacity (e.g., 1kB). The memory
is reallocated when needed.
The efficiency of `string_builder` stems from its internal use of a resizable array or buffer. When you append data, it adds the characters to this buffer, resizing it only when necessary, typically in a way that minimizes reallocations. This approach contrasts with regular string concatenation, where each operation creates a new string, copying all previous content, leading to quadratic time complexity for repeated concatenations.


It has the following methods to add content to the string:


- `append(number_type v)`: Appends a number (including booleans) to the JSON buffer. Booleans are converted to the strings "false" or "true". Numbers are formatted according to the JSON standard, with floating-point numbers using the shortest representation that accurately reflects the value.
- `append(char c)`: Appends a single character to the JSON buffer.
- `append_null()`: Appends the string "null" to the JSON buffer.
- `clear()`: Clears the contents of the JSON buffer, resetting the position to 0 while retaining the allocated capacity.
- `escape_and_append(std::string_view input)`: Appends a string view to the JSON buffer after escaping special characters (e.g., quotes, backslashes) as required by JSON.
- `escape_and_append_with_quotes(std::string_view input)` Appends a string view surrounded by double quotes (e.g., "input") to the JSON buffer after escaping special characters.
Parameters:
- `escape_and_append_with_quotes(char input)`: Appends a single character surrounded by double quotes (e.g., "c") to the JSON buffer after escaping it if necessary.
- `append_raw(const char *c)`: Appends a null-terminated C string directly to the JSON buffer without escaping.
- `append_raw(std::string_view input)`: Appends a string view directly to the JSON buffer without escaping.
- `append_raw(const char *str, size_t len)`: Appends a specified number of characters from a C string directly to the JSON

After writting the content, if you have reasons to believe that the content might violate UTF-8 conventions, you can check it as follows:

- `validate_unicode()`: Checks if the content in the JSON buffer is valid UTF-8. Returns: true if the content is valid UTF-8, false otherwise.

Once you are satisfied, you can recover the string as follows:

- `operator std::string()`: Converts the JSON buffer to an std::string. (Might throw if an error occurred.)
- `operator std::string_view()`: Converts the JSON buffer to an std::string_view.  (Might throw if an error occurred.)
- `view()`: Returns a view of the written JSON buffer as a `simdjson_result<std::string_view>`.

The later method (`view()`) is recommended.

Example: string_builder
---------------------------

```C++

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
        std::string_view p;
        if(sb.view().get(p)) {
            return false; // there was an error
        }
        // p holds the JSON:
        // "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}"
        return true;
    }
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
    bool car_test() {
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        append(sb, c);
        std::string_view p;
        if(sb.view().get(p)) {
            return false; // there was an error
        }
        // p holds the JSON:
        // "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}"
        return true;
    }
```

If you prefer, you can also create a string directly:

```cpp
  std::string json;
  if(simdjson::builder::to_json_string(c).get(json)) {
    // there was an error
  } else {
    // json contain the serialized JSON
  }
```