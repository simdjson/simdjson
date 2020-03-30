# 0.3

## Highlights

- **Multi-Document Parsing:** Read a bundle of JSON documents (ndjson) 2-4x faster than doing it
  individually. [API docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#newline-delimited-json-ndjson-and-json-lines) / [Design Details](https://github.com/simdjson/simdjson/blob/master/doc/parse_many.md)
- **Simplified API:** The API has been completely revamped for ease of use, including a new JSON
  navigation API and fluent support for error code *and* exception styles of error handling with a
  single API. [Docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#the-basics-loading-and-parsing-json-documents)
- **Exact Float Parsing:** Now simdjson parses floats flawlessly *without* any performance loss,
  thanks to [great work by @michaeleisel and @lemire](https://github.com/simdjson/simdjson/pull/558).
  [Blog Post](https://lemire.me/blog/2020/03/10/fast-float-parsing-in-practice/)
- **Even Faster:** The fastest parser got faster! With a [shiny new UTF-8 validator](https://github.com/simdjson/simdjson/pull/387)
  and meticulously refactored SIMD core, simdjson 0.3 is 15% faster than before, running at 2.5 GB/s
  (where 0.2 ran at 2.2 GB/s).

## Minor Highlights

- Fallback implementation: simdjson now has a non-SIMD fallback implementation, and can run even on
  very old 64-bit machines.
- Automatic allocation: as part of API simplification, the parser no longer has to be preallocated--
  it will adjust automatically when it encounters larger files.
- Runtime selection API: We've exposed simdjson's runtime CPU detection and implementation selection
  as an API, so you can tell what implementation we detected and test with other implementations.
- Error handling your way: Whether you use exceptions or check error codes, simdjson lets you handle
  errors in your style. APIs that can fail return simdjson_result<T>, letting you check the error
  code before using the result. But if you are more comfortable with exceptions, skip the error code
  and cast straight to T, and exceptions will be thrown automatically if an error happens. Use the
  same API either way!
- Error chaining: We also worked to keep non-exception error-handling short and sweet. Instead of
  having to check the error code after every single operation, now you can *chain* JSON navigation
  calls like looking up an object field or array element, or casting to a string, so that you only
  have to check the error code once at the very end.
