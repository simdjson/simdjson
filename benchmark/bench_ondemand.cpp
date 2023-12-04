#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS

#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson.h"
#endif

#ifdef SIMDJSON_COMPETITION_RAPIDJSON
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#endif

#ifdef SIMDJSON_COMPETITION_SAJSON
#include "sajson.h"
#endif

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#ifdef SIMDJSON_COMPETITION_BOOSTJSON
#include <boost/json.hpp>
#endif

// This has to be last, for reasons I don't yet understand
#include <benchmark/benchmark.h>

SIMDJSON_POP_DISABLE_WARNINGS
#include "json2msgpack/simdjson_ondemand.h"
#include "json2msgpack/simdjson_dom.h"
#include "json2msgpack/yyjson.h"
#include "json2msgpack/rapidjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "json2msgpack/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "json2msgpack/nlohmann_json.h"
#include "json2msgpack/boostjson.h"

#include "partial_tweets/simdjson_ondemand.h"
#include "partial_tweets/simdjson_dom.h"
#include "partial_tweets/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "partial_tweets/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "partial_tweets/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "partial_tweets/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "partial_tweets/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "partial_tweets/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "partial_tweets/boostjson.h"


#include "distinct_user_id/simdjson_ondemand.h"
#include "distinct_user_id/simdjson_ondemand_json_pointer.h"
#include "distinct_user_id/simdjson_dom.h"
#include "distinct_user_id/simdjson_dom_json_pointer.h"
#include "distinct_user_id/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "distinct_user_id/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "distinct_user_id/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "distinct_user_id/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "distinct_user_id/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "distinct_user_id/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "distinct_user_id/boostjson.h"

#include "find_tweet/simdjson_ondemand.h"
#include "find_tweet/simdjson_dom.h"
#include "find_tweet/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "find_tweet/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "find_tweet/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "find_tweet/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "find_tweet/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "find_tweet/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "find_tweet/boostjson.h"

#include "top_tweet/simdjson_ondemand.h"
#include "top_tweet/simdjson_dom.h"
#include "top_tweet/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "top_tweet/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "top_tweet/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "top_tweet/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "top_tweet/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "top_tweet/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "top_tweet/boostjson.h"


#include "kostya/simdjson_ondemand.h"
#include "kostya/simdjson_dom.h"
#include "kostya/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "kostya/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "kostya/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "kostya/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "kostya/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "kostya/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "kostya/boostjson.h"

#include "large_random/simdjson_ondemand.h"
#if SIMDJSON_COMPETITION_ONDEMAND_UNORDERED
#include "large_random/simdjson_ondemand_unordered.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_UNORDERED
#include "large_random/simdjson_dom.h"
#include "large_random/yyjson.h"
#if SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "large_random/sajson.h"
#endif // SIMDJSON_COMPETITION_ONDEMAND_SAJSON
#include "large_random/rapidjson.h"
#if SIMDJSON_COMPETITION_SAX
#include "large_random/rapidjson_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "large_random/nlohmann_json.h"
#if SIMDJSON_COMPETITION_SAX
#include "large_random/nlohmann_json_sax.h"
#endif // SIMDJSON_COMPETITION_SAX
#include "large_random/boostjson.h"

#include "amazon_cellphones/simdjson_dom.h"
#include "amazon_cellphones/simdjson_ondemand.h"

#include "large_amazon_cellphones/simdjson_dom.h"
#include "large_amazon_cellphones/simdjson_ondemand.h"

BENCHMARK_MAIN();
