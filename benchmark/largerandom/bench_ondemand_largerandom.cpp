#include "simdjson.h"
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#define BENCHMARK_NO_DOM

#include "largerandom/ondemand.h"

BENCHMARK_MAIN();
