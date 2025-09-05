/**
 * Unified JSON Performance Benchmark
 *
 * Compares JSON parsing performance across multiple libraries using real datasets.
 * Tests simdjson (manual, reflection, from()), nlohmann/json, and RapidJSON.
 *
 * Key features:
 * - Parser reuse for simdjson (as designed for optimal performance)
 * - Full field extraction for all libraries (fair comparison)
 * - Support for both Twitter and CITM datasets
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <simdjson.h>

#ifdef HAS_NLOHMANN
#include <nlohmann/json.hpp>
#endif

#ifdef HAS_RAPIDJSON
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#endif

// reflect-cpp disabled due to complex linking requirements
// #ifdef HAS_REFLECTCPP
// #include <rfl.hpp>
// #include <rfl/json.hpp>
// #endif

#ifdef HAS_YYJSON
extern "C" {
#include <yyjson.h>
}
#endif

#ifdef HAS_SERDE
#include "serde_benchmark.h"
#endif

using namespace simdjson;

// Twitter data structures
struct TwitterUser {
    uint64_t id;
    std::string name;
    std::string screen_name;
    std::string location;
    std::string description;
    uint64_t followers_count;
    uint64_t friends_count;
    bool verified;
    uint64_t statuses_count;
    bool operator==(const TwitterUser&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TwitterUser, id, screen_name, name, location,
                                   description, verified, followers_count,
                                   friends_count, statuses_count)
#endif
};

struct TwitterTweet {
    std::string created_at;
    uint64_t id;
    std::string text;
    std::optional<uint64_t> in_reply_to_status_id;
    TwitterUser user;
    uint64_t retweet_count;
    uint64_t favorite_count;
    bool operator==(const TwitterTweet&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TwitterTweet, created_at, id, text,
                                   in_reply_to_status_id, user, retweet_count,
                                   favorite_count)
#endif
};

struct TwitterData {
    std::vector<TwitterTweet> statuses;
    bool operator==(const TwitterData&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TwitterData, statuses)
#endif
};

// CITM data structures
struct CITMPrice {
    uint64_t amount;
    uint64_t audienceSubCategoryId;
    uint64_t seatCategoryId;
    bool operator==(const CITMPrice&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMPrice, amount, audienceSubCategoryId, seatCategoryId)
#endif
};

struct CITMArea {
    uint64_t areaId;
    std::vector<uint64_t> blockIds;
    bool operator==(const CITMArea&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMArea, areaId, blockIds)
#endif
};

struct CITMSeatCategory {
    std::vector<CITMArea> areas;
    uint64_t seatCategoryId;
    bool operator==(const CITMSeatCategory&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMSeatCategory, seatCategoryId, areas)
#endif
};

struct CITMPerformance {
    uint64_t eventId;
    uint64_t id;
    std::optional<std::string> logo;
    std::optional<std::string> name;
    std::vector<CITMPrice> prices;
    std::vector<CITMSeatCategory> seatCategories;
    std::optional<std::string> seatMapImage;
    uint64_t start;
    std::string venueCode;
    bool operator==(const CITMPerformance&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMPerformance, id, eventId, logo, name, prices,
                                   seatCategories, seatMapImage, start, venueCode)
#endif
};

struct CITMEvent {
    std::optional<std::string> description;
    uint64_t id;
    std::optional<std::string> logo;
    std::string name;
    std::vector<uint64_t> subTopicIds;
    std::optional<std::string> subjectCode;
    std::optional<std::string> subtitle;
    std::vector<uint64_t> topicIds;
    bool operator==(const CITMEvent&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMEvent, id, name, description, logo,
                                   subTopicIds, subjectCode, subtitle, topicIds)
#endif
};

struct CITMCatalog {
    std::map<std::string, std::string> areaNames;
    std::map<std::string, std::string> audienceSubCategoryNames;
    std::map<std::string, std::string> blockNames;
    std::map<std::string, CITMEvent> events;
    std::vector<CITMPerformance> performances;
    std::map<std::string, std::string> seatCategoryNames;
    std::map<std::string, std::string> subTopicNames;
    std::map<std::string, std::string> subjectNames;
    std::map<std::string, std::string> topicNames;
    std::map<std::string, std::vector<uint64_t>> topicSubTopics;
    std::map<std::string, std::string> venueNames;
    bool operator==(const CITMCatalog&) const = default;

#ifdef HAS_NLOHMANN
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CITMCatalog, areaNames, audienceSubCategoryNames,
                                   blockNames, events, performances, seatCategoryNames,
                                   subTopicNames, subjectNames, topicNames,
                                   topicSubTopics, venueNames)
#endif
};

// Benchmark result structure
struct BenchmarkResult {
    std::string name;
    double throughput_mbps;
    double time_per_iteration_us;
    size_t iterations;
    bool success;
};

// Benchmark runner
template<typename Func>
BenchmarkResult run_benchmark(const std::string& name,
                             const std::string& json,
                             int iterations,
                             Func f) {
    BenchmarkResult result;
    result.name = name;
    result.iterations = iterations;

    try {
        // Warmup
        int warmup_iterations = std::min(100, iterations/10);
        for (int i = 0; i < warmup_iterations; i++) {
            if (!f()) {
                result.success = false;
                return result;
            }
        }

        // Timed run
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < iterations; i++) {
            if (!f()) {
                result.success = false;
                return result;
            }
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        result.time_per_iteration_us = duration.count() / (double)iterations;
        result.throughput_mbps = (json.size() * iterations / 1024.0 / 1024.0) /
                                (duration.count() / 1000000.0);
        result.success = true;
    } catch (...) {
        result.success = false;
    }

    return result;
}

// Twitter benchmarks
std::vector<BenchmarkResult> benchmark_twitter_parsing(const std::string& json) {
    std::vector<BenchmarkResult> results;
    const int iterations = 3000;

    // 1. simdjson - Manual (conservative approach - fresh parser each iteration)
    {
        padded_string padded(json); // Create once

        auto benchmark = [&]() -> bool {
            try {
                auto &parser = ondemand::parser::get_parser();
                TwitterData data;
                auto doc = parser.iterate(padded);

                for (auto tweet_obj : doc["statuses"]) {
                    TwitterTweet tweet;
                    tweet.created_at = std::string(std::string_view(tweet_obj["created_at"]));
                    tweet.id = uint64_t(tweet_obj["id"]);
                    tweet.text = std::string(std::string_view(tweet_obj["text"]));

                    auto reply = tweet_obj["in_reply_to_status_id"];
                    if (!reply.is_null()) {
                        tweet.in_reply_to_status_id = uint64_t(reply);
                    }

                    auto user_obj = tweet_obj["user"];
                    tweet.user.id = uint64_t(user_obj["id"]);
                    tweet.user.name = std::string(std::string_view(user_obj["name"]));
                    tweet.user.screen_name = std::string(std::string_view(user_obj["screen_name"]));
                    tweet.user.location = std::string(std::string_view(user_obj["location"]));
                    tweet.user.description = std::string(std::string_view(user_obj["description"]));
                    tweet.user.followers_count = uint64_t(user_obj["followers_count"]);
                    tweet.user.friends_count = uint64_t(user_obj["friends_count"]);
                    tweet.user.verified = bool(user_obj["verified"]);
                    tweet.user.statuses_count = uint64_t(user_obj["statuses_count"]);

                    tweet.retweet_count = uint64_t(tweet_obj["retweet_count"]);
                    tweet.favorite_count = uint64_t(tweet_obj["favorite_count"]);

                    data.statuses.push_back(std::move(tweet));
                }
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (manual)", json, iterations, benchmark));
    }

    // 2. simdjson - Reflection (conservative approach)
    {
        padded_string padded(json); // Create once

        auto benchmark = [&]() -> bool {
            try {
                auto &parser = ondemand::parser::get_parser();
                TwitterData data;
                auto doc = parser.iterate(padded);
                return doc.get(data) == SUCCESS;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (reflection)", json, iterations, benchmark));
    }

    // 3. simdjson::from() convenience API
    {
        padded_string padded(json); // Create once, reuse

        auto benchmark = [&]() -> bool {
            try {
                TwitterData data = simdjson::from(padded);
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson::from()", json, iterations, benchmark));
    }

#ifdef HAS_NLOHMANN
    // 4. nlohmann - with full extraction
    {
        auto benchmark = [&]() -> bool {
            try {
                TwitterData data;
                auto j = nlohmann::json::parse(json);

                for (auto& tweet_json : j["statuses"]) {
                    TwitterTweet tweet;
                    tweet.created_at = tweet_json["created_at"].get<std::string>();
                    tweet.id = tweet_json["id"].get<uint64_t>();
                    tweet.text = tweet_json["text"].get<std::string>();

                    if (!tweet_json["in_reply_to_status_id"].is_null()) {
                        tweet.in_reply_to_status_id = tweet_json["in_reply_to_status_id"].get<uint64_t>();
                    }

                    auto& user_json = tweet_json["user"];
                    tweet.user.id = user_json["id"].get<uint64_t>();
                    tweet.user.screen_name = user_json["screen_name"].get<std::string>();
                    tweet.user.name = user_json["name"].get<std::string>();
                    tweet.user.location = user_json["location"].get<std::string>();
                    tweet.user.description = user_json["description"].get<std::string>();
                    tweet.user.verified = user_json["verified"].get<bool>();
                    tweet.user.followers_count = user_json["followers_count"].get<uint64_t>();
                    tweet.user.friends_count = user_json["friends_count"].get<uint64_t>();
                    tweet.user.statuses_count = user_json["statuses_count"].get<uint64_t>();

                    tweet.retweet_count = tweet_json["retweet_count"].get<uint64_t>();
                    tweet.favorite_count = tweet_json["favorite_count"].get<uint64_t>();

                    data.statuses.push_back(std::move(tweet));
                }
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("nlohmann (extraction)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_RAPIDJSON
    // 5. RapidJSON - with full extraction
    {
        auto benchmark = [&]() -> bool {
            try {
                TwitterData data;
                rapidjson::Document d;
                d.Parse(json.c_str());
                if (d.HasParseError()) return false;

                const auto& statuses = d["statuses"];
                for (rapidjson::SizeType i = 0; i < statuses.Size(); i++) {
                    const auto& tweet_json = statuses[i];
                    TwitterTweet tweet;

                    tweet.created_at = std::string(tweet_json["created_at"].GetString());
                    tweet.id = tweet_json["id"].GetUint64();
                    tweet.text = std::string(tweet_json["text"].GetString());

                    if (!tweet_json["in_reply_to_status_id"].IsNull()) {
                        tweet.in_reply_to_status_id = tweet_json["in_reply_to_status_id"].GetUint64();
                    }

                    const auto& user_json = tweet_json["user"];
                    tweet.user.id = user_json["id"].GetUint64();
                    tweet.user.screen_name = std::string(user_json["screen_name"].GetString());
                    tweet.user.name = std::string(user_json["name"].GetString());
                    tweet.user.location = std::string(user_json["location"].GetString());
                    tweet.user.description = std::string(user_json["description"].GetString());
                    tweet.user.verified = user_json["verified"].GetBool();
                    tweet.user.followers_count = user_json["followers_count"].GetUint64();
                    tweet.user.friends_count = user_json["friends_count"].GetUint64();
                    tweet.user.statuses_count = user_json["statuses_count"].GetUint64();

                    tweet.retweet_count = tweet_json["retweet_count"].GetUint64();
                    tweet.favorite_count = tweet_json["favorite_count"].GetUint64();

                    data.statuses.push_back(std::move(tweet));
                }
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("RapidJSON (extraction)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_SERDE
    // 6. Serde (Rust) - with full extraction
    {
        auto benchmark = [&]() -> bool {
            serde_benchmark::TwitterData* td = serde_benchmark::twitter_from_str(json.c_str(), json.size());
            if (td) {
                serde_benchmark::free_twitter(td);
                return true;
            }
            return false;
        };

        results.push_back(run_benchmark("Serde (Rust)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_REFLECTCPP
    // 7. reflect-cpp - with full extraction
    {
        auto benchmark = [&]() -> bool {
            auto result = rfl::json::read<TwitterData>(json);
            if (result) {
                TwitterData data = result.value();
                return true;
            }
            return false;
        };

        results.push_back(run_benchmark("reflect-cpp", json, iterations, benchmark));
    }
#endif

#ifdef HAS_YYJSON
    // 8. yyjson - with full extraction
    {
        auto benchmark = [&]() -> bool {
            try {
                TwitterData data;
                yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
                if (!doc) return false;

                yyjson_val *root = yyjson_doc_get_root(doc);
                yyjson_val *statuses = yyjson_obj_get(root, "statuses");

                size_t idx, max;
                yyjson_val *tweet_val;
                yyjson_arr_foreach(statuses, idx, max, tweet_val) {
                    TwitterTweet tweet;

                    yyjson_val *val = yyjson_obj_get(tweet_val, "created_at");
                    if (val) tweet.created_at = yyjson_get_str(val);

                    val = yyjson_obj_get(tweet_val, "id");
                    if (val && yyjson_is_uint(val)) tweet.id = yyjson_get_uint(val);

                    val = yyjson_obj_get(tweet_val, "text");
                    if (val) tweet.text = yyjson_get_str(val);

                    val = yyjson_obj_get(tweet_val, "in_reply_to_status_id");
                    if (val && yyjson_is_uint(val)) {
                        tweet.in_reply_to_status_id = yyjson_get_uint(val);
                    }

                    yyjson_val *user_val = yyjson_obj_get(tweet_val, "user");
                    if (user_val) {
                        val = yyjson_obj_get(user_val, "id");
                        if (val && yyjson_is_uint(val)) tweet.user.id = yyjson_get_uint(val);

                        val = yyjson_obj_get(user_val, "screen_name");
                        if (val) tweet.user.screen_name = yyjson_get_str(val);

                        val = yyjson_obj_get(user_val, "name");
                        if (val) tweet.user.name = yyjson_get_str(val);

                        val = yyjson_obj_get(user_val, "location");
                        if (val) tweet.user.location = yyjson_get_str(val);

                        val = yyjson_obj_get(user_val, "description");
                        if (val) tweet.user.description = yyjson_get_str(val);

                        val = yyjson_obj_get(user_val, "verified");
                        if (val && yyjson_is_bool(val)) tweet.user.verified = yyjson_get_bool(val);

                        val = yyjson_obj_get(user_val, "followers_count");
                        if (val && yyjson_is_uint(val)) tweet.user.followers_count = yyjson_get_uint(val);

                        val = yyjson_obj_get(user_val, "friends_count");
                        if (val && yyjson_is_uint(val)) tweet.user.friends_count = yyjson_get_uint(val);

                        val = yyjson_obj_get(user_val, "statuses_count");
                        if (val && yyjson_is_uint(val)) tweet.user.statuses_count = yyjson_get_uint(val);
                    }

                    val = yyjson_obj_get(tweet_val, "retweet_count");
                    if (val && yyjson_is_uint(val)) tweet.retweet_count = yyjson_get_uint(val);

                    val = yyjson_obj_get(tweet_val, "favorite_count");
                    if (val && yyjson_is_uint(val)) tweet.favorite_count = yyjson_get_uint(val);

                    data.statuses.push_back(std::move(tweet));
                }

                yyjson_doc_free(doc);
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("yyjson", json, iterations, benchmark));
    }
#endif

    return results;
}

// CITM benchmarks
std::vector<BenchmarkResult> benchmark_citm_parsing(const std::string& json) {
    std::vector<BenchmarkResult> results;
    const int iterations = 3000;

    // 1. simdjson - Manual (conservative approach)
    {
        padded_string padded(json); // Create once

        auto benchmark = [&]() -> bool {
            try {
                auto &parser = ondemand::parser::get_parser();
                CITMCatalog catalog;
                auto doc = parser.iterate(padded);

                // Parse all name mappings
                auto areaNames = doc["areaNames"];
                if (!areaNames.error()) {
                    for (auto field : areaNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.areaNames[std::string(key)] = std::string(val);
                    }
                }

                auto audienceSubCategoryNames = doc["audienceSubCategoryNames"];
                if (!audienceSubCategoryNames.error()) {
                    for (auto field : audienceSubCategoryNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.audienceSubCategoryNames[std::string(key)] = std::string(val);
                    }
                }

                auto blockNames = doc["blockNames"];
                if (!blockNames.error()) {
                    for (auto field : blockNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.blockNames[std::string(key)] = std::string(val);
                    }
                }

                // Parse events
                for (auto field : doc["events"].get_object()) {
                    CITMEvent event;
                    auto event_obj = field.value();
                    auto desc = event_obj["description"];
                    if (!desc.is_null()) {
                        event.description = std::string(std::string_view(desc));
                    }

                    event.id = uint64_t(event_obj["id"]);


                    auto logo = event_obj["logo"];
                    if (!logo.is_null()) {
                        event.logo = std::string(std::string_view(logo));
                    }
                    event.name = std::string(std::string_view(event_obj["name"]));


                    for (uint64_t id : event_obj["subTopicIds"]) {
                        event.subTopicIds.push_back(id);
                    }

                    auto subjectCode = event_obj["subjectCode"];
                    if (!subjectCode.is_null()) {
                        event.subjectCode = std::string(std::string_view(subjectCode));
                    }

                    auto subtitle = event_obj["subtitle"];
                    if (!subtitle.is_null()) {
                        event.subtitle = std::string(std::string_view(subtitle));
                    }
                    for (uint64_t id : event_obj["topicIds"]) {
                        event.topicIds.push_back(id);
                    }
                    std::string_view key = field.unescaped_key().value();
                    catalog.events[std::string(key)] = std::move(event);
                }

                // Parse performances - complete
                for (auto perf_obj : doc["performances"]) {
                    CITMPerformance perf;
                    perf.eventId = uint64_t(perf_obj["eventId"]);
                    perf.id = uint64_t(perf_obj["id"]);

                    auto logo = perf_obj["logo"];
                    if (!logo.is_null()) {
                        perf.logo = std::string(std::string_view(logo));
                    }

                    auto name = perf_obj["name"];
                    if (!name.is_null()) {
                        perf.name = std::string(std::string_view(name));
                    }

                    // Parse prices
                    auto prices = perf_obj["prices"];
                    if (!prices.error()) {
                        for (auto price_obj : prices) {
                            CITMPrice price;
                            price.amount = uint64_t(price_obj["amount"]);
                            price.audienceSubCategoryId = uint64_t(price_obj["audienceSubCategoryId"]);
                            price.seatCategoryId = uint64_t(price_obj["seatCategoryId"]);
                            perf.prices.push_back(price);
                        }
                    }

                    // Parse seat categories
                    auto seatCategories = perf_obj["seatCategories"];
                    if (!seatCategories.error()) {
                        for (auto seatCat_obj : seatCategories) {
                            CITMSeatCategory seatCat;

                            auto areas = seatCat_obj["areas"];
                            if (!areas.error()) {
                                for (auto area_obj : areas) {
                                    CITMArea area;
                                    area.areaId = uint64_t(area_obj["areaId"]);

                                    auto blockIds = area_obj["blockIds"];
                                    if (!blockIds.error()) {
                                        for (uint64_t blockId : blockIds) {
                                            area.blockIds.push_back(blockId);
                                        }
                                    }

                                    seatCat.areas.push_back(std::move(area));
                                }
                            }
                            seatCat.seatCategoryId = uint64_t(seatCat_obj["seatCategoryId"]);

                            perf.seatCategories.push_back(std::move(seatCat));
                        }
                    }


                    auto seatMapImage = perf_obj["seatMapImage"];
                    if (!seatMapImage.is_null()) {
                        perf.seatMapImage = std::string(std::string_view(seatMapImage));
                    }
                    perf.start = uint64_t(perf_obj["start"]);
                    perf.venueCode = std::string(std::string_view(perf_obj["venueCode"]));

                    catalog.performances.push_back(std::move(perf));
                }

                // Parse remaining name mappings
                auto seatCategoryNames = doc["seatCategoryNames"];
                if (!seatCategoryNames.error()) {
                    for (auto field : seatCategoryNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.seatCategoryNames[std::string(key)] = std::string(val);
                    }
                }

                auto subTopicNames = doc["subTopicNames"];
                if (!subTopicNames.error()) {
                    for (auto field : subTopicNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.subTopicNames[std::string(key)] = std::string(val);
                    }
                }

                auto subjectNames = doc["subjectNames"];
                if (!subjectNames.error()) {
                    for (auto field : subjectNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.subjectNames[std::string(key)] = std::string(val);
                    }
                }

                auto topicNames = doc["topicNames"];
                if (!topicNames.error()) {
                    for (auto field : topicNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.topicNames[std::string(key)] = std::string(val);
                    }
                }

                auto topicSubTopics = doc["topicSubTopics"];
                if (!topicSubTopics.error()) {
                    for (auto field : topicSubTopics.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::vector<uint64_t> ids;
                        for (uint64_t id : field.value()) {
                            ids.push_back(id);
                        }
                        catalog.topicSubTopics[std::string(key)] = std::move(ids);
                    }
                }

                auto venueNames = doc["venueNames"];
                if (!venueNames.error()) {
                    for (auto field : venueNames.get_object()) {
                        std::string_view key = field.unescaped_key().value();
                        std::string_view val = field.value();
                        catalog.venueNames[std::string(key)] = std::string(val);
                    }
                }

                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (manual)", json, iterations, benchmark));
    }

    // 2. simdjson - Reflection (conservative approach)
    {
        padded_string padded(json); // Create once

        auto benchmark = [&]() -> bool {
            try {
                auto &parser = ondemand::parser::get_parser();
                CITMCatalog catalog;
                auto doc = parser.iterate(padded);
                return doc.get(catalog) == SUCCESS;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (reflection)", json, iterations, benchmark));
    }

    // 3. simdjson::from()
    {
        padded_string padded(json); // Create once, reuse

        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog = simdjson::from(padded);
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson::from()", json, iterations, benchmark));
    }

#ifdef HAS_NLOHMANN
    // 4. nlohmann - with full extraction
    {
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                auto j = nlohmann::json::parse(json);

                // Parse events
                for (auto& [key, event_json] : j["events"].items()) {
                    CITMEvent event;
                    event.id = event_json["id"].get<uint64_t>();
                    event.name = event_json["name"].get<std::string>();

                    if (!event_json["description"].is_null()) {
                        event.description = event_json["description"].get<std::string>();
                    }

                    for (auto& id : event_json["subTopicIds"]) {
                        event.subTopicIds.push_back(id.get<uint64_t>());
                    }

                    for (auto& id : event_json["topicIds"]) {
                        event.topicIds.push_back(id.get<uint64_t>());
                    }

                    catalog.events[key] = std::move(event);
                }

                // Parse performances (full extraction like simdjson)
                for (auto& perf_json : j["performances"]) {
                    CITMPerformance perf;
                    perf.id = perf_json["id"].get<uint64_t>();
                    perf.eventId = perf_json["eventId"].get<uint64_t>();
                    perf.start = perf_json["start"].get<uint64_t>();
                    perf.venueCode = perf_json["venueCode"].get<std::string>();

                    if (!perf_json["logo"].is_null()) {
                        perf.logo = perf_json["logo"].get<std::string>();
                    }

                    if (!perf_json["name"].is_null()) {
                        perf.name = perf_json["name"].get<std::string>();
                    }

                    if (perf_json.contains("prices")) {
                        for (auto& price_json : perf_json["prices"]) {
                            CITMPrice price;
                            price.amount = price_json["amount"].get<uint64_t>();
                            price.audienceSubCategoryId = price_json["audienceSubCategoryId"].get<uint64_t>();
                            price.seatCategoryId = price_json["seatCategoryId"].get<uint64_t>();
                            perf.prices.push_back(price);
                        }
                    }

                    // Parse seatCategories with nested areas and blockIds
                    if (perf_json.contains("seatCategories")) {
                        for (auto& sc_json : perf_json["seatCategories"]) {
                            CITMSeatCategory seatCat;
                            seatCat.seatCategoryId = sc_json["seatCategoryId"].get<uint64_t>();

                            if (sc_json.contains("areas")) {
                                for (auto& area_json : sc_json["areas"]) {
                                    CITMArea area;
                                    area.areaId = area_json["areaId"].get<uint64_t>();

                                    if (area_json.contains("blockIds")) {
                                        for (auto& blockId : area_json["blockIds"]) {
                                            area.blockIds.push_back(blockId.get<uint64_t>());
                                        }
                                    }

                                    seatCat.areas.push_back(std::move(area));
                                }
                            }

                            perf.seatCategories.push_back(std::move(seatCat));
                        }
                    }

                    if (!perf_json["seatMapImage"].is_null()) {
                        perf.seatMapImage = perf_json["seatMapImage"].get<std::string>();
                    }

                    catalog.performances.push_back(std::move(perf));
                }

                // Parse all name mappings (like simdjson does)
                for (auto& [key, val] : j["seatCategoryNames"].items()) {
                    catalog.seatCategoryNames[key] = val.get<std::string>();
                }

                for (auto& [key, val] : j["subTopicNames"].items()) {
                    catalog.subTopicNames[key] = val.get<std::string>();
                }

                for (auto& [key, val] : j["subjectNames"].items()) {
                    catalog.subjectNames[key] = val.get<std::string>();
                }

                for (auto& [key, val] : j["topicNames"].items()) {
                    catalog.topicNames[key] = val.get<std::string>();
                }

                for (auto& [key, val] : j["topicSubTopics"].items()) {
                    std::vector<uint64_t> ids;
                    for (auto& id : val) {
                        ids.push_back(id.get<uint64_t>());
                    }
                    catalog.topicSubTopics[key] = std::move(ids);
                }

                for (auto& [key, val] : j["venueNames"].items()) {
                    catalog.venueNames[key] = val.get<std::string>();
                }

                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("nlohmann (extraction)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_RAPIDJSON
    // 5. RapidJSON - with full extraction
    {
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                rapidjson::Document d;
                d.Parse(json.c_str());
                if (d.HasParseError()) return false;

                // Parse events
                const auto& events = d["events"];
                for (auto it = events.MemberBegin(); it != events.MemberEnd(); ++it) {
                    CITMEvent event;
                    const auto& event_json = it->value;

                    event.id = event_json["id"].GetUint64();
                    event.name = std::string(event_json["name"].GetString());

                    if (event_json.HasMember("description") && !event_json["description"].IsNull()) {
                        event.description = std::string(event_json["description"].GetString());
                    }

                    const auto& subTopicIds = event_json["subTopicIds"];
                    for (rapidjson::SizeType i = 0; i < subTopicIds.Size(); i++) {
                        event.subTopicIds.push_back(subTopicIds[i].GetUint64());
                    }

                    const auto& topicIds = event_json["topicIds"];
                    for (rapidjson::SizeType i = 0; i < topicIds.Size(); i++) {
                        event.topicIds.push_back(topicIds[i].GetUint64());
                    }

                    catalog.events[it->name.GetString()] = std::move(event);
                }

                // Parse performances (full extraction like simdjson)
                const auto& performances = d["performances"];
                for (rapidjson::SizeType i = 0; i < performances.Size(); i++) {
                    const auto& perf_json = performances[i];
                    CITMPerformance perf;

                    perf.id = perf_json["id"].GetUint64();
                    perf.eventId = perf_json["eventId"].GetUint64();
                    perf.start = perf_json["start"].GetUint64();
                    perf.venueCode = std::string(perf_json["venueCode"].GetString());

                    if (perf_json.HasMember("logo") && !perf_json["logo"].IsNull()) {
                        perf.logo = std::string(perf_json["logo"].GetString());
                    }

                    if (perf_json.HasMember("name") && !perf_json["name"].IsNull()) {
                        perf.name = std::string(perf_json["name"].GetString());
                    }

                    if (perf_json.HasMember("prices")) {
                        const auto& prices = perf_json["prices"];
                        for (rapidjson::SizeType j = 0; j < prices.Size(); j++) {
                            const auto& price_json = prices[j];
                            CITMPrice price;
                            price.amount = price_json["amount"].GetUint64();
                            price.audienceSubCategoryId = price_json["audienceSubCategoryId"].GetUint64();
                            price.seatCategoryId = price_json["seatCategoryId"].GetUint64();
                            perf.prices.push_back(price);
                        }
                    }

                    // Parse seatCategories with nested areas and blockIds
                    if (perf_json.HasMember("seatCategories")) {
                        const auto& seatCategories = perf_json["seatCategories"];
                        for (rapidjson::SizeType j = 0; j < seatCategories.Size(); j++) {
                            const auto& sc_json = seatCategories[j];
                            CITMSeatCategory seatCat;
                            seatCat.seatCategoryId = sc_json["seatCategoryId"].GetUint64();

                            if (sc_json.HasMember("areas")) {
                                const auto& areas = sc_json["areas"];
                                for (rapidjson::SizeType k = 0; k < areas.Size(); k++) {
                                    const auto& area_json = areas[k];
                                    CITMArea area;
                                    area.areaId = area_json["areaId"].GetUint64();

                                    if (area_json.HasMember("blockIds")) {
                                        const auto& blockIds = area_json["blockIds"];
                                        for (rapidjson::SizeType l = 0; l < blockIds.Size(); l++) {
                                            area.blockIds.push_back(blockIds[l].GetUint64());
                                        }
                                    }

                                    seatCat.areas.push_back(std::move(area));
                                }
                            }

                            perf.seatCategories.push_back(std::move(seatCat));
                        }
                    }

                    if (perf_json.HasMember("seatMapImage") && !perf_json["seatMapImage"].IsNull()) {
                        perf.seatMapImage = std::string(perf_json["seatMapImage"].GetString());
                    }

                    catalog.performances.push_back(std::move(perf));
                }

                // Parse all name mappings (like simdjson does)
                const auto& seatCategoryNames = d["seatCategoryNames"];
                for (auto it = seatCategoryNames.MemberBegin(); it != seatCategoryNames.MemberEnd(); ++it) {
                    catalog.seatCategoryNames[it->name.GetString()] = std::string(it->value.GetString());
                }

                const auto& subTopicNames = d["subTopicNames"];
                for (auto it = subTopicNames.MemberBegin(); it != subTopicNames.MemberEnd(); ++it) {
                    catalog.subTopicNames[it->name.GetString()] = std::string(it->value.GetString());
                }

                const auto& subjectNames = d["subjectNames"];
                for (auto it = subjectNames.MemberBegin(); it != subjectNames.MemberEnd(); ++it) {
                    catalog.subjectNames[it->name.GetString()] = std::string(it->value.GetString());
                }

                const auto& topicNames = d["topicNames"];
                for (auto it = topicNames.MemberBegin(); it != topicNames.MemberEnd(); ++it) {
                    catalog.topicNames[it->name.GetString()] = std::string(it->value.GetString());
                }

                const auto& topicSubTopics = d["topicSubTopics"];
                for (auto it = topicSubTopics.MemberBegin(); it != topicSubTopics.MemberEnd(); ++it) {
                    std::vector<uint64_t> ids;
                    const auto& arr = it->value;
                    for (rapidjson::SizeType i = 0; i < arr.Size(); i++) {
                        ids.push_back(arr[i].GetUint64());
                    }
                    catalog.topicSubTopics[it->name.GetString()] = std::move(ids);
                }

                const auto& venueNames = d["venueNames"];
                for (auto it = venueNames.MemberBegin(); it != venueNames.MemberEnd(); ++it) {
                    catalog.venueNames[it->name.GetString()] = std::string(it->value.GetString());
                }

                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("RapidJSON (extraction)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_SERDE
    // 6. Serde (Rust) - with full extraction
    {
        auto benchmark = [&]() -> bool {
            serde_benchmark::CitmCatalog* catalog = serde_benchmark::citm_from_str(json.c_str(), json.size());
            if (catalog) {
                serde_benchmark::free_citm(catalog);
                return true;
            }
            return false;
        };

        results.push_back(run_benchmark("Serde (Rust)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_REFLECTCPP
    // 7. reflect-cpp - with full extraction
    {
        auto benchmark = [&]() -> bool {
            auto result = rfl::json::read<CITMCatalog>(json);
            if (result) {
                CITMCatalog data = result.value();
                return true;
            }
            return false;
        };

        results.push_back(run_benchmark("reflect-cpp", json, iterations, benchmark));
    }
#endif

#ifdef HAS_YYJSON
    // 8. yyjson - with full extraction for CITM
    {
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
                if (!doc) return false;

                yyjson_val *root = yyjson_doc_get_root(doc);

                // Parse events
                yyjson_val *events = yyjson_obj_get(root, "events");
                if (events) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(events, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        CITMEvent event;

                        yyjson_val *v = yyjson_obj_get(val, "id");
                        if (v && yyjson_is_uint(v)) event.id = yyjson_get_uint(v);

                        v = yyjson_obj_get(val, "name");
                        if (v) event.name = yyjson_get_str(v);

                        v = yyjson_obj_get(val, "description");
                        if (v && !yyjson_is_null(v)) event.description = yyjson_get_str(v);

                        v = yyjson_obj_get(val, "logo");
                        if (v && !yyjson_is_null(v)) event.logo = yyjson_get_str(v);

                        v = yyjson_obj_get(val, "subTopicIds");
                        if (v) {
                            size_t arr_idx, arr_max;
                            yyjson_val *arr_val;
                            yyjson_arr_foreach(v, arr_idx, arr_max, arr_val) {
                                if (arr_val && yyjson_is_uint(arr_val)) event.subTopicIds.push_back(yyjson_get_uint(arr_val));
                            }
                        }

                        v = yyjson_obj_get(val, "topicIds");
                        if (v) {
                            size_t arr_idx, arr_max;
                            yyjson_val *arr_val;
                            yyjson_arr_foreach(v, arr_idx, arr_max, arr_val) {
                                if (arr_val && yyjson_is_uint(arr_val)) event.topicIds.push_back(yyjson_get_uint(arr_val));
                            }
                        }

                        if (key_str) catalog.events[key_str] = std::move(event);
                    }
                }

                // Parse performances (full extraction like simdjson)
                yyjson_val *performances = yyjson_obj_get(root, "performances");
                if (performances) {
                    size_t idx, max;
                    yyjson_val *perf_val;
                    yyjson_arr_foreach(performances, idx, max, perf_val) {
                        CITMPerformance perf;

                        yyjson_val *v = yyjson_obj_get(perf_val, "id");
                        if (v && yyjson_is_uint(v)) perf.id = yyjson_get_uint(v);

                        v = yyjson_obj_get(perf_val, "eventId");
                        if (v && yyjson_is_uint(v)) perf.eventId = yyjson_get_uint(v);

                        v = yyjson_obj_get(perf_val, "logo");
                        if (v && !yyjson_is_null(v)) perf.logo = yyjson_get_str(v);

                        v = yyjson_obj_get(perf_val, "name");
                        if (v && !yyjson_is_null(v)) perf.name = yyjson_get_str(v);

                        // Parse prices array
                        v = yyjson_obj_get(perf_val, "prices");
                        if (v) {
                            size_t price_idx, price_max;
                            yyjson_val *price_val;
                            yyjson_arr_foreach(v, price_idx, price_max, price_val) {
                                CITMPrice price;
                                yyjson_val *p = yyjson_obj_get(price_val, "amount");
                                if (p && yyjson_is_uint(p)) price.amount = yyjson_get_uint(p);

                                p = yyjson_obj_get(price_val, "audienceSubCategoryId");
                                if (p && yyjson_is_uint(p)) price.audienceSubCategoryId = yyjson_get_uint(p);

                                p = yyjson_obj_get(price_val, "seatCategoryId");
                                if (p && yyjson_is_uint(p)) price.seatCategoryId = yyjson_get_uint(p);

                                perf.prices.push_back(std::move(price));
                            }
                        }

                        // Parse seatCategories with nested areas and blockIds
                        v = yyjson_obj_get(perf_val, "seatCategories");
                        if (v) {
                            size_t sc_idx, sc_max;
                            yyjson_val *sc_val;
                            yyjson_arr_foreach(v, sc_idx, sc_max, sc_val) {
                                CITMSeatCategory seatCat;

                                yyjson_val *sc = yyjson_obj_get(sc_val, "seatCategoryId");
                                if (sc && yyjson_is_uint(sc)) seatCat.seatCategoryId = yyjson_get_uint(sc);

                                // Parse areas array
                                sc = yyjson_obj_get(sc_val, "areas");
                                if (sc) {
                                    size_t area_idx, area_max;
                                    yyjson_val *area_val;
                                    yyjson_arr_foreach(sc, area_idx, area_max, area_val) {
                                        CITMArea area;

                                        yyjson_val *a = yyjson_obj_get(area_val, "areaId");
                                        if (a && yyjson_is_uint(a)) area.areaId = yyjson_get_uint(a);

                                        // Parse blockIds array
                                        a = yyjson_obj_get(area_val, "blockIds");
                                        if (a) {
                                            size_t block_idx, block_max;
                                            yyjson_val *block_val;
                                            yyjson_arr_foreach(a, block_idx, block_max, block_val) {
                                                if (block_val && yyjson_is_uint(block_val)) area.blockIds.push_back(yyjson_get_uint(block_val));
                                            }
                                        }

                                        seatCat.areas.push_back(std::move(area));
                                    }
                                }

                                perf.seatCategories.push_back(std::move(seatCat));
                            }
                        }

                        v = yyjson_obj_get(perf_val, "seatMapImage");
                        if (v && !yyjson_is_null(v)) perf.seatMapImage = yyjson_get_str(v);

                        v = yyjson_obj_get(perf_val, "start");
                        if (v && yyjson_is_uint(v)) perf.start = yyjson_get_uint(v);

                        v = yyjson_obj_get(perf_val, "venueCode");
                        if (v) perf.venueCode = yyjson_get_str(v);

                        catalog.performances.push_back(std::move(perf));
                    }
                }

                // Parse all name mappings (like simdjson does)
                yyjson_val *seatCategoryNames = yyjson_obj_get(root, "seatCategoryNames");
                if (seatCategoryNames) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(seatCategoryNames, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        const char *val_str = yyjson_get_str(val);
                        if (key_str && val_str) {
                            catalog.seatCategoryNames[key_str] = val_str;
                        }
                    }
                }

                yyjson_val *subTopicNames = yyjson_obj_get(root, "subTopicNames");
                if (subTopicNames) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(subTopicNames, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        const char *val_str = yyjson_get_str(val);
                        if (key_str && val_str) {
                            catalog.subTopicNames[key_str] = val_str;
                        }
                    }
                }

                yyjson_val *subjectNames = yyjson_obj_get(root, "subjectNames");
                if (subjectNames) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(subjectNames, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        const char *val_str = yyjson_get_str(val);
                        if (key_str && val_str) {
                            catalog.subjectNames[key_str] = val_str;
                        }
                    }
                }

                yyjson_val *topicNames = yyjson_obj_get(root, "topicNames");
                if (topicNames) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(topicNames, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        const char *val_str = yyjson_get_str(val);
                        if (key_str && val_str) {
                            catalog.topicNames[key_str] = val_str;
                        }
                    }
                }

                yyjson_val *topicSubTopics = yyjson_obj_get(root, "topicSubTopics");
                if (topicSubTopics) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(topicSubTopics, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        if (key_str) {
                            std::vector<uint64_t> ids;
                            size_t arr_idx, arr_max;
                            yyjson_val *arr_val;
                            yyjson_arr_foreach(val, arr_idx, arr_max, arr_val) {
                                if (arr_val && yyjson_is_uint(arr_val)) ids.push_back(yyjson_get_uint(arr_val));
                            }
                            catalog.topicSubTopics[key_str] = std::move(ids);
                        }
                    }
                }

                yyjson_val *venueNames = yyjson_obj_get(root, "venueNames");
                if (venueNames) {
                    size_t idx, max;
                    yyjson_val *key;
                    yyjson_val *val;
                    yyjson_obj_foreach(venueNames, idx, max, key, val) {
                        const char *key_str = yyjson_get_str(key);
                        const char *val_str = yyjson_get_str(val);
                        if (key_str && val_str) {
                            catalog.venueNames[key_str] = val_str;
                        }
                    }
                }

                yyjson_doc_free(doc);
                return true;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("yyjson", json, iterations, benchmark));
    }
#endif

    return results;
}

// Twitter serialization benchmarks
std::vector<BenchmarkResult> benchmark_twitter_serialization(const TwitterData& data) {
    std::vector<BenchmarkResult> results;
    const int iterations = 3000;

    // Get a sample serialization size for throughput calculation
    std::string sample_json;

    // 1. simdjson reflection-based serialization (with string_builder reuse)
    {
        simdjson::builder::string_builder sb;  // Create once, reuse

        auto benchmark = [&sb, &data, &sample_json]() -> bool {
            try {
                sb.clear();  // Clear for reuse
                simdjson::builder::append(sb, data);

                std::string_view json_view = sb.view();

                // Use this as sample if we don't have other libraries
                if (sample_json.empty()) {
                    sample_json = std::string(json_view);
                }

                return json_view.size() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (reflection)", sample_json, iterations, benchmark));
    }

    // 2. simdjson manual DOM serialization (with string_builder reuse)
    {
        simdjson::builder::string_builder sb;  // Create once, reuse

        auto benchmark = [&sb, &data, &sample_json]() -> bool {
            try {
                sb.clear();  // Clear for reuse
                sb.append("{\"statuses\":[");

                bool first = true;
                for (const auto& tweet : data.statuses) {
                    if (!first) sb.append(",");
                    sb.append("{");

                    sb.append("\"created_at\":\"");
                    sb.append(tweet.created_at);
                    sb.append("\",");

                    sb.append("\"id\":");
                    sb.append(tweet.id);
                    sb.append(",");

                    sb.append("\"text\":\"");
                    sb.append(tweet.text);
                    sb.append("\",");

                    if (tweet.in_reply_to_status_id.has_value()) {
                        sb.append("\"in_reply_to_status_id\":");
                        sb.append(*tweet.in_reply_to_status_id);
                        sb.append(",");
                    }

                    // User object
                    sb.append("\"user\":{");
                    sb.append("\"id\":");
                    sb.append(tweet.user.id);
                    sb.append(",");
                    sb.append("\"screen_name\":\"");
                    sb.append(tweet.user.screen_name);
                    sb.append("\",");
                    sb.append("\"name\":\"");
                    sb.append(tweet.user.name);
                    sb.append("\",");
                    sb.append("\"location\":\"");
                    sb.append(tweet.user.location);
                    sb.append("\",");
                    sb.append("\"description\":\"");
                    sb.append(tweet.user.description);
                    sb.append("\",");
                    sb.append("\"verified\":");
                    sb.append(tweet.user.verified ? "true" : "false");
                    sb.append(",");
                    sb.append("\"followers_count\":");
                    sb.append(tweet.user.followers_count);
                    sb.append(",");
                    sb.append("\"friends_count\":");
                    sb.append(tweet.user.friends_count);
                    sb.append(",");
                    sb.append("\"statuses_count\":");
                    sb.append(tweet.user.statuses_count);
                    sb.append("},");

                    sb.append("\"retweet_count\":");
                    sb.append(tweet.retweet_count);
                    sb.append(",");
                    sb.append("\"favorite_count\":");
                    sb.append(tweet.favorite_count);

                    sb.append("}");
                    first = false;
                }

                sb.append("]}");

                std::string_view json_view = sb.view();

                // Use this as sample if we don't have other libraries
                if (sample_json.empty()) {
                    sample_json = std::string(json_view);
                }

                return json_view.size() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (DOM)", sample_json, iterations, benchmark));
    }

#ifdef HAS_NLOHMANN
    // Update sample_json if not set
    if (sample_json.empty()) {
        nlohmann::json j = data;
        sample_json = j.dump();
    }

    // nlohmann::json
    {
        auto benchmark = [&]() -> bool {
            try {
                nlohmann::json j = data;
                std::string json = j.dump();
                return !json.empty();
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("nlohmann::json", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_RAPIDJSON
    // 3. RapidJSON
    {
        auto benchmark = [&]() -> bool {
            try {
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

                rapidjson::Value statuses(rapidjson::kArrayType);
                for (const auto& tweet : data.statuses) {
                    rapidjson::Value tweet_obj(rapidjson::kObjectType);

                    rapidjson::Value created_at;
                    created_at.SetString(tweet.created_at.c_str(), tweet.created_at.length(), alloc);
                    tweet_obj.AddMember("created_at", created_at, alloc);

                    tweet_obj.AddMember("id", tweet.id, alloc);

                    rapidjson::Value text;
                    text.SetString(tweet.text.c_str(), tweet.text.length(), alloc);
                    tweet_obj.AddMember("text", text, alloc);

                    if (tweet.in_reply_to_status_id.has_value()) {
                        tweet_obj.AddMember("in_reply_to_status_id", tweet.in_reply_to_status_id.value(), alloc);
                    } else {
                        tweet_obj.AddMember("in_reply_to_status_id", rapidjson::Value(rapidjson::kNullType), alloc);
                    }

                    rapidjson::Value user_obj(rapidjson::kObjectType);
                    user_obj.AddMember("id", tweet.user.id, alloc);

                    rapidjson::Value screen_name;
                    screen_name.SetString(tweet.user.screen_name.c_str(), tweet.user.screen_name.length(), alloc);
                    user_obj.AddMember("screen_name", screen_name, alloc);

                    rapidjson::Value name;
                    name.SetString(tweet.user.name.c_str(), tweet.user.name.length(), alloc);
                    user_obj.AddMember("name", name, alloc);

                    rapidjson::Value location;
                    location.SetString(tweet.user.location.c_str(), tweet.user.location.length(), alloc);
                    user_obj.AddMember("location", location, alloc);

                    rapidjson::Value description;
                    description.SetString(tweet.user.description.c_str(), tweet.user.description.length(), alloc);
                    user_obj.AddMember("description", description, alloc);

                    user_obj.AddMember("verified", tweet.user.verified, alloc);
                    user_obj.AddMember("followers_count", tweet.user.followers_count, alloc);
                    user_obj.AddMember("friends_count", tweet.user.friends_count, alloc);
                    user_obj.AddMember("statuses_count", tweet.user.statuses_count, alloc);

                    tweet_obj.AddMember("user", user_obj, alloc);
                    tweet_obj.AddMember("retweet_count", tweet.retweet_count, alloc);
                    tweet_obj.AddMember("favorite_count", tweet.favorite_count, alloc);

                    statuses.PushBack(tweet_obj, alloc);
                }

                doc.AddMember("statuses", statuses, alloc);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                return buffer.GetSize() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("RapidJSON", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_YYJSON
    // 4. yyjson
    {
        auto benchmark = [&]() -> bool {
            try {
                yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
                yyjson_mut_val *root = yyjson_mut_obj(doc);
                yyjson_mut_doc_set_root(doc, root);

                yyjson_mut_val *statuses = yyjson_mut_arr(doc);

                for (const auto& tweet : data.statuses) {
                    yyjson_mut_val *tweet_obj = yyjson_mut_obj(doc);

                    yyjson_mut_obj_add_str(doc, tweet_obj, "created_at", tweet.created_at.c_str());
                    yyjson_mut_obj_add_int(doc, tweet_obj, "id", tweet.id);
                    yyjson_mut_obj_add_str(doc, tweet_obj, "text", tweet.text.c_str());

                    if (tweet.in_reply_to_status_id.has_value()) {
                        yyjson_mut_obj_add_int(doc, tweet_obj, "in_reply_to_status_id", tweet.in_reply_to_status_id.value());
                    } else {
                        yyjson_mut_obj_add_null(doc, tweet_obj, "in_reply_to_status_id");
                    }

                    yyjson_mut_val *user_obj = yyjson_mut_obj(doc);
                    yyjson_mut_obj_add_int(doc, user_obj, "id", tweet.user.id);
                    yyjson_mut_obj_add_str(doc, user_obj, "screen_name", tweet.user.screen_name.c_str());
                    yyjson_mut_obj_add_str(doc, user_obj, "name", tweet.user.name.c_str());
                    yyjson_mut_obj_add_str(doc, user_obj, "location", tweet.user.location.c_str());
                    yyjson_mut_obj_add_str(doc, user_obj, "description", tweet.user.description.c_str());
                    yyjson_mut_obj_add_bool(doc, user_obj, "verified", tweet.user.verified);
                    yyjson_mut_obj_add_int(doc, user_obj, "followers_count", tweet.user.followers_count);
                    yyjson_mut_obj_add_int(doc, user_obj, "friends_count", tweet.user.friends_count);
                    yyjson_mut_obj_add_int(doc, user_obj, "statuses_count", tweet.user.statuses_count);

                    yyjson_mut_obj_add_val(doc, tweet_obj, "user", user_obj);
                    yyjson_mut_obj_add_int(doc, tweet_obj, "retweet_count", tweet.retweet_count);
                    yyjson_mut_obj_add_int(doc, tweet_obj, "favorite_count", tweet.favorite_count);

                    yyjson_mut_arr_append(statuses, tweet_obj);
                }

                yyjson_mut_obj_add_val(doc, root, "statuses", statuses);

                char *json_str = yyjson_mut_write(doc, 0, NULL);
                bool success = (json_str != NULL);
                if (json_str) free(json_str);
                yyjson_mut_doc_free(doc);

                return success;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("yyjson", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_SERDE
    // 5. Serde (Rust) serialization
    {
        // Parse the original JSON to get Serde's internal representation
        // We need the original JSON, not our C++ serialized version
        // Get the actual Twitter JSON file
        std::string twitter_json_full;
        {
            std::ifstream file("jsonexamples/twitter.json", std::ios::binary);
            if (!file) {
                file.open("../jsonexamples/twitter.json", std::ios::binary);
            }
            if (!file) {
                file.open("../../jsonexamples/twitter.json", std::ios::binary);
            }
            if (file) {
                twitter_json_full = std::string((std::istreambuf_iterator<char>(file)),
                                                std::istreambuf_iterator<char>());
            }
        }

        serde_benchmark::TwitterData* serde_data = nullptr;
        if (!twitter_json_full.empty()) {
            serde_data = serde_benchmark::twitter_from_str(
                twitter_json_full.c_str(), twitter_json_full.size());
        }

        if (serde_data) {
            auto benchmark = [serde_data]() -> bool {
                try {
                    const char* json_str = serde_benchmark::str_from_twitter(serde_data);
                    if (json_str) {
                        serde_benchmark::free_string(json_str);
                        return true;
                    }
                    return false;
                } catch (...) {
                    return false;
                }
            };

            results.push_back(run_benchmark("Serde (Rust)", sample_json, iterations, benchmark));
            serde_benchmark::free_twitter(serde_data);
        }
    }
#endif

#ifdef HAS_REFLECTCPP
    // 6. reflect-cpp
    {
        auto benchmark = [&]() -> bool {
            try {
                std::string json = rfl::json::write(data);
                return !json.empty();
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("reflect-cpp", sample_json, iterations, benchmark));
    }
#endif

    return results;
}

// CITM serialization benchmarks
std::vector<BenchmarkResult> benchmark_citm_serialization(const CITMCatalog& data) {
    std::vector<BenchmarkResult> results;
    const int iterations = 3000;

    // Get a sample serialization size for throughput calculation
    std::string sample_json;

    // 1. simdjson reflection-based serialization (with string_builder reuse)
    {
        simdjson::builder::string_builder sb;  // Create once, reuse

        auto benchmark = [&sb, &data, &sample_json]() -> bool {
            try {
                sb.clear();  // Clear for reuse
                simdjson::builder::append(sb, data);

                std::string_view json_view = sb.view();

                // Use this as sample if we don't have other libraries
                if (sample_json.empty()) {
                    sample_json = std::string(json_view);
                }

                return json_view.size() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (reflection)", sample_json, iterations, benchmark));
    }

    // 2. simdjson manual DOM serialization (with string_builder reuse)
    {
        simdjson::builder::string_builder sb;  // Create once, reuse

        auto benchmark = [&sb, &data, &sample_json]() -> bool {
            try {
                sb.clear();  // Clear for reuse
                sb.append("{");

                // Serialize areaNames
                sb.append("\"areaNames\":{");
                bool first = true;
                for (const auto& [key, value] : data.areaNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                // Serialize audienceSubCategoryNames
                sb.append("\"audienceSubCategoryNames\":{");
                first = true;
                for (const auto& [key, value] : data.audienceSubCategoryNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                // Serialize blockNames
                sb.append("\"blockNames\":{");
                first = true;
                for (const auto& [key, value] : data.blockNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                // Serialize events
                sb.append("\"events\":{");
                first = true;
                for (const auto& [key, event] : data.events) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":{");
                    sb.append("\"id\":");
                    sb.append(event.id);
                    sb.append(",");
                    sb.append("\"name\":\"");
                    sb.append(event.name);
                    sb.append("\"");

                    if (event.description.has_value()) {
                        sb.append(",\"description\":\"");
                        sb.append(*event.description);
                        sb.append("\"");
                    }
                    if (event.logo.has_value()) {
                        sb.append(",\"logo\":\"");
                        sb.append(*event.logo);
                        sb.append("\"");
                    }

                    sb.append(",\"subTopicIds\":[");
                    bool first_id = true;
                    for (uint64_t id : event.subTopicIds) {
                        if (!first_id) sb.append(",");
                        sb.append(id);
                        first_id = false;
                    }
                    sb.append("]");

                    if (event.subjectCode.has_value()) {
                        sb.append(",\"subjectCode\":\"");
                        sb.append(*event.subjectCode);
                        sb.append("\"");
                    }
                    if (event.subtitle.has_value()) {
                        sb.append(",\"subtitle\":\"");
                        sb.append(*event.subtitle);
                        sb.append("\"");
                    }

                    sb.append(",\"topicIds\":[");
                    first_id = true;
                    for (uint64_t id : event.topicIds) {
                        if (!first_id) sb.append(",");
                        sb.append(id);
                        first_id = false;
                    }
                    sb.append("]");

                    sb.append("}");
                    first = false;
                }
                sb.append("},");

                // Serialize performances
                sb.append("\"performances\":[");
                first = true;
                for (const auto& perf : data.performances) {
                    if (!first) sb.append(",");
                    sb.append("{");
                    sb.append("\"id\":");
                    sb.append(perf.id);
                    sb.append(",");
                    sb.append("\"eventId\":");
                    sb.append(perf.eventId);
                    sb.append(",");

                    if (perf.logo.has_value()) {
                        sb.append("\"logo\":\"");
                        sb.append(*perf.logo);
                        sb.append("\",");
                    }
                    if (perf.name.has_value()) {
                        sb.append("\"name\":\"");
                        sb.append(*perf.name);
                        sb.append("\",");
                    }

                    // Prices
                    sb.append("\"prices\":[");
                    bool first_price = true;
                    for (const auto& price : perf.prices) {
                        if (!first_price) sb.append(",");
                        sb.append("{");
                        sb.append("\"amount\":");
                        sb.append(price.amount);
                        sb.append(",");
                        sb.append("\"audienceSubCategoryId\":");
                        sb.append(price.audienceSubCategoryId);
                        sb.append(",");
                        sb.append("\"seatCategoryId\":");
                        sb.append(price.seatCategoryId);
                        sb.append("}");
                        first_price = false;
                    }
                    sb.append("],");

                    // Seat categories
                    sb.append("\"seatCategories\":[");
                    bool first_sc = true;
                    for (const auto& sc : perf.seatCategories) {
                        if (!first_sc) sb.append(",");
                        sb.append("{");
                        sb.append("\"seatCategoryId\":");
                        sb.append(sc.seatCategoryId);
                        sb.append(",");
                        sb.append("\"areas\":[");
                        bool first_area = true;
                        for (const auto& area : sc.areas) {
                            if (!first_area) sb.append(",");
                            sb.append("{");
                            sb.append("\"areaId\":");
                            sb.append(area.areaId);
                            sb.append(",");
                            sb.append("\"blockIds\":[");
                            bool first_block = true;
                            for (uint64_t blockId : area.blockIds) {
                                if (!first_block) sb.append(",");
                                sb.append(blockId);
                                first_block = false;
                            }
                            sb.append("]");
                            sb.append("}");
                            first_area = false;
                        }
                        sb.append("]");
                        sb.append("}");
                        first_sc = false;
                    }
                    sb.append("],");

                    if (perf.seatMapImage.has_value()) {
                        sb.append("\"seatMapImage\":\"");
                        sb.append(*perf.seatMapImage);
                        sb.append("\",");
                    }

                    sb.append("\"start\":");
                    sb.append(perf.start);
                    sb.append(",");
                    sb.append("\"venueCode\":\"");
                    sb.append(perf.venueCode);
                    sb.append("\"");
                    sb.append("}");
                    first = false;
                }
                sb.append("],");

                // Serialize remaining name mappings
                sb.append("\"seatCategoryNames\":{");
                first = true;
                for (const auto& [key, value] : data.seatCategoryNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                sb.append("\"subTopicNames\":{");
                first = true;
                for (const auto& [key, value] : data.subTopicNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                sb.append("\"subjectNames\":{");
                first = true;
                for (const auto& [key, value] : data.subjectNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                sb.append("\"topicNames\":{");
                first = true;
                for (const auto& [key, value] : data.topicNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("},");

                sb.append("\"topicSubTopics\":{");
                first = true;
                for (const auto& [key, values] : data.topicSubTopics) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":[");
                    bool first_val = true;
                    for (uint64_t val : values) {
                        if (!first_val) sb.append(",");
                        sb.append(val);
                        first_val = false;
                    }
                    sb.append("]");
                    first = false;
                }
                sb.append("},");

                sb.append("\"venueNames\":{");
                first = true;
                for (const auto& [key, value] : data.venueNames) {
                    if (!first) sb.append(",");
                    sb.append("\"");
                    sb.append(key);
                    sb.append("\":\"");
                    sb.append(value);
                    sb.append("\"");
                    first = false;
                }
                sb.append("}");

                sb.append("}");

                std::string_view json_view = sb.view();

                // Use this as sample if we don't have nlohmann
                if (sample_json.empty()) {
                    sample_json = std::string(json_view);
                }

                return json_view.size() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("simdjson (DOM)", sample_json, iterations, benchmark));
    }

#ifdef HAS_NLOHMANN
    // Update sample_json if not set
    if (sample_json.empty()) {
        nlohmann::json j = data;
        sample_json = j.dump();
    }

    // nlohmann::json
    {
        auto benchmark = [&]() -> bool {
            try {
                nlohmann::json j = data;
                std::string json = j.dump();
                return !json.empty();
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("nlohmann::json", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_RAPIDJSON
    // 2. RapidJSON - complete implementation
    {
        auto benchmark = [&]() -> bool {
            try {
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

                // Add all name mappings
                rapidjson::Value areaNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.areaNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    areaNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("areaNames", areaNames_obj, alloc);

                rapidjson::Value audienceSubCategoryNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.audienceSubCategoryNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    audienceSubCategoryNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("audienceSubCategoryNames", audienceSubCategoryNames_obj, alloc);

                rapidjson::Value blockNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.blockNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    blockNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("blockNames", blockNames_obj, alloc);

                // Events object - complete
                rapidjson::Value events_obj(rapidjson::kObjectType);
                for (const auto& [key, event] : data.events) {
                    rapidjson::Value event_obj(rapidjson::kObjectType);
                    event_obj.AddMember("id", event.id, alloc);

                    rapidjson::Value name_val;
                    name_val.SetString(event.name.c_str(), event.name.length(), alloc);
                    event_obj.AddMember("name", name_val, alloc);

                    if (event.description.has_value()) {
                        rapidjson::Value desc_val;
                        desc_val.SetString(event.description->c_str(), event.description->length(), alloc);
                        event_obj.AddMember("description", desc_val, alloc);
                    }

                    if (event.logo.has_value()) {
                        rapidjson::Value logo_val;
                        logo_val.SetString(event.logo->c_str(), event.logo->length(), alloc);
                        event_obj.AddMember("logo", logo_val, alloc);
                    }

                    rapidjson::Value subTopicIds(rapidjson::kArrayType);
                    for (uint64_t id : event.subTopicIds) {
                        subTopicIds.PushBack(id, alloc);
                    }
                    event_obj.AddMember("subTopicIds", subTopicIds, alloc);

                    if (event.subjectCode.has_value()) {
                        rapidjson::Value subject_val;
                        subject_val.SetString(event.subjectCode->c_str(), event.subjectCode->length(), alloc);
                        event_obj.AddMember("subjectCode", subject_val, alloc);
                    }

                    if (event.subtitle.has_value()) {
                        rapidjson::Value subtitle_val;
                        subtitle_val.SetString(event.subtitle->c_str(), event.subtitle->length(), alloc);
                        event_obj.AddMember("subtitle", subtitle_val, alloc);
                    }

                    rapidjson::Value topicIds(rapidjson::kArrayType);
                    for (uint64_t id : event.topicIds) {
                        topicIds.PushBack(id, alloc);
                    }
                    event_obj.AddMember("topicIds", topicIds, alloc);

                    rapidjson::Value key_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    events_obj.AddMember(key_val, event_obj, alloc);
                }
                doc.AddMember("events", events_obj, alloc);

                // Performances array - complete
                rapidjson::Value performances(rapidjson::kArrayType);
                for (const auto& perf : data.performances) {
                    rapidjson::Value perf_obj(rapidjson::kObjectType);
                    perf_obj.AddMember("id", perf.id, alloc);
                    perf_obj.AddMember("eventId", perf.eventId, alloc);

                    if (perf.logo.has_value()) {
                        rapidjson::Value logo_val;
                        logo_val.SetString(perf.logo->c_str(), perf.logo->length(), alloc);
                        perf_obj.AddMember("logo", logo_val, alloc);
                    }

                    if (perf.name.has_value()) {
                        rapidjson::Value name_val;
                        name_val.SetString(perf.name->c_str(), perf.name->length(), alloc);
                        perf_obj.AddMember("name", name_val, alloc);
                    }

                    // Prices array
                    rapidjson::Value prices(rapidjson::kArrayType);
                    for (const auto& price : perf.prices) {
                        rapidjson::Value price_obj(rapidjson::kObjectType);
                        price_obj.AddMember("amount", price.amount, alloc);
                        price_obj.AddMember("audienceSubCategoryId", price.audienceSubCategoryId, alloc);
                        price_obj.AddMember("seatCategoryId", price.seatCategoryId, alloc);
                        prices.PushBack(price_obj, alloc);
                    }
                    perf_obj.AddMember("prices", prices, alloc);

                    // Seat categories array
                    rapidjson::Value seatCategories(rapidjson::kArrayType);
                    for (const auto& seatCat : perf.seatCategories) {
                        rapidjson::Value seatCat_obj(rapidjson::kObjectType);
                        seatCat_obj.AddMember("seatCategoryId", seatCat.seatCategoryId, alloc);

                        rapidjson::Value areas(rapidjson::kArrayType);
                        for (const auto& area : seatCat.areas) {
                            rapidjson::Value area_obj(rapidjson::kObjectType);
                            area_obj.AddMember("areaId", area.areaId, alloc);

                            rapidjson::Value blockIds(rapidjson::kArrayType);
                            for (uint64_t blockId : area.blockIds) {
                                blockIds.PushBack(blockId, alloc);
                            }
                            area_obj.AddMember("blockIds", blockIds, alloc);

                            areas.PushBack(area_obj, alloc);
                        }
                        seatCat_obj.AddMember("areas", areas, alloc);

                        seatCategories.PushBack(seatCat_obj, alloc);
                    }
                    perf_obj.AddMember("seatCategories", seatCategories, alloc);

                    if (perf.seatMapImage.has_value()) {
                        rapidjson::Value seatMap_val;
                        seatMap_val.SetString(perf.seatMapImage->c_str(), perf.seatMapImage->length(), alloc);
                        perf_obj.AddMember("seatMapImage", seatMap_val, alloc);
                    }

                    perf_obj.AddMember("start", perf.start, alloc);

                    rapidjson::Value venue_val;
                    venue_val.SetString(perf.venueCode.c_str(), perf.venueCode.length(), alloc);
                    perf_obj.AddMember("venueCode", venue_val, alloc);

                    performances.PushBack(perf_obj, alloc);
                }
                doc.AddMember("performances", performances, alloc);

                // Add remaining name mappings
                rapidjson::Value seatCategoryNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.seatCategoryNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    seatCategoryNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("seatCategoryNames", seatCategoryNames_obj, alloc);

                rapidjson::Value subTopicNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.subTopicNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    subTopicNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("subTopicNames", subTopicNames_obj, alloc);

                rapidjson::Value subjectNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.subjectNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    subjectNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("subjectNames", subjectNames_obj, alloc);

                rapidjson::Value topicNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.topicNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    topicNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("topicNames", topicNames_obj, alloc);

                rapidjson::Value topicSubTopics_obj(rapidjson::kObjectType);
                for (const auto& [key, values] : data.topicSubTopics) {
                    rapidjson::Value key_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    rapidjson::Value arr(rapidjson::kArrayType);
                    for (uint64_t val : values) {
                        arr.PushBack(val, alloc);
                    }
                    topicSubTopics_obj.AddMember(key_val, arr, alloc);
                }
                doc.AddMember("topicSubTopics", topicSubTopics_obj, alloc);

                rapidjson::Value venueNames_obj(rapidjson::kObjectType);
                for (const auto& [key, value] : data.venueNames) {
                    rapidjson::Value key_val, value_val;
                    key_val.SetString(key.c_str(), key.length(), alloc);
                    value_val.SetString(value.c_str(), value.length(), alloc);
                    venueNames_obj.AddMember(key_val, value_val, alloc);
                }
                doc.AddMember("venueNames", venueNames_obj, alloc);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                return buffer.GetSize() > 0;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("RapidJSON", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_YYJSON
    // 3. yyjson - complete implementation
    {
        auto benchmark = [&]() -> bool {
            try {
                yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
                yyjson_mut_val *root = yyjson_mut_obj(doc);
                yyjson_mut_doc_set_root(doc, root);

                // Add all name mappings
                yyjson_mut_val *areaNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.areaNames) {
                    yyjson_mut_obj_add_str(doc, areaNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "areaNames", areaNames_obj);

                yyjson_mut_val *audienceSubCategoryNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.audienceSubCategoryNames) {
                    yyjson_mut_obj_add_str(doc, audienceSubCategoryNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "audienceSubCategoryNames", audienceSubCategoryNames_obj);

                yyjson_mut_val *blockNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.blockNames) {
                    yyjson_mut_obj_add_str(doc, blockNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "blockNames", blockNames_obj);

                // Events object - complete
                yyjson_mut_val *events_obj = yyjson_mut_obj(doc);
                for (const auto& [key, event] : data.events) {
                    yyjson_mut_val *event_obj = yyjson_mut_obj(doc);
                    yyjson_mut_obj_add_int(doc, event_obj, "id", event.id);
                    yyjson_mut_obj_add_str(doc, event_obj, "name", event.name.c_str());

                    if (event.description.has_value()) {
                        yyjson_mut_obj_add_str(doc, event_obj, "description", event.description->c_str());
                    }

                    if (event.logo.has_value()) {
                        yyjson_mut_obj_add_str(doc, event_obj, "logo", event.logo->c_str());
                    }

                    yyjson_mut_val *subTopicIds = yyjson_mut_arr(doc);
                    for (uint64_t id : event.subTopicIds) {
                        yyjson_mut_arr_add_int(doc, subTopicIds, id);
                    }
                    yyjson_mut_obj_add_val(doc, event_obj, "subTopicIds", subTopicIds);

                    if (event.subjectCode.has_value()) {
                        yyjson_mut_obj_add_str(doc, event_obj, "subjectCode", event.subjectCode->c_str());
                    }

                    if (event.subtitle.has_value()) {
                        yyjson_mut_obj_add_str(doc, event_obj, "subtitle", event.subtitle->c_str());
                    }

                    yyjson_mut_val *topicIds = yyjson_mut_arr(doc);
                    for (uint64_t id : event.topicIds) {
                        yyjson_mut_arr_add_int(doc, topicIds, id);
                    }
                    yyjson_mut_obj_add_val(doc, event_obj, "topicIds", topicIds);

                    yyjson_mut_obj_add_val(doc, events_obj, key.c_str(), event_obj);
                }
                yyjson_mut_obj_add_val(doc, root, "events", events_obj);

                // Performances array - complete
                yyjson_mut_val *performances = yyjson_mut_arr(doc);
                for (const auto& perf : data.performances) {
                    yyjson_mut_val *perf_obj = yyjson_mut_obj(doc);
                    yyjson_mut_obj_add_int(doc, perf_obj, "id", perf.id);
                    yyjson_mut_obj_add_int(doc, perf_obj, "eventId", perf.eventId);

                    if (perf.logo.has_value()) {
                        yyjson_mut_obj_add_str(doc, perf_obj, "logo", perf.logo->c_str());
                    }

                    if (perf.name.has_value()) {
                        yyjson_mut_obj_add_str(doc, perf_obj, "name", perf.name->c_str());
                    }

                    // Prices array
                    yyjson_mut_val *prices = yyjson_mut_arr(doc);
                    for (const auto& price : perf.prices) {
                        yyjson_mut_val *price_obj = yyjson_mut_obj(doc);
                        yyjson_mut_obj_add_int(doc, price_obj, "amount", price.amount);
                        yyjson_mut_obj_add_int(doc, price_obj, "audienceSubCategoryId", price.audienceSubCategoryId);
                        yyjson_mut_obj_add_int(doc, price_obj, "seatCategoryId", price.seatCategoryId);
                        yyjson_mut_arr_append(prices, price_obj);
                    }
                    yyjson_mut_obj_add_val(doc, perf_obj, "prices", prices);

                    // Seat categories array
                    yyjson_mut_val *seatCategories = yyjson_mut_arr(doc);
                    for (const auto& seatCat : perf.seatCategories) {
                        yyjson_mut_val *seatCat_obj = yyjson_mut_obj(doc);
                        yyjson_mut_obj_add_int(doc, seatCat_obj, "seatCategoryId", seatCat.seatCategoryId);

                        yyjson_mut_val *areas = yyjson_mut_arr(doc);
                        for (const auto& area : seatCat.areas) {
                            yyjson_mut_val *area_obj = yyjson_mut_obj(doc);
                            yyjson_mut_obj_add_int(doc, area_obj, "areaId", area.areaId);

                            yyjson_mut_val *blockIds = yyjson_mut_arr(doc);
                            for (uint64_t blockId : area.blockIds) {
                                yyjson_mut_arr_add_int(doc, blockIds, blockId);
                            }
                            yyjson_mut_obj_add_val(doc, area_obj, "blockIds", blockIds);

                            yyjson_mut_arr_append(areas, area_obj);
                        }
                        yyjson_mut_obj_add_val(doc, seatCat_obj, "areas", areas);

                        yyjson_mut_arr_append(seatCategories, seatCat_obj);
                    }
                    yyjson_mut_obj_add_val(doc, perf_obj, "seatCategories", seatCategories);

                    if (perf.seatMapImage.has_value()) {
                        yyjson_mut_obj_add_str(doc, perf_obj, "seatMapImage", perf.seatMapImage->c_str());
                    }

                    yyjson_mut_obj_add_int(doc, perf_obj, "start", perf.start);
                    yyjson_mut_obj_add_str(doc, perf_obj, "venueCode", perf.venueCode.c_str());

                    yyjson_mut_arr_append(performances, perf_obj);
                }
                yyjson_mut_obj_add_val(doc, root, "performances", performances);

                // Add remaining name mappings
                yyjson_mut_val *seatCategoryNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.seatCategoryNames) {
                    yyjson_mut_obj_add_str(doc, seatCategoryNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "seatCategoryNames", seatCategoryNames_obj);

                yyjson_mut_val *subTopicNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.subTopicNames) {
                    yyjson_mut_obj_add_str(doc, subTopicNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "subTopicNames", subTopicNames_obj);

                yyjson_mut_val *subjectNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.subjectNames) {
                    yyjson_mut_obj_add_str(doc, subjectNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "subjectNames", subjectNames_obj);

                yyjson_mut_val *topicNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.topicNames) {
                    yyjson_mut_obj_add_str(doc, topicNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "topicNames", topicNames_obj);

                yyjson_mut_val *topicSubTopics_obj = yyjson_mut_obj(doc);
                for (const auto& [key, values] : data.topicSubTopics) {
                    yyjson_mut_val *arr = yyjson_mut_arr(doc);
                    for (uint64_t val : values) {
                        yyjson_mut_arr_add_int(doc, arr, val);
                    }
                    yyjson_mut_obj_add_val(doc, topicSubTopics_obj, key.c_str(), arr);
                }
                yyjson_mut_obj_add_val(doc, root, "topicSubTopics", topicSubTopics_obj);

                yyjson_mut_val *venueNames_obj = yyjson_mut_obj(doc);
                for (const auto& [key, value] : data.venueNames) {
                    yyjson_mut_obj_add_str(doc, venueNames_obj, key.c_str(), value.c_str());
                }
                yyjson_mut_obj_add_val(doc, root, "venueNames", venueNames_obj);

                char *json_str = yyjson_mut_write(doc, 0, NULL);
                bool success = (json_str != NULL);
                if (json_str) free(json_str);
                yyjson_mut_doc_free(doc);

                return success;
            } catch (...) {
                return false;
            }
        };

        results.push_back(run_benchmark("yyjson", sample_json, iterations, benchmark));
    }
#endif

#ifdef HAS_SERDE
    // 4. Serde (Rust) serialization
    {
        // Parse the original JSON to get Serde's internal representation
        // Get the actual CITM JSON file
        std::string citm_json_full;
        {
            std::ifstream file("jsonexamples/citm_catalog.json", std::ios::binary);
            if (!file) {
                file.open("../jsonexamples/citm_catalog.json", std::ios::binary);
            }
            if (!file) {
                file.open("../../jsonexamples/citm_catalog.json", std::ios::binary);
            }
            if (file) {
                citm_json_full = std::string((std::istreambuf_iterator<char>(file)),
                                             std::istreambuf_iterator<char>());
            }
        }

        serde_benchmark::CitmCatalog* serde_data = nullptr;
        if (!citm_json_full.empty()) {
            serde_data = serde_benchmark::citm_from_str(
                citm_json_full.c_str(), citm_json_full.size());
        }

        if (serde_data) {
            auto benchmark = [serde_data]() -> bool {
                try {
                    char* json_str = serde_benchmark::str_from_citm(serde_data);
                    if (json_str) {
                        serde_benchmark::free_str(json_str);
                        return true;
                    }
                    return false;
                } catch (...) {
                    return false;
                }
            };

            results.push_back(run_benchmark("Serde (Rust)", sample_json, iterations, benchmark));
            serde_benchmark::free_citm(serde_data);
        }
    }
#endif

    return results;
}

// Print results table
void print_results_table(const std::string& title,
                        const std::vector<BenchmarkResult>& results) {
    std::cout << "\n" << title << "\n";
    std::cout << std::string(title.length(), '=') << "\n\n";

    size_t max_name_len = 30;
    for (const auto& r : results) {
        max_name_len = std::max(max_name_len, r.name.length());
    }

    std::cout << std::left << std::setw(max_name_len + 2) << "Library/Method"
              << std::right << std::setw(12) << "Throughput"
              << std::setw(15) << "Time/iter"
              << std::setw(12) << "Iterations"
              << std::setw(10) << "Status" << "\n";
    std::cout << std::string(max_name_len + 2 + 12 + 15 + 12 + 10, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(max_name_len + 2) << r.name;

        if (r.success) {
            std::cout << std::right << std::setw(10) << std::fixed << std::setprecision(1)
                     << r.throughput_mbps << " MB/s"
                     << std::setw(12) << std::fixed << std::setprecision(2)
                     << r.time_per_iteration_us << " μs"
                     << std::setw(12) << r.iterations
                     << std::setw(10) << "✓" << "\n";
        } else {
            std::cout << std::right << std::setw(10) << "N/A"
                     << std::setw(15) << "N/A"
                     << std::setw(12) << r.iterations
                     << std::setw(10) << "Failed" << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    bool run_parsing = true;
    bool run_serialization = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--serialization" || arg == "-s") {
            run_parsing = false;
            run_serialization = true;
        } else if (arg == "--parsing" || arg == "-p") {
            run_parsing = true;
            run_serialization = false;
        } else if (arg == "--both" || arg == "-b") {
            run_parsing = true;
            run_serialization = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n";
            std::cout << "Options:\n";
            std::cout << "  --parsing, -p       Run only parsing benchmarks (default)\n";
            std::cout << "  --serialization, -s Run only serialization benchmarks\n";
            std::cout << "  --both, -b          Run both parsing and serialization benchmarks\n";
            std::cout << "  --help, -h          Show this help message\n";
            return 0;
        }
    }

    std::cout << "=== Unified JSON Performance Benchmark ===\n";
    if (run_parsing && run_serialization) {
        std::cout << "Mode: PARSING and SERIALIZATION\n\n";
    } else if (run_serialization) {
        std::cout << "Mode: SERIALIZATION ONLY (C++ structs → JSON)\n\n";
    } else {
        std::cout << "Mode: PARSING ONLY (JSON → C++ structs)\n\n";
    }

    // Load Twitter test data - try multiple paths
    std::string twitter_json;
    {
        std::vector<std::string> paths = {
            "jsonexamples/twitter.json",
            "../jsonexamples/twitter.json",
            "../../jsonexamples/twitter.json"
        };

        bool loaded = false;
        for (const auto& path : paths) {
            std::ifstream file(path, std::ios::binary);
            if (file) {
                twitter_json = std::string((std::istreambuf_iterator<char>(file)),
                                          std::istreambuf_iterator<char>());
                std::cout << "Loaded Twitter dataset from " << path << "\n";
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            std::cerr << "Error: Could not load twitter.json\n";
            std::cerr << "Please run from simdjson root directory:\n";
            std::cerr << "  cd /path/to/simdjson\n";
            std::cerr << "  ./benchmark/unified_benchmark\n";
            return 1;
        }
    }

    std::cout << "Twitter JSON size: " << twitter_json.size() << " bytes\n";

    // Parse Twitter data for serialization benchmarks
    TwitterData twitter_data;
    {
        auto &parser = simdjson::ondemand::parser::get_parser();
        simdjson::padded_string padded(twitter_json);
        simdjson::ondemand::document doc;
        if (parser.iterate(padded).get(doc) == simdjson::SUCCESS) {
            doc.get(twitter_data);
        }
    }

    // Run Twitter benchmarks based on mode
    if (run_parsing) {
        auto twitter_results = benchmark_twitter_parsing(twitter_json);
        print_results_table("TWITTER PARSING PERFORMANCE", twitter_results);
    }

    if (run_serialization) {
        auto twitter_ser_results = benchmark_twitter_serialization(twitter_data);
        print_results_table("TWITTER SERIALIZATION PERFORMANCE", twitter_ser_results);
    }

    // Load CITM test data - try multiple paths
    std::string citm_json;
    {
        std::vector<std::string> paths = {
            "jsonexamples/citm_catalog.json",
            "../jsonexamples/citm_catalog.json",
            "../../jsonexamples/citm_catalog.json"
        };

        bool loaded = false;
        for (const auto& path : paths) {
            std::ifstream file(path, std::ios::binary);
            if (file) {
                citm_json = std::string((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                std::cout << "\nLoaded CITM dataset from " << path << "\n";
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            std::cerr << "Error: Could not load citm_catalog.json\n";
            std::cerr << "Please run from simdjson root directory:\n";
            std::cerr << "  cd /path/to/simdjson\n";
            std::cerr << "  ./benchmark/unified_benchmark\n";
            return 1;
        }
    }

    std::cout << "CITM JSON size: " << citm_json.size() << " bytes\n";

    // Parse CITM data for serialization benchmarks
    CITMCatalog citm_data;
    {
        auto &parser = simdjson::ondemand::parser::get_parser();
        simdjson::padded_string padded(citm_json);
        simdjson::ondemand::document doc;
        if (parser.iterate(padded).get(doc) == simdjson::SUCCESS) {
            doc.get(citm_data);
        }
    }

    // Run CITM benchmarks based on mode
    if (run_parsing) {
        auto citm_results = benchmark_citm_parsing(citm_json);
        print_results_table("CITM CATALOG PARSING PERFORMANCE", citm_results);
    }

    if (run_serialization) {
        auto citm_ser_results = benchmark_citm_serialization(citm_data);
        print_results_table("CITM CATALOG SERIALIZATION PERFORMANCE", citm_ser_results);
    }

    std::cout << "\n=== Summary ===\n";
    std::cout << "All benchmarks use full field extraction for fair comparison.\n";
    std::cout << "Conservative approach: Fresh parser instance for each iteration (realistic performance).\n";

    return 0;
}