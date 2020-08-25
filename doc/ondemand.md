I'll write a few of the design features / innovations here:

## Classes

In general, simdjson's parser classes are divided into two categories:

* Persistent State:
  - Generally persisted through multiple algorithm runs.
  - Intended to be stored in memory.
  - We try not to access members of these structs repeatedly if we can avoid it.
  - We limit mallocs to these classes so they can be reused.
  - Examples: dom::parser, ondemand::parser, dom_parser_implementation
* Inline Algorithm Context:
  - Context and counters for a single hot-loop algorithm (stage 1, stage 2, etc.).
  - Member variables intended to be broken up and stored in registers where possible.
  - We try to store as few things as we can manage to avoid register pressure.
  - We never copy these (except perhaps during construction).
  - We *only* pass references / pointers to really_inline functions, expecting the
    compiler to eliminate the indirection.
  - Examples: stage2::structural_parser, stage1::json_structural_indexer

In ondemand, `ondemand::parser` and `dom_parser_implementation` are used to store persistent state,
and all other classes are inline algorithm context.

### ondemand::parser / dom_parser_implementation

On-demand parsing has several primary classes that keep the main state:

* `ondemand::parser`: Holds resources for parsing.
  - Persistent State.
  - `parser.parse()` calls stage 1 and then returns `ondemand::document` for you to iterate.
  - `structural_indexes`: Buffer for structural indexes for stage 1.
  - `string_buf`: Allocates string buffer for fast string parsing.
  - `buf`, `len`: cached pointer to JSON buffer (and length) being parsed.
  - `current_string_buf_loc`: The current append position in string_buf (see string parsing, later).
    Like buf and len, this is reset on every parse.
* `json_iterator`: Low-level iteration over JSON structurals from stage 1.
  - Not user-facing (though you can call doc.iterate() to get it right now, I don't think we want to
    expose that API unless we absolutely must).
  - Inline Algorithm Context. Stored internally in `document`.
  - `buf`: Pointer to JSON buffer, intended to be stored in a register since it's used so much.
    (This is also in dom_parser_implementation, but that's stored in memory and we're avoiding
    indirection here.)
  - `index`: Pointer to next structural index.
  - NOTE: probably *should* have a cached (or primary) copy of current_string_buf_loc for registerness.

### Value

value represents an value whose type is not yet known. The On Demand philosophy is to let the *user*
tell us what type the value is by asking us to convert it, and *then* check the type. A value can
be converted to an array or object, parsed to string, double, uint64_t, int64_t, or bool, or checked
for is_null().

* **Arrays** can be used with get_array(). More on that in the array section.
* **Objects** can be used with get_object(). More on that in the object section.
* **Strings** are parsed with get_string(), which parses the string, appending the unescaped value
  into a buffer allocated by the parser, and returning a `std::string_view` pointing at the buffer
  and telling you the location. We append a terminating `\0` in case you end up using these values
  with routines designed for C strings (the string_view's length of course does not include that
  `\0').
  
  These string_views *will be invalidated on the next parse,* so if you want to persist a copy,
  you'll need to allocate your own actual strings. This case is designed to be fast in the
  server-like scenario, where you parse a document, use the values, and only then move on to parse
  another one.

  Optionally, get_raw_json_string() gives you a pointer to the raw, unescaped JSON, allowing you to
  work with strings efficiently in cases where your format disallows escaping--for example,
  enumeration values and GUIDs.
* **Numbers** can be parsed with get_double(), get_uint64() and get_int64(), each of which is a separate,
battle-tested parser specifically targeted to the type. The algorithms give exact answers, even for
floating-point numbers, and check for overflow or invalid characters.
* **true, false and null** are parsed through get_bool() and is_null(). simdjson checks these quickly by
read the next 4 characters as a 32-bit integer and comparing that to true, fals, or null. Then it
checks the extra "e" in false, and ensures that the next character is either whitespace, or a JSON
operator (, : ] or }).

### Document

document represents the top level value in the document, behaving much like a value but also does
double duty, storing iteration state. It can be converted to an array or object, parsed to
string, double, uint64_t, int64_t, bool, or checked for is_null().

The document *must* be kept around during iteration, as all other iterator classes rely on it to
provide iterator state. 

If the document itself is a single number, boolean, or null, the parsing algorithm is very slightly
different from the value parsers, which rely on there being more JSON after the value. To
accomodate, simdjson copies the number or atom into a small buffer, places a space after it, and
then runs the normal algorithm. Strings, arrays and objects at the root all use exactly the same
stuff. (NOTE: I haven't implemented this difference yet.)

### Array

The `ondemand::array` object lets you iterate the values of an array in a forward-only manner:

```c++
for (object tweet : doc["tweets"].get_array()) {
  ...
}
```

This is equivalent to:

```c++
array tweets = doc["tweets"].get_array();
array::iterator iter = tweets.begin();
array::iterator end = tweets.end();
while (iter != end) {
  object tweet = *iter;
  ...
  iter++;
}
```

The way you *parse* an array is somewhat split into pieces by the iterator. Here is how we section
the work to parse and validate the array:

1. `get_array()`:
   - Validates that this is an array (starts with `[`). Returns INCORRECT_TYPE if not.
   - If the array is empty (followed by `]`), advances the iterator past the `]` and returns an
     array with finished == true.
   - If the array is not empty, returns an array with finished == false. Iterator remains pointed
     at the first value (the token just after `[`).
2. `tweets.begin()`, `tweets.end()`: Returns an `array::iterator`, which just points back at the
   array object.
3. `while (iter != end)`: Stops if finished == true.
4. `*iter`: Yields the value (or error).
   - If there is an error, returns it and sets finished == true.
   - Returns a value object, advancing the iterator just past the value token (if it is `[`, `{`,
     etc. then that will be handled when the value is converted to an array/object).
5. `iter++`: Expects the iterator to have finished with the previous array value, and looks for `,` or `]`.
   - Advances the iterator and gets the JSON `]` or `,`.
   - If the array just ended (there is a `]`), sets finished == true.
   - If the array continues (`,`), does nothing.
   - If anything else is there (`true`, `:`, `}`, etc.), sets error = TAPE_ERROR.
   - #3 gets run next.

### Array

The `ondemand::array` object lets you iterate the values of an array in a forward-only manner:

```c++
for (object tweet : doc["tweets"].get_array()) {
  ...
}
```

This is equivalent to:

```c++
array tweets = doc["tweets"].get_array();
array::iterator iter = tweets.begin();
array::iterator end = tweets.end();
while (iter != end) {
  object tweet = *iter;
  ...
  iter++;
}
```

The way you *parse* an array is somewhat split into pieces by the iterator. Here is how we section
the work to parse and validate the array:

1. `get_array()`:
   - Validates that this is an array (starts with `[`). Returns INCORRECT_TYPE if not.
   - If the array is empty (followed by `]`), advances the iterator past the `]` and returns an
     array with finished == true.
   - If the array is not empty, returns an array with finished == false. Iterator remains pointed
     at the first value (the token just after `[`).
2. `tweets.begin()`, `tweets.end()`: Returns an `array::iterator`, which just points back at the
   array object.
3. `while (iter != end)`: Stops if finished == true.
4. `*iter`: Yields the value (or error).
   - If there is an error, returns it and sets finished == true.
   - Returns a value object, advancing the iterator just past the value token (if it is `[`, `{`,
     etc. then that will be handled when the value is converted to an array/object).
5. `iter++`: Expects the iterator to have finished with the previous array value, and looks for `,` or `]`.
   - Advances the iterator and gets the JSON `]` or `,`.
   - If the array just ended (there is a `]`), sets finished == true.
   - If the array continues (`,`), does nothing.
   - If anything else is there (`true`, `:`, `}`, etc.), sets error = TAPE_ERROR.
   - #3 gets run next.

#### Error Chaining

When you iterate over an array or object with an error, the error is yielded in the loop

#### Error Chaining


* `document`: Represents the root value of the document, as well as iteration state.
  - Inline Algorithm Context. MUST be kept around for the duration of the algorithm.
  - `iter`: The `json_iterator`, which handles low-level iteration.
  - `parser`: A pointer to the parser to get at persistent state.
  - Can be converted to `value` (and by proxy, to array/object/string/number/bool/null).
  - Can be iterated (like `value`, assumes it is an array).
  - Safety: can only be converted to `value` once. This prevents double iteration of arrays/objects
    (which will cause inconsistent iterator state) or double-unescaping strings (which could cause
    memory overruns if done too much). NOTE: this is actually not ideal; it means you can't do
    `if (document.parse_double().error() == INCORRECT_TYPE) { document.parse_string(); }`

The `value` class is expected to be temporary (NOT kept around), used to check the type of a value
and parse it to another type. It can convert to array or object, and has helpers to parse string,
double, int64, uint64, boolean and is_null().

`value` does not check the type of the value ahead of time. Instead, when you ask for a value of a
given type, it tries to parse as that type, treating a parser error as INCORRECT_TYPE. For example,
if you call get_int64() on the JSON string `"123"`, it will fail because it is looking for either a
`-` or digit at the front. The philosophy here is "proceed AS IF the JSON has the desired type, and
have an unlikely error branch if not." Since most files have well-known types, this avoids
unnecessary branchiness and extra checks for the value type.

It does preemptively advance the iterator and store the pointer to the JSON, allowing you to attempt
to parse more than one type. This is how you do nullable ints, for example: check is_null() and
then get_int64(). If we didn't store the JSON, and instead did `iter.advance()` during is_null(),
you would get garbage if you then did get_int64().

## 

until it's asked to convert, in which case it proceeds
    *expecting* the value is of the given type, treating an error in parsing as "it must be some
    other type." This saves the extra explicit type check in the common case where you already know
    saving the if/switch statement
    and . We find out it's not a double
    when the double parser says "I couldn't find any digits and I don't understand what this `t`
    character is for."
  - The iterator has already been advanced past the value token. If it's { or [, the iterator is just
    after the open brace.
  - Can be parsed as a string, double, int, unsigned, boolean, or `is_null()`, in which case a parser is run
    and the value is returned.
  - Can be converted to array or object iterator.
* `object`: Represents an object iteration.
  - `doc`: A pointer to the document (and the iterator).
  - `at_start`: boolean saying whether we're at the start of the object, used for `[]`.
  - Can do order-sensitive field lookups.
  - Can iterate over key/value pairs.
* `array`: Represents an array iteration.
  - `doc`: A pointer to the document (and the iterator).
  

    
  - Can be returned as a `raw_json_string`, so that people who want to check JSON strings without
    unescaping can do so.
  - Can be converted to array or object
     and keep member variables in the same registers.
    In fact, , and . Usually based on whether they are persistent--i.e. we intend them
to be stored in memory--or algorithmic--
(which generally means we intend to persist them), or algorithm-lifetime, possibly

- persistent classes
- non-persistent 
`ondemand::parser` is the equivalent of `dom::parser`, and . 
`json_iterator` handles all iteration state.

### json_iterator / document

`document` owns the json_iterator and the . I'm considering moving this into the json_iterator so there's one
less class that requires persistent ownership, however.

### String unescaping

When the user requests strings, we unescape them to a single string_buf (much like the DOM parser)
so that users enjoy the same string performance as the core simdjson. The current_string_buf_loc is
presently stored in the 

We do not write the length to the string buffer, however; that is stored in the string_view we
return to the user, and immediately forgotten.

Presently, we use the `char string_buf[]` to write

 The top-level document stores a json_iterator inside, which 

It is illegal to move the document after it has been iterated (otherwise, pointers would be
invalidated). Note: I need to check that the current code actually checks this.

If it's stored in a `simdjson_result<document>` (as it would be in `auto doc = parser.parse(json)`),
the document pointed to by children is the one inside the simdjson_result, and the simdjson_result,
therefore, can't be moved either.

### Object Iteration

Because the C++ iterator contract requires iterators to be const-assignable and const-constructable,
object and array iterators are separate classes from the object/array itself, and have an interior
mutable reference to it.

### Array Iteration

### Iteration Safety

  - If the value fails to be parsed as one type, you can try to parse it as something else until you
    succeed.
  - Safety: If the value succeeds in being parsed or converted to a type, you cannot try again. (It
    sets `json` to null, so you will segfault.) This prevents double iteration of an array (which
    will cause inconsistent iterator state) or double-unescaping a string (which could cause memory
    overruns if done too much).
  - Guaranteed Iteration: If you discard a value without using it--perhaps you just wanted to know
    if it was null but didn't care what the actual value was--it will iterate. See 

This is an area I'm chomping at the bit for Rust, actually ... a whole lot of this would be safer if
we had compiler-enforced borrowing.

### Raw Key Lookup

### Skip Algorithm

### Root Scalar Parsing Without Malloc

The malloc when we parse the number / atoms at the root of the document has always bothered me a little, so I wrote alternate routines that use a stack-based buffer instead, based on the type in question. Atoms require no more than 6 bytes; integers no more than 21; and floats ... well, I [wanted your opinion on that, actually.](https://github.com/simdjson/simdjson/pull/947/files#diff-979f6706620f56f5d6a45ca3bf511669R166). I wanted to set a limit on the biggest possible float, and came up with:

> Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of fractional digits needed to distinguish any two doubles (including zeroes and significant digits). Add 8 more digits for the other stuff (`-0.<fraction>e-308`) -- and you get 1082.

Technically, people could add an arbitrary number of digits after that ... but we could actually scan for that and ignore, if we wanted. I know it's a lot of convolutions to avoid malloc / free, but I think there are really good reasons to have a 100% malloc-free library (well, we have integration points that malloc, but they are predictable and could easily be swapped out.

I considered just using separate algorithms, and I think for numbers in particular there is probably a way to do that without having two separate functions, but I haven't figured out the *clean* way yet.
