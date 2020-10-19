#include "simdjson.h"
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#include "partial_tweets/ondemand.h"
#include "partial_tweets/iter.h"
#include "partial_tweets/dom.h"

#include "largerandom/ondemand.h"
#include "largerandom/iter.h"
#include "largerandom/dom.h"

#include "kostya/ondemand.h"
#include "kostya/iter.h"
#include "kostya/dom.h"

#include "distinctuserid/ondemand.h"
#include "distinctuserid/dom.h"


BENCHMARK_MAIN();
