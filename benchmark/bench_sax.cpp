#include "simdjson.h"
#include "simdjson.cpp"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/begin_implementation.h"
#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#include <iostream>
#include <sstream>
#include <random>
#include <vector>

#include "partial_tweets/ondemand.h"
#include "partial_tweets/iter.h"
#include "partial_tweets/sax.h"
#include "partial_tweets/dom.h"

#include "largerandom/ondemand.h"
#include "largerandom/iter.h"
#include "largerandom/sax.h"
#include "largerandom/dom.h"

#include "kostya/ondemand.h"
#include "kostya/iter.h"
#include "kostya/dom.h"

BENCHMARK_MAIN();

#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/end_implementation.h"
#endif // SIMDJSON_IMPLEMENTATION_HASWELL
