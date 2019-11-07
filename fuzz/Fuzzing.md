# Fuzzing

[Fuzzing](https://en.wikipedia.org/wiki/Fuzzing) is efficient for finding bugs. Here are a few bugs found by fuzzing:

 - https://github.com/lemire/simdjson/issues/353
 - https://github.com/lemire/simdjson/issues/351
 - https://github.com/lemire/simdjson/issues/345

Simdjson is continuously fuzzed on [oss-fuzz](https://github.com/google/oss-fuzz).

## Running the fuzzers locally

Make sure you have clang and cmake installed.
The easiest way to get started is to run the following, standing in the root of the checked out repo:
```
fuzz/build_like_ossfuzz.sh
```

Then invoke a fuzzer as tshown by the following example:
```
mkdir -p out/parser
build/fuzz/fuzz_parser out/parser/
```

