# Compile-Time JSONPath and JSON Pointer Accessors

**Note:** This feature requires C++26 Static Reflection support (P2996) and is currently only available with experimental compilers. You must enable it with `-DSIMDJSON_STATIC_REFLECTION=ON` when building.

## Overview

simdjson provides compile-time JSONPath and JSON Pointer accessors that validate paths against struct definitions at compile time and generate optimized accessor code with zero runtime overhead. This combines the safety of compile-time type checking with the performance of pre-parsed, pre-validated access paths.

## Requirements

- C++26 compiler with Static Reflection support (P2996)
- Experimental compiler flags:
  - Clang with P2996 support: `-std=c++26 -freflection -fexpansion-statements`
- Build configuration: `-DSIMDJSON_STATIC_REFLECTION=ON`

## How It Works

**Compile Time:**
1. Path string is parsed and converted to access steps
2. Path is validated against struct definition using reflection
3. Field types are checked and verified
4. Optimized accessor code is generated

**Runtime:**
- Direct navigation with no parsing
- No validation overhead
- No string comparisons for path components
- Type-safe extraction

## Two Usage Modes

### Mode 1: With Type Validation (Recommended)

When you provide a struct type, the compiler validates the entire path at compile time:

```cpp
struct User {
  std::string name;
  int age;
  std::vector<std::string> emails;
};

//  R"( ... )" is a C++ raw string literal.
const padded_string json = R"({
  "name": "Alice",
  "age": 30,
  "emails": ["alice@example.com", "alice@work.com"]
})"_padded;

ondemand::parser parser;
auto doc = parser.iterate(json);

// Compile-time validation: checks that User has "name" field of type std::string
std::string name;
auto result = ondemand::json_path::at_path_compiled<User, ".name">(doc);
result.get(name); // name = "Alice"

// Compile-time validation: checks that "emails" is array-like with string elements
std::string email;
result = ondemand::json_path::at_path_compiled<User, ".emails[0]">(doc);
result.get(email); // email = "alice@example.com"
```

**Benefits:**
- **Compile-time errors** if path doesn't exist in struct
- **Type safety** - verifies field types match expected types
- **Refactoring protection** - renaming struct fields causes compile errors

**What gets validated:**
- Field existence
- Field types
- Array/container access validity
- Nested struct navigation

### Mode 2: Without Validation

When you omit the struct type, the path is parsed at compile time but not validated:

```cpp
const padded_string json = R"({
  "name": "Alice",
  "age": 30,
  "address": {"city": "Boston"}
})"_padded;

ondemand::parser parser;
auto doc = parser.iterate(json);

// No compile-time validation - path is only parsed
std::string name;
auto result = ondemand::json_path::at_path_compiled<".name">(doc);
result.get(name); // name = "Alice"

std::string_view city;
result = ondemand::json_path::at_path_compiled<".address.city">(doc);
result.get(city); // city = "Boston"
```

**Benefits:**
- Works with dynamic/unknown JSON structures
- Still benefits from compile-time path parsing
- No runtime string parsing overhead

**Use when:**
- JSON structure is not known at compile time
- Working with varied JSON schemas
- Prototyping or exploratory parsing

## JSONPath Syntax

JSONPath uses dot notation and bracket notation for field access:

### Supported Syntax

| Syntax | Description | Example |
|--------|-------------|---------|
| `.field` | Dot notation for field access | `.name`, `.address.city` |
| `["field"]` | Bracket notation with quotes | `["name"]`, `["address"]["city"]` |
| `[index]` | Array index access | `[0]`, `[1]` |
| Mixed | Combination of notations | `.emails[0]`, `["users"][0].name` |
| `$` prefix | Optional root indicator | `$.name`, `$["name"]` |

### Examples

```cpp
struct Address {
  std::string city;
  int zip;
};

struct Person {
  std::string name;
  int age;
  Address address;
  std::vector<std::string> emails;
};

// Dot notation
at_path_compiled<Person, ".name">(doc)
at_path_compiled<Person, ".address.city">(doc)

// Bracket notation
at_path_compiled<Person, "[\"name\"]">(doc)
at_path_compiled<Person, "[\"address\"][\"city\"]">(doc)

// Array access
at_path_compiled<Person, ".emails[0]">(doc)
at_path_compiled<Person, ".emails[1]">(doc)

// Mixed notation
at_path_compiled<Person, ".address[\"zip\"]">(doc)
at_path_compiled<Person, "[\"emails\"][0]">(doc)

// With root indicator
at_path_compiled<Person, "$.name">(doc)
at_path_compiled<Person, "$.address.city">(doc)
```

## JSON Pointer Syntax

JSON Pointer (RFC 6901) uses slash-separated paths:

### Supported Syntax

| Syntax | Description | Example |
|--------|-------------|---------|
| `/field` | Field access | `/name`, `/address/city` |
| `/index` | Array index | `/0`, `/1` |
| `~0` | Escaped `~` | `/field~0name` → field~name |
| `~1` | Escaped `/` | `/field~1name` → field/name |

### Examples

```cpp
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

// Field access
at_pointer_compiled<Car, "/make">(doc)
at_pointer_compiled<Car, "/model">(doc)

// Array access
at_pointer_compiled<Car, "/tire_pressure/0">(doc)
at_pointer_compiled<Car, "/tire_pressure/1">(doc)

// Root pointer (returns whole document)
at_pointer_compiled<Car, "">(doc)
at_pointer_compiled<Car, "/">(doc)
```

## API Reference

### JSONPath Functions

```cpp
// With type validation
template<typename T, constevalutil::fixed_string Path, typename DocOrValue>
simdjson_result<value> at_path_compiled(DocOrValue& doc_or_val);

// Without validation
template<constevalutil::fixed_string Path, typename DocOrValue>
simdjson_result<value> at_path_compiled(DocOrValue& doc_or_val);
```

### JSON Pointer Functions

```cpp
// With type validation
template<typename T, constevalutil::fixed_string Pointer, typename DocOrValue>
simdjson_result<value> at_pointer_compiled(DocOrValue& doc_or_val);

// Without validation
template<constevalutil::fixed_string Pointer, typename DocOrValue>
simdjson_result<value> at_pointer_compiled(DocOrValue& doc_or_val);
```

### Direct Field Extraction

Extract values directly into variables with compile-time type checking:

```cpp
// JSONPath
template<typename T, constevalutil::fixed_string Path>
struct path_accessor {
  template<typename DocOrValue, typename FieldType>
  static error_code extract_field(DocOrValue& doc_or_val, FieldType& target);
};

// JSON Pointer
template<typename T, constevalutil::fixed_string Pointer>
struct pointer_accessor {
  template<typename DocOrValue, typename FieldType>
  static error_code extract_field(DocOrValue& doc_or_val, FieldType& target);
};
```

**Example:**

```cpp
struct User {
  std::string name;
  int age;
};

ondemand::parser parser;
auto doc = parser.iterate(json);

// Extract directly into variable
std::string name;
ondemand::json_path::path_accessor<User, ".name">::extract_field(doc, name);

int age;
ondemand::json_path::pointer_accessor<User, "/age">::extract_field(doc, age);
```

The compiler verifies that the target variable type matches the field type at the path.

## Complete Examples

### Example 1: Validated Access

```cpp
#include "simdjson.h"
using namespace simdjson;

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

int main() {
  const padded_string json = R"({
    "make": "Toyota",
    "model": "Camry",
    "year": 2018,
    "tire_pressure": [40.1, 39.9, 37.7, 40.4]
  })"_padded;

  ondemand::parser parser;
  auto doc = parser.iterate(json);

  // Type-validated access
  std::string make;
  auto result = ondemand::json_path::at_path_compiled<Car, ".make">(doc);
  result.get(make); // make = "Toyota"

  // Array access with validation
  double pressure;
  result = ondemand::json_path::at_path_compiled<Car, ".tire_pressure[1]">(doc);
  result.get(pressure); // pressure = 39.9

  return 0;
}
```

### Example 2: Non-Validated Access

```cpp
#include "simdjson.h"
using namespace simdjson;

int main() {
  const padded_string json = R"({
    "user": {
      "name": "Alice",
      "preferences": {
        "theme": "dark",
        "notifications": true
      }
    }
  })"_padded;

  ondemand::parser parser;
  auto doc = parser.iterate(json);

  // No validation - works with any JSON structure
  std::string_view theme;
  auto result = ondemand::json_path::at_path_compiled<".user.preferences.theme">(doc);
  result.get(theme); // theme = "dark"

  bool notifications;
  result = ondemand::json_path::at_path_compiled<".user.preferences.notifications">(doc);
  result.get(notifications); // notifications = true

  return 0;
}
```

### Example 3: Direct Extraction

```cpp
#include "simdjson.h"
using namespace simdjson;

struct Person {
  std::string name;
  int age;
  std::vector<std::string> emails;
};

int main() {
  const padded_string json = R"({
    "name": "Bob",
    "age": 25,
    "emails": ["bob@example.com", "bob@work.com"]
  })"_padded;

  ondemand::parser parser;
  auto doc = parser.iterate(json);

  // Extract with type validation
  std::string name;
  ondemand::json_path::path_accessor<Person, ".name">::extract_field(doc, name);
  // name = "Bob"

  int age;
  ondemand::json_path::pointer_accessor<Person, "/age">::extract_field(doc, age);
  // age = 25

  std::string email;
  ondemand::json_path::path_accessor<Person, ".emails[0]">::extract_field(doc, email);
  // email = "bob@example.com"

  return 0;
}
```

## Error Handling

Compile-time errors occur when:
- Path doesn't exist in struct: `static_assert` failure
- Field type mismatch: `static_assert` failure
- Invalid array access on non-array field: `static_assert` failure

Runtime errors occur when:
- JSON structure doesn't match expected structure
- Array index out of bounds
- Type conversion failures

```cpp
struct User {
  std::string name;
  int age;
};

// Compile-time error: no "email" field in User
// auto result = ondemand::json_path::at_path_compiled<User, ".email">(doc);

// Compile-time error: age is not an array
// auto result = ondemand::json_path::at_path_compiled<User, ".age[0]">(doc);

// Runtime error if JSON doesn't have "name" field
auto result = ondemand::json_path::at_path_compiled<User, ".name">(doc);
std::string name;
if (result.get(name) != SUCCESS) {
  // Handle error
}
```

## Performance

Compile-time accessors provide:
- **Zero path parsing overhead** - paths parsed at compile time
- **Zero validation overhead** - validation done at compile time
- **Direct field access** - no runtime path traversal
- **Type-safe extraction** - no dynamic type checking

Compared to runtime `at_path()` and `at_pointer()`:
- Eliminates runtime path string parsing
- Eliminates runtime path validation
- Generates optimal code path directly

## Limitations

- Requires C++26 compiler with P2996 support (experimental)
- Paths must be compile-time constants (string literals)
- Cannot use runtime-computed paths
- Limited to struct types that support reflection
- Array indices must be compile-time constants in the path

## When to Use

**Use compile-time accessors when:**
- You have well-defined struct types
- JSON structure is known at compile time
- You want maximum type safety
- Performance is critical

**Use runtime `at_path()`/`at_pointer()` when:**
- JSON structure varies or is unknown
- Paths are computed at runtime
- Working with C++20 or earlier
- Flexibility is more important than compile-time checks

## See Also

- [JSON Pointer](basics.md#json-pointer) - Runtime JSON Pointer support
- [JSONPath](basics.md#jsonpath) - Runtime JSONPath support
- [Static Reflection for Deserialization](basics.md#3-using-static-reflection-c26) - Using reflection for full struct deserialization
