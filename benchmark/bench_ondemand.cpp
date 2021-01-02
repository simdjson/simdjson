#include "simdjson.h"

#include <iostream>
#include <sstream>
#include <random>
#include <vector>

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson.h"
#endif
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

// simdjson ondemand
#include "partial_tweets/ondemand.h"
#include "largerandom/ondemand.h"
#include "largerandom/ondemand_unordered.h"
#include "kostya/ondemand.h"
#include "distinctuserid/ondemand.h"
#include "find_tweet/ondemand.h"

// simdjson dom
#include "partial_tweets/dom.h"
#include "largerandom/dom.h"
#include "kostya/dom.h"
#include "distinctuserid/dom.h"
#include "find_tweet/dom.h"

// // yyjson
#ifdef SIMDJSON_COMPETITION_YYJSON
#include "partial_tweets/yyjson.h"
#endif


BENCHMARK_MAIN();
