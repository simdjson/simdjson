# Parse json at compile time
   * [Introduction](#introduction)
   * [Example](#example)
   * [Loading from disk](#loading-from-disk)
   * [Limitations (compile-time errors)](#limitations-compile-time-errors)


## Introduction
In some instances, you may want to configure your software at compile-time with a JSON document.
Maybe you have a single code base but many different possible configurations, all resulting in
different software. For example, you might be programming robots, using the same software, but
different robot configurations.

To achieve the desire result, you have a few options. You may start the software and parser a
JSON file at runtime. Or you might convert your JSON data into C++ code that you can compile with
your software.

With C++26, there is another way: parse the JSON file along with your C++ code. In this manner,
the JSON data becomes native C++ data.

The simdjson library support parsing JSON documents at compile time if you have C++26 support. To
activate C++26 reflection support, you can compile
your code with the `SIMDJSON_STATIC_REFLECTION` macro set:

```cpp
#define SIMDJSON_STATIC_REFLECTION 1
//...
#include "simdjson.h"
```

The `simdjson::compile_time::parse_json` function parses a JSON document at **compile time** and returns a `constexpr` structure reflecting its content. We support the full range of JSON values, which are mapped to C++ types as in
the following table.


| JSON type      | C++ type                         |
|----------------|----------------------------------|
| object         | anonymous struct                 |
| array          | `std::array<T, N>` (homogeneous) |
| string         | `const char*` (UTF-8)            |
| number         | `int64_t`, `uint64_t`, `double`  |
| `true`/`false` | `bool`                           |
| `null`         | `std::nullptr_t`                 |

## Example

Suppose you want to parse the following JSON document:

```cpp
{
    "port": 8080,
    "host": "localhost",
    "debug": true
}
```

You can do so, at compile-time, as follows:


```cpp
constexpr auto cfg = simdjson::compile_time::parse_json<R"({
    "port": 8080,
    "host": "localhost",
    "debug": true
})">();

// cfg.port == 8080
// std::string_view(cfg.host) == "localhost"
// cfg.debug == true
```

You can nest objects and arrays:

```cpp
constexpr auto data = simdjson::compile_time::parse_json<R"({
    "servers": [
        {"host": "s1", "port": 3000},
        {"host": "s2", "port": 3001}
    ]
})">();


// data.servers.size() == 2
// std::string_view(data.servers[0].host) == "s1"
```

Top-level arrays are allowed:

```cpp
constexpr auto arr = simdjson::compile_time::parse_json<R"([1, 2, 3])">();
static_assert(arr.size() == 3);
static_assert(arr[1] == 2);
```

## Loading from disk

In practice, you may have a JSON file, say `json_data` that you want to parse
at compile time. You may do so as follows.

```c++
constexpr const char json_data[] = {
    #embed "test.json"
    , 0
};

constexpr auto json = simdjson::compile_time::parse_json<json_data>();
```


## Limitations (compile-time errors)

We have a few limitations which trigger compile-time errors if violated.


- Only JSON objects and arrays are supported at the top level (no primitives).
  We will lift this limitation in the future.
- Strings are represented using the const char * in UTF-8, but they must not
  contain embedded nulls. We would prefer to represent them as std::string or
  std::string_view, and hope to do so in the future.
- Heterogeneous arrays are not supported yet. E.g., you need to have arrays of
  all integers, or all strings, all floats, all compatible objects, etc.
  For example, the following is accepted:
  ```json
   [
    { "name": "Alice", "age": 30 },
    { "name": "Bob", "age": 25 },
    { "name": "Charlie", "age": 35 }
  ]
  ```
  but the following is not:
  ```json
  [
    { "name": "Alice", "age": 30 },
    "Just a string",
    42,
    { "name": "Charlie", "age": 35 }
  ]
  ```
  We may support it Heterogeneous in the future with std::variant types.
- We parse the first JSON document encountered in the string. Trailing
  characters are ignored. Thus if you JSON begins with {"a":1}, everything
  after the closing brace is ignored. This limitation will be lifted in the future,
  reporting an error.

These limitations are safe in the sense that they result in compile-time errors.
Thus you will not get truncated strings or imprecise floats silently.


