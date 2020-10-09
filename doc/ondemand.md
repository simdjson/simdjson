How simdjson's On Demand Parsing works
======================================

The simdjson On Demand API is a natural C++ DOM-like API backed by a forward-only iterator over JSON
source text with a natural C++ interface on top, including object and array iterators, object lookup, and conversion to native C++
types (string_view, double, bool, etc.).

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

It is designed around several principles:

* **Streaming (\*):** Does not preparse values, keeping memory usage and latency down. NOTE: Right now, simdjson has a preprocessing step that identifies the location of each value in the whole input. This means you have to pass the whole input, and the parser does have an internal buffer that consumes 4 bytes per JSON value (plus each operator like , : ] and }). This limitation will be reduced/eliminated in later versions.
* **Forward-Only:** To prevent reiteration of the same values and to keep the number of variables down (literally), only a single index is maintained and everything uses it (even if you have nested for loops). This means when you're going through an array of arrays, for example, that the inner array loop will advance the index to the next comma, and the array can just pick it up and look at it.
* **Natural Iteration:** A JSON array or object can be iterated with a normal C++ for loop. Nested arrays and objects are supported by nested for loops.
* **Use-Specific Parsing:** Parsing is always specific to the type the you ask for. For example, if you ask for an unsigned integer, we just start parsing digits. If there were no digits, we toss an error. There are even different parsers for double, uint64_t and int64_t. This avoids the branchiness of a generic "type switch," and makes the code more inlineable and compact.
* **Validate What You Use:** On Demand deliberately validates the values you use and the structure leading to it, but nothing else. The goal is a guarantee that the value you asked for is the correct one and is not malformed: there must be no confusion over whether you got the right value. But it leaves the possibility that the JSON *as a whole* is invalid. A full-validation mode is possible and planned, but I think this mode should be the default, personally, or at least pretty heavily advertised. Full-validation mode should really only be for debug.

Rationale
---------

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

To understand exactly what's happening here and why it's different, it's helpful to review the major
approaches to parsing and parser APIs in use today.

### Generic DOM Parsers

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

### SAX (SAJ?) Parsers

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

### Type Blindness and Branch Misprediction

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

Algorithm
---------

To help visualize the algorithm, we'll walk through the example C++ given at the top, for this JSON:

```json
{
  "statuses": [
    { "id": 1, "text": "first!", "user": { "screen_name": "lemire", "name": "Daniel" }, "favorite_count": 100, "retweet_count": 40 },
    { "id": 2, "text": "second!", "user": { "screen_name": "jkeiser2", "name": "John" }, "favorite_count": 2, "retweet_count": 3 }
  ]
}
```

### Starting the iteration

1. First, we declare a parser object that keeps internal buffers necessary for parsing. This can be
   reused to parse multiple JSON files, so you don't pay the high cost of allocating memory every
   time (and so it can stay in cache!).

   This declaration doesn't actually allocate any memory; that will happen in the next step.

   ```c++
   ondemand::parser parser;
   ```

2. We then start iterating the JSON document by allocating internal parser buffers, preprocessing
   the JSON, and initializing the iterator.

   ```c++
   auto doc = parser.iterate(json);
   ```
   
   Since this is the first time this parser has been used, iterate() first allocates internal
   parser buffers if this is the first time through. When reusing an existing parser, allocation
   only happens if the new document is bigger than internal buffers can handle. This is the only
   place On Demand does allocation.
   
   simdjson then preprocesses the JSON text at high speed, finding all tokens (i.e. the starting
   position of any JSON value, as well as any important operators like `,`, `:`, `]` or `}`).

   Finally, a `document` iterator is created, initialized at the position of the first value in the
   `json` text input. The document iterator is bumped forward by array / object iterators and
   object[] lookup, and must be kept around until iteration is complete.

   This operation can fail! The result type here is actually `simdjson_result<document>`.
   simdjson uses simdjson_result whenever a value needs to be returned, but the function could fail.
   It has an error_code and a document in it, and was designed to allow you to use either error code
   checking or C++ exceptions via a direct cast `document(parser.iterate(json)); you can use get()
   to check the error and cast to a value, or cast directly to a value.

   But as you can see, we don't check for the failure just yet.

3. We iterate over the "statuses" field using a typical C++ iterator, reading past the initial
   `{ "statuses": [ {`.

   ```c++
   for (ondemand::object tweet : doc["statuses"]) {
   ```

  This shorthand does a lot of stuff, and it's helpful to see what the initial bits expand to.
  Comments in front of each one explain what's going on:

  ```c++
  // Validate that the top-level value is an object: check for {
  ondemand::object top = doc.get_object();

  // Find the field statuses by:
  // 1. Check whether the object is empty (check for }). (TODO we don't really need to do this unless the key lookup fails!)
  // 2. Check if we're at the field by looking for the string "statuses".
  // 3. Validate that there is a `:` after it.
  auto tweets_field = top["statuses"];

  // Validate that the field value is an array: check for [
  // Also mark the array as finished if there is a ] next, which would cause the while () statement to exit immediately.
  ondemand::array tweets = tweets_field.get_array();
  // These three method calls do nothing substantial (the real checking happens in get_array() and ++)
  // != checks whether the array is marked as finished (if we have found a ]).
  ondemand::array_iterator tweets_iter = tweets.begin();
  while (tweets_iter != tweets.end()) {
    auto tweet_value = *tweets_iter;

    // Validate that the array element is an object: check for {
    ondemand::object tweet = tweet_value.get_object();
    ...
  }
  ```

   The one bit of shorthand that's not explained there is *error chaining*.
   Generally, you can use `document` methods on a simdjson_result<document> (this applies to all
   simdjson types); any errors will just be passed down the chain. Many method calls
   can be chained like this. So `for (object tweet : doc["statuses"])`, which is the equivalent of
   `object tweet = *(doc.get_object()["statuses"].get_array().begin()).get_object()` (what
   a mouthful!), could fail in any of 6 method calls, and the error will only be checked at the end,
   when you attempt to cast the final `simdjson_result<object>` to object (an exception will be
   thrown if there was an error).

4. We get the `"text"` field as a string.

   ```c++
   std::string_view text        = tweet["text"];
   ```

   First, `["text"]` skips the `"id"` field because it doesn't match: skips the key, `:` and
   value (`1`). We then check whether there are more fields by looking for either `,`
   or `}`.

   The second field is matched (`"text"`), so we validate the `:` and move to the actual value.

   NOTE: `["text"]` does a *raw match*, comparing the key directly against the raw JSON. This means
   that keys with escapes in them may not be matched.

   To convert to a string, we check for `"` and use simdjson's fast unescaping algorithm to copy
   `first!` (plus a terminating `\0`) into a buffer managed by the `document`. This buffer stores
   all strings from a single iteration. The next string will be written after the `\0`.

   A `string_view` is returned which points to that buffer, and contains the length.

4. We get the `"screen_name"` from the `"user"` object.

   ```c++
   std::string_view screen_name = tweet["user"]["screen_name"];
   ```

   First, `["user"]` checks whether there are any more object fields by looking for either `,` or
  `}`. Then it matches `"user"` and validates the `:`.

   `["screen_name"]` then converts to object, checking for `{`, and finds `"screen_name"`.

   To convert to string, `lemire` is written to the document's string buffer, which now has *two*
   string_views pointing into it, and looks like `first!\0lemire\0`.

   Finally, the temporary user object is destroyed, causing it to skip the remainder of the object
   (`}`).

5. We get `"retweet_count"` and `"favorite_count"` as unsigned integers.

   ```c++
   uint64_t         retweets    = tweet["retweet_count"];
   uint64_t         favorites   = tweet["favorite_count"];
   ```

   When it comes time to parse a number, we immediately.

6. We loop to the next tweet.

   ```c++
   for (ondemand::object tweet : doc["statuses"]) {
     ...
   }
   ```

   The relevant parts of the loop here are:

   ```c++
   while (iter != statuses.end()) {
     ondemand::object tweet = *iter;
     ...
     iter++;
   }
   ```

   First, the `tweet` destructor runs, skipping the remainder of the object (which in this case is
   just `}`).

   Next, `iter++` checks whether there are more values and finds `,`. The loop continues.

   Finally, `ondemand::object tweet = *iter` checks for `{` and returns the object.

   This tweet is processed just like the previous one.

7. We finish the last tweet.

   At the end of the loop, the `tweet` is first destroyed, skipping the remainder of the tweet
   object (`}`).
   
   `iter++` (from `for (ondemand::object tweet : doc["statuses"])`) then checks whether there are
   more values and finds there are not (`]`). It marks the array iteration as finished and the for
   loop terminates.

   Then the outer object is destroyed, skipping everything up to the `}`. TODO: I'm less than certain
   this actually happens: when does the temporary object actually go away, again?
Design Features
---------------

### String Parsing

When the user requests strings, we unescape them to a single string_buf (much like the DOM parser)
so that users enjoy the same string performance as the core simdjson. The current_string_buf_loc is
presently stored in the 

We do not write the length to the string buffer, however; that is stored in the string_view we
return to the user, and immediately forgotten.

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
    if it was null but didn't care what the actual value was--it will iterate. The destructor will
    take care of this.

### Raw Key Lookup

TODO

### Skip Algorithm

TODO

### Root Scalar Parsing Without Malloc

The malloc when we parse the number / atoms at the root of the document has always bothered me a little, so I wrote alternate routines that use a stack-based buffer instead, based on the type in question. Atoms require no more than 6 bytes; integers no more than 21; and floats ... well, I [wanted your opinion on that, actually.](https://github.com/simdjson/simdjson/pull/947/files#diff-979f6706620f56f5d6a45ca3bf511669R166). I wanted to set a limit on the biggest possible float, and came up with:

> Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of fractional digits needed to distinguish any two doubles (including zeroes and significant digits). Add 8 more digits for the other stuff (`-0.<fraction>e-308`) -- and you get 1082.

Technically, people could add an arbitrary number of digits after that ... but we could actually scan for that and ignore, if we wanted. I know it's a lot of convolutions to avoid malloc / free, but I think there are really good reasons to have a 100% malloc-free library (well, we have integration points that malloc, but they are predictable and could easily be swapped out.

I considered just using separate algorithms, and I think for numbers in particular there is probably a way to do that without having two separate functions, but I haven't figured out the *clean* way yet.
