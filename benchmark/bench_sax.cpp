#include "simdjson.h"
#include "simdjson.cpp"
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#include "partial_tweets/sax.h"
#include "largerandom/sax.h"

BENCHMARK_MAIN();
