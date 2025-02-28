Builder
==========

Sometimes you want to generate JSON string outputs efficiently.
The simdjson library provides high-performance low-level facilities.
When using these low-level functionalities, you are responsible to
define the structure of your JSON document. However, string escaping
and UTF-8 validation is automated.

Overview: string_builder
---------------------------

The string_builder class is a low-level utility for constructing JSON strings representing documents. It is optimized for performance, potentially leveraging kernel-specific features like SIMD instructions for tasks such as string escaping. This class supports atomic types (e.g., booleans, numbers, strings) but does not handle composed types directly (like arrays or objects).

An `string_builder` is created with an initial buffer capacity (e.g., 1kB). The memory
is reallocated when needed. It has the following methods to add content to the string:


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
        builder.append_raw("{");

        // "make"
        builder.escape_and_append_with_quotes("make");
        builder.append_raw(":");
        builder.escape_and_append_with_quotes(car.make);

        // "model"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("model");
        builder.append_raw(":");
        builder.escape_and_append_with_quotes(car.model);

        // "year"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("year");
        builder.append_raw(":");
        builder.append(car.year);

        // "tire_pressure"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("tire_pressure");
        builder.append_raw(":[");

        // vector tire_pressure
        for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
            builder.append(car.tire_pressure[i]);
            if (i < car.tire_pressure.size() - 1) {
                builder.append_raw(",");
            }
        }
        // end of array
        builder.append_raw("]");

        // end of object
        builder.append_raw("}");
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