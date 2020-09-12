How simdjson's On Demand Parsing works
======================================

Current JSON parsers generally have either ease of use or performance. Very few have both at
once. simdjson's On Demand API bridges that gap with a familiar, friendly DOM API and the
performance of just-in-time parsing on top of the simdjson core's legendary performance.

To achieve ease of use, we mimicked the *form* of a traditional DOM API: you can iterate over
arrays, look up fields in objects, and extract native values like double, uint64_t, string and bool.
To achieve performance, we introduced some key limitations that make the DOM API *streaming*:
array/object iteration cannot be restarted, and fields must be looked up in order, and string/number
values can only be parsed once.

```c++
ondemand::parser parser;
auto doc = parser.iterate(json);
for (auto tweet : doc["statuses"]) {
  std::string_view text        = tweet["text"];
  std::string_view screen_name = tweet["user"]["screen_name"];
  uint64_t         retweets    = tweet["retweet_count"];
  uint64_t         favorites   = tweet["favorite_count"];
  cout << screen_name << " (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
}
```

This streaming approach means that fields or values you don't use don't have to get parsed or
converted, saving space and time.

Further, the On Demand API doesn't parse a value *at all* until you try to convert it to double,
int, string, or bool. Because you have told it the type at that point, it can avoid the the key
"what type is this" branch present in almost all other parsers, avoiding branch misprediction that
cause massive (sometimes 2-4x) slowdowns.

Parsers Today
-------------

To understand exactly what's happening here and why it's different, it's helpful to review the major
approaches to parsing and parser APIs in use today.

### Generic DOM

Many of the most usable, popular JSON APIs deserialize into a **DOM**: an intermediate tree of
objects, arrays and values. The resulting API lets you refer to each array or object separately,
using familiar techniques like iteration (`for (auto value : array)`) or indexing (`object["key"]`).
In some cases the values are even deserialized straight into familiar C++ constructs like vector and
map.

This model is dead simple to use, since it talks in terms of *data types* instead of JSON. It's
often easy enough that many users use the deserialized JSON as-is instead of deserializing into
their own custom structs, saving a ton of development work.

simdjson's DOM parser is one such example. It looks very similar to the ondemand example, except
it calls `parse` instead of `iterate`:

```c++
dom::parser parser;
auto doc = parser.parse(json);
for (auto tweet : doc["statuses"]) {
  std::string_view text        = tweet["text"];
  std::string_view screen_name = tweet["user"]["screen_name"];
  uint64_t         retweets    = tweet["retweet_count"];
  uint64_t         favorites   = tweet["favorite_count"];
  cout << screen_name << " (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
}
```

Pros and cons of generic DOM:
* Straightforward, user-friendly interface (arrays and objects)
* No lifetime concerns (arrays and objects are often independent of JSON text and parser internal state)
* Parses and stores everything, using memory/CPU even on values you don't end up using (cost can be brought down some with lazy numbers/strings and top-level iterators)
* Values stay in memory even if you only use them once
* Heavy performance drain from [type blindness](#type-blindness).

### SAX (SAJ?)

The SAX model ("Streaming API for XML") uses streaming to eliminate the high cost of
parsing and storing the entire JSON. In the SAX model, a core JSON parser parses the JSON document
piece by piece, but instead of stuffing values in a DOM tree, it passes each value to a callback,
letting the user use the value and decide for themselves whether to discard it and where to store
it. or discard it.

This allows users to work with much larger files without running out of memory. Some SAX APIs even
allow the user to skip values entirely, lightening the parsing burden as well.

The big drawback is complexity: SAX APIs generally have you define a single callback for each type
(e.g. `string_field(std::string_view key, std::string_view value)`). Because of this, you suffer
from context blindness: when you find a string you have to check where it is before you know what to
do with it. Is this string the text of the tweet, the screen name, or something else I don't even
care about? Are we even in a tweet right now, or is this from some other place in the document
entirely?

The following is SAX example of the Twitter problem we've seen in the Generic DOM and On Demand
examples. To make it short enough to use as an example at all, it's heavily redacted: it only solves
a part of the problem (doesn't get user.screen_name), it has bugs (it doesn't handle sub-objects
in a tweet at all), and it uses a theoretical, simple SAX API that minimizes ceremony and skips over
the issue of lazy parsing and number types entirely.

```c++
struct twitter_callbacks {
  bool in_statuses;
  bool in_tweet;
  std::string_view text;
  uint64_t         retweets;
  uint64_t         favorites;
  void start_object_field(std::string_view key) {
    if (key == "statuses") { in_statuses = true; }
  }
  void start_object() {
    if (in_statuses) { in_tweet = true; }
  }
  void string_field(std::string_view key, std::string_view value) {
    if (in_tweet && key == "text") { text = value; }
  }
  void number_field(std::string_view key, uint64_t value) {
    if (in_tweet) {
      if (key == "retweet_count") { retweets = value; }
      if (key == "favorite_count") { favorites = value; }
    }
  }
  void end_object() {
    if (in_tweet) {
      cout << "[redacted] (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
      in_tweet = false;
    } else if (in_statuses) {
      in_statuses = false;
    }
  }
};
sax::parser parser;
parser.parse(twitter_callbacks());
```

This is a startling amount of code, requiring mental gymnastics even to read, and in order to get it
this small and illustrate basic usage, *it has bugs* and skips over parsing user.screen_name
entirely. The real implementation is much, much harder to write (and to read).

Pros and cons of SAX (SAJ):
* Speed and space benefits from low, predictable memory usage
* Some SAX APIs can lazily parse numbers/strings, another performance win (pay for what you use)
* Performance drain from context blindness (switch statement for "where am I in the document")
* Startlingly difficult to use

### Schema-Based Parser Generators

There is another breed of parser, commonly used to generate REST API clients, which is in principle
capable of fixing most of the issues with DOM and SAX. These parsers take a schema--a description of
your JSON, with field names, types, everything--and generate classes/structs in your language of
choice, as well as a parser to deserialize the JSON into those structs. (In another variant, you
define your own struct and a preprocessor inspects it and generates a JSON parser for it.)

Not all of these schema-based parser generators actually generate a parser or even optimize for
streaming, but they are *able* to.

Some of the features help entirely eliminate the DOM and SAX issues:

Pros and cons:
* Ease of Use is on par with DOM
* Parsers that generate iterators and lazy values in structs can keep memory pressure down to SAX levels.
* Type Blindness can be entirely solved with specific parsers for each type, saving many branches.
* Context Blindness can be solved, especially if object fields are required and in order, saving
  even more branches.
* Scenarios are limited by declarative language (often limited to deserialization-to-objects)

Rust's serde does a lot of the necessary legwork here, for example. (Editor's Note: I don't know
*how much* it does, but I know it does a decent amount, and is very fast.)

Type Blindness and Branch Misprediction
---------------------------------------

The DOM parsing model, and even the SAX model to a great extent, suffers from **type
blindness:** you, the user, almost always know exactly what fields and what types are in your JSON,
but the parser doesn't. When you say `json_parser.parse(json)`, the parser doesn't get told *any*
of this. It has no way to know. This means it has to look at each value blind with a big "switch"
statement, asking "is this a number? A string? A boolean? An array? An object?"

In modern processors, this kind of switch statement can make your program run *3-4 times slower*
than it needs to. This is because of the high cost of branch misprediction.

Modern processors have more than one core, even on a single thread. To go fast, each of these cores
"reads ahead" in your program, each picking different instructions to run (as soon as data is
available). If all the cores are working almost all the time, your single-threaded program will run
around 4 instructions per cycle--*4 times faster* than it theoretically could.

Most modern programs don't manage to get much past 1 instruction per cycle, however. This is
because of branch misprediction. Programs have a lot of if statements, so to read ahead, processors
guess which branch will be taken and read ahead from that branch. If it guesses wrong, all that
wonderful work it did is discarded, and it starts over from the if statement. It *was* running at
4x speed, but it was all wasted work!

And this brings us back to that switch statement. Type blindness means the processor essentially has
to guess, for every JSON value, whether it will be an array, an object, number, string or boolean.
Unless your file is something ridiculously predictable, like a giant array of numbers, it's going to
trip up a lot. (Processors get better about this all the time, but for something complex like this
there is only so much it can do in the tiny amount of time it has to guess.)

On Demand parsing is tailor-made to solve this problem at the source, parsing values only after the
user declares their type by asking for a double, an int, a string, etc.



NOTE: EVERYTHING BELOW THIS NEEDS REWRITING AND MAY NOT BE ACCURATE AT PRESENT
==============================================================================

Classes
-------

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
for (object tweet : doc["statuses"].get_array()) {
  ...
}
```

This is equivalent to:

```c++
array tweets = doc["statuses"].get_array();
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
   - If the array is empty (followed by `]`), advances the iterator past the `]` and then releases the
     iterator (indicating the array is finished iterating).
   - If the array is not empty, the iterator remains pointed at the first value (the token just
     after `[`).
2. `tweets.begin()`, `tweets.end()`: Returns an `array_iterator`, which just points back at the
   array object.
3. `while (iter != end)`: Stops if the iterator has been released.
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

Design Features
---------------

#### Error Chaining

When you iterate over an array or object with an error, the error is yielded in the loop

### String Parsing

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

### Object/Array Iteration

Because the C++ iterator contract requires iterators to be const-assignable and const-constructable,
object and array iterators are separate classes from the object/array itself, and have an interior
mutable reference to it.

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
