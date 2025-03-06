## Rust Serde FFI

This folder includes FFI bindings for rust/serde.

### Links

- https://github.com/eqrion/cbindgen/blob/master/docs.md
- https://gist.github.com/zbraniecki/b251714d77ffebbc73c03447f2b2c69f
- https://michael-f-bryan.github.io/rust-ffi-guide/setting_up.html

### Building

- Generating cbindgen output
  - Install dependencies with `brew install cbindgen` or `apt-get install cbindgen` or `cargo install cbindgen` or the equivalent: we used `cargo install --version 0.23.0 cbindgen`.
  - Go to the directory where this README.md file is located
  - Generate with `cbindgen --config cbindgen.toml --crate serde-benchmark --output serde_benchmark.h`
- Building
  - Run with `cargo build --release`
