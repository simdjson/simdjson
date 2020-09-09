#include "simdjson.h"
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "simdjson/haswell/begin.h"
#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#include "partial_tweets/ondemand.h"
#include "partial_tweets/iter.h"
#include "partial_tweets/dom.h"

#include "largerandom/ondemand.h"
#include "largerandom/iter.h"
#include "largerandom/dom.h"

#include "kostya/ondemand.h"
#include "kostya/iter.h"
#include "kostya/dom.h"

BENCHMARK_MAIN();

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "simdjson/haswell/end.h"
#endif // SIMDJSON_IMPLEMENTATION_HASWELL
