#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS

#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson.h"
#endif

// This has to be last, for reasons I don't yet understand
#include <benchmark/benchmark.h>

SIMDJSON_POP_DISABLE_WARNINGS

#include "partial_tweets/simdjson_dom.h"
#include "partial_tweets/simdjson_ondemand.h"
#include "partial_tweets/yyjson.h"

#include "large_random/simdjson_dom.h"
#include "large_random/simdjson_ondemand.h"
#include "large_random/simdjson_ondemand_unordered.h"
#include "large_random/yyjson.h"

#include "kostya/simdjson_dom.h"
#include "kostya/simdjson_ondemand.h"
#include "kostya/yyjson.h"

#include "distinct_user_id/simdjson_dom.h"
#include "distinct_user_id/simdjson_ondemand.h"
#include "distinct_user_id/yyjson.h"

#include "find_tweet/simdjson_dom.h"
#include "find_tweet/simdjson_ondemand.h"
#include "find_tweet/yyjson.h"

BENCHMARK_MAIN();
