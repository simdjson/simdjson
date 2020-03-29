# 0.3

## Highlights

- **Multi-Document Parsing:** read a bundle of JSON documents (ndjson) 2-4x faster than doing it
  individually. [API docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#newline-delimited-json-ndjson-and-json-lines) / [Design Details](https://github.com/simdjson/simdjson/blob/master/doc/JsonStream.md#motivations)
- **Simplified API:** The API has been completely revamped for ease of use, from the DOM API to
  error handling and error chaining! [Docs](https://github.com/simdjson/simdjson/blob/master/doc/basics.md#the-basics-loading-and-parsing-json-documents)
- **Exact Float Parsing:** simdjson now parses floats flawlessly *without* any performance loss,
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

