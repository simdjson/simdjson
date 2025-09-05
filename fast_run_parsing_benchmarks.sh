#!/bin/bash
#
# Top-level script to run all JSON PARSING benchmarks
# Tests: JSON → C++ structs performance
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set library path for Serde if it exists
if [ -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.so" ]; then
    export LD_LIBRARY_PATH="$SCRIPT_DIR/benchmark/static_reflect/serde-benchmark/target/release:$LD_LIBRARY_PATH"
fi

# The unified benchmark handles both datasets internally
# Pass --parsing flag to run parsing benchmarks (this is the default)
./benchmark/unified_benchmark --parsing
