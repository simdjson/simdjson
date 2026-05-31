# simdjson documentation

User-facing guides live in this directory. For building from source, testing, and internal layout, see [HACKING.md](../HACKING.md) ([#801](https://github.com/simdjson/simdjson/issues/801)).

## Guides

| Document | Description |
|----------|-------------|
| [basics.md](basics.md) | Getting started, parsing, On-Demand API, error handling |
| [dom.md](dom.md) | DOM API (materialized JSON trees) |
| [builder.md](builder.md) | JSON serialization / `string_builder` ([#1327](https://github.com/simdjson/simdjson/issues/1327)) |
| [parse_many.md](parse_many.md) | NDJSON / JSON Lines (`parse_many`) |
| [iterate_many.md](iterate_many.md) | Streaming iteration over many documents |
| [performance.md](performance.md) | Tuning, `NDEBUG`, buffers, huge pages |
| [implementation-selection.md](implementation-selection.md) | Runtime CPU dispatch |
| [tape.md](tape.md) | DOM tape layout (developer-oriented) ([#951](https://github.com/simdjson/simdjson/issues/951)) |
| [ondemand_design.md](ondemand_design.md) | On-Demand parsing model and depth ([#1533](https://github.com/simdjson/simdjson/issues/1533)) |
| [compile_time.md](compile_time.md) | Compile-time parsing (C++26) |
| [compile_time_accessors.md](compile_time_accessors.md) | Compile-time accessors |

API reference: [simdjson.github.io/simdjson](https://simdjson.github.io/simdjson/).
