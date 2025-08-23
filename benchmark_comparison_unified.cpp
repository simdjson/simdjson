/**
 * Unified JSON Performance Benchmark
 * 
 * This benchmark compares JSON parsing performance across multiple libraries
 * using their intended/typical usage patterns:
 * 
 * - simdjson: Parser and padded_string created once, reused across iterations
 *   (This is the recommended pattern for simdjson to amortize setup costs)
 * 
 * - nlohmann/json: Creates new parser for each iteration
 *   (Typical usage: nlohmann::json::parse() creates parser internally)
 * 
 * - RapidJSON: Creates new Document for each iteration  
 *   (Typical usage: Document allocated per parse operation)
 * 
 * This approach reflects real-world usage patterns for each library.
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

using namespace simdjson;

// Data structures remain the same
struct TwitterUser {
    uint64_t id;
    std::string screen_name;
    std::string name;
    std::string location;
    std::string description;
    bool verified;
    uint64_t followers_count;
    uint64_t friends_count;
    uint64_t statuses_count;
};

struct TwitterTweet {
    std::string created_at;
    uint64_t id;
    std::string text;
    std::optional<uint64_t> in_reply_to_status_id;
    TwitterUser user;
    uint64_t retweet_count;
    uint64_t favorite_count;
};

struct TwitterData {
    std::vector<TwitterTweet> statuses;
};

// Benchmark result structure
struct BenchmarkResult {
    std::string name;
    double throughput_mbps;
    double time_per_iteration_us;
    size_t iterations;
    bool success;
};

// Note on Parser Reuse:
// simdjson is designed to reuse parsers across multiple documents for optimal performance.
// This benchmark uses parser reuse for simdjson (typical usage pattern) while other 
// libraries create new parsers/documents each iteration (their typical pattern).
// This reflects how each library is intended to be used in production.

// Benchmark runner - consistent methodology for all libraries
template<typename Func>
BenchmarkResult run_benchmark(const std::string& name, 
                             const std::string& json,
                             int iterations,
                             Func f) {
    BenchmarkResult result;
    result.name = name;
    result.iterations = iterations;
    
    try {
        // Same warmup for all
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

// Twitter benchmark
std::vector<BenchmarkResult> benchmark_twitter_parsing(const std::string& json) {
    std::vector<BenchmarkResult> results;
    const int iterations = 1000; // SAME for all libraries
    
    // 1. simdjson - Manual (parser reuse - typical usage pattern)
    {
        padded_string padded(json); // Create once, reuse
        ondemand::parser parser;    // Create once, reuse
        
        auto benchmark = [&]() -> bool {
            try {
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
                    tweet.user.screen_name = std::string(std::string_view(user_obj["screen_name"]));
                    tweet.user.name = std::string(std::string_view(user_obj["name"]));
                    tweet.user.location = std::string(std::string_view(user_obj["location"]));
                    tweet.user.description = std::string(std::string_view(user_obj["description"]));
                    tweet.user.verified = bool(user_obj["verified"]);
                    tweet.user.followers_count = uint64_t(user_obj["followers_count"]);
                    tweet.user.friends_count = uint64_t(user_obj["friends_count"]);
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
    
    // 2. simdjson - Reflection (parser reuse - typical usage pattern)
    {
        padded_string padded(json); // Create once, reuse
        ondemand::parser parser;    // Create once, reuse
        
        auto benchmark = [&]() -> bool {
            try {
                TwitterData data;
                auto doc = parser.iterate(padded);
                return doc.get(data) == SUCCESS;
            } catch (...) {
                return false;
            }
        };
        
        results.push_back(run_benchmark("simdjson (reflection)", json, iterations, benchmark));
    }
    
    // 3. simdjson::from() convenience API (parser reuse internally)
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
    // 4. nlohmann - with extraction
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
    // 5. RapidJSON - with extraction
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
                    
                    // Create std::string for consistency
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
    
    return results;
}

// CITM benchmark
std::vector<BenchmarkResult> benchmark_citm_parsing(const std::string& json) {
    std::vector<BenchmarkResult> results;
    const int iterations = 500; // Fewer iterations for larger document
    
    // Full CITM structures - parsing everything
    struct CITMPrice {
        uint64_t amount;
        uint64_t audienceSubCategoryId;
        uint64_t seatCategoryId;
    };
    
    struct CITMArea {
        uint64_t areaId;
        std::vector<uint64_t> blockIds;
    };
    
    struct CITMSeatCategory {
        uint64_t seatCategoryId;
        std::vector<CITMArea> areas;
    };
    
    struct CITMPerformance {
        uint64_t id;
        uint64_t eventId;
        std::optional<std::string> logo;
        std::optional<std::string> name;
        std::vector<CITMPrice> prices;
        std::vector<CITMSeatCategory> seatCategories;
        std::optional<std::string> seatMapImage;
        uint64_t start;
        std::string venueCode;
    };
    
    struct CITMEvent {
        uint64_t id;
        std::string name;
        std::optional<std::string> description;
        std::optional<std::string> logo;
        std::vector<uint64_t> subTopicIds;
        std::optional<std::string> subjectCode;
        std::optional<std::string> subtitle;
        std::vector<uint64_t> topicIds;
    };
    
    struct CITMCatalog {
        std::map<std::string, CITMEvent> events;
        std::vector<CITMPerformance> performances;
    };
    
    // 1. simdjson - Manual (parser reuse - typical usage pattern)
    {
        padded_string padded(json); // Create once, reuse
        ondemand::parser parser;    // Create once, reuse
        
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                auto doc = parser.iterate(padded);
                
                // Parse events with all fields
                for (auto field : doc["events"].get_object()) {
                    CITMEvent event;
                    auto event_obj = field.value();
                    event.id = uint64_t(event_obj["id"]);
                    event.name = std::string(std::string_view(event_obj["name"]));
                    
                    auto desc = event_obj["description"];
                    if (!desc.is_null()) {
                        event.description = std::string(std::string_view(desc));
                    }
                    
                    auto logo = event_obj["logo"];
                    if (!logo.is_null()) {
                        event.logo = std::string(std::string_view(logo));
                    }
                    
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
                
                // Parse performances with nested structures
                for (auto perf_obj : doc["performances"]) {
                    CITMPerformance perf;
                    perf.id = uint64_t(perf_obj["id"]);
                    perf.eventId = uint64_t(perf_obj["eventId"]);
                    
                    auto logo = perf_obj["logo"];
                    if (!logo.is_null()) {
                        perf.logo = std::string(std::string_view(logo));
                    }
                    
                    auto name = perf_obj["name"];
                    if (!name.is_null()) {
                        perf.name = std::string(std::string_view(name));
                    }
                    
                    for (auto price_obj : perf_obj["prices"]) {
                        CITMPrice price;
                        price.amount = uint64_t(price_obj["amount"]);
                        price.audienceSubCategoryId = uint64_t(price_obj["audienceSubCategoryId"]);
                        price.seatCategoryId = uint64_t(price_obj["seatCategoryId"]);
                        perf.prices.push_back(price);
                    }
                    
                    for (auto seat_cat : perf_obj["seatCategories"]) {
                        CITMSeatCategory sc;
                        sc.seatCategoryId = uint64_t(seat_cat["seatCategoryId"]);
                        
                        for (auto area_obj : seat_cat["areas"]) {
                            CITMArea area;
                            area.areaId = uint64_t(area_obj["areaId"]);
                            for (uint64_t blockId : area_obj["blockIds"]) {
                                area.blockIds.push_back(blockId);
                            }
                            sc.areas.push_back(area);
                        }
                        perf.seatCategories.push_back(sc);
                    }
                    
                    auto seatMapImage = perf_obj["seatMapImage"];
                    if (!seatMapImage.is_null()) {
                        perf.seatMapImage = std::string(std::string_view(seatMapImage));
                    }
                    
                    perf.start = uint64_t(perf_obj["start"]);
                    perf.venueCode = std::string(std::string_view(perf_obj["venueCode"]));
                    
                    catalog.performances.push_back(std::move(perf));
                }
                return true;
            } catch (...) {
                return false;
            }
        };
        
        results.push_back(run_benchmark("simdjson (full extraction)", json, iterations, benchmark));
    }
    
    // 2. simdjson - Reflection (parser reuse - typical usage pattern)
    {
        padded_string padded(json); // Create once, reuse
        ondemand::parser parser;    // Create once, reuse
        
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                auto doc = parser.iterate(padded);
                return doc.get(catalog) == SUCCESS;
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
                CITMCatalog catalog = simdjson::from(padded);
                return true;
            } catch (...) {
                return false;
            }
        };
        
        results.push_back(run_benchmark("simdjson::from()", json, iterations, benchmark));
    }
    
#ifdef HAS_NLOHMANN
    // 4. nlohmann - parsing full structure
    {
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                auto j = nlohmann::json::parse(json);
                
                // Parse events with all fields
                for (auto& [key, event_json] : j["events"].items()) {
                    CITMEvent event;
                    event.id = event_json["id"].get<uint64_t>();
                    event.name = event_json["name"].get<std::string>();
                    
                    if (!event_json["description"].is_null()) {
                        event.description = event_json["description"].get<std::string>();
                    }
                    
                    if (!event_json["logo"].is_null()) {
                        event.logo = event_json["logo"].get<std::string>();
                    }
                    
                    for (auto& id : event_json["subTopicIds"]) {
                        event.subTopicIds.push_back(id.get<uint64_t>());
                    }
                    
                    if (!event_json["subjectCode"].is_null()) {
                        event.subjectCode = event_json["subjectCode"].get<std::string>();
                    }
                    
                    if (!event_json["subtitle"].is_null()) {
                        event.subtitle = event_json["subtitle"].get<std::string>();
                    }
                    
                    for (auto& id : event_json["topicIds"]) {
                        event.topicIds.push_back(id.get<uint64_t>());
                    }
                    
                    catalog.events[key] = std::move(event);
                }
                
                // Parse performances with all nested structures
                for (auto& perf_json : j["performances"]) {
                    CITMPerformance perf;
                    perf.id = perf_json["id"].get<uint64_t>();
                    perf.eventId = perf_json["eventId"].get<uint64_t>();
                    
                    if (!perf_json["logo"].is_null()) {
                        perf.logo = perf_json["logo"].get<std::string>();
                    }
                    
                    if (!perf_json["name"].is_null()) {
                        perf.name = perf_json["name"].get<std::string>();
                    }
                    
                    for (auto& price_json : perf_json["prices"]) {
                        CITMPrice price;
                        price.amount = price_json["amount"].get<uint64_t>();
                        price.audienceSubCategoryId = price_json["audienceSubCategoryId"].get<uint64_t>();
                        price.seatCategoryId = price_json["seatCategoryId"].get<uint64_t>();
                        perf.prices.push_back(price);
                    }
                    
                    for (auto& seat_json : perf_json["seatCategories"]) {
                        CITMSeatCategory sc;
                        sc.seatCategoryId = seat_json["seatCategoryId"].get<uint64_t>();
                        
                        for (auto& area_json : seat_json["areas"]) {
                            CITMArea area;
                            area.areaId = area_json["areaId"].get<uint64_t>();
                            for (auto& blockId : area_json["blockIds"]) {
                                area.blockIds.push_back(blockId.get<uint64_t>());
                            }
                            sc.areas.push_back(area);
                        }
                        perf.seatCategories.push_back(sc);
                    }
                    
                    if (!perf_json["seatMapImage"].is_null()) {
                        perf.seatMapImage = perf_json["seatMapImage"].get<std::string>();
                    }
                    
                    perf.start = perf_json["start"].get<uint64_t>();
                    perf.venueCode = perf_json["venueCode"].get<std::string>();
                    
                    catalog.performances.push_back(std::move(perf));
                }
                
                return true;
            } catch (...) {
                return false;
            }
        };
        
        results.push_back(run_benchmark("nlohmann (full extraction)", json, iterations, benchmark));
    }
#endif

#ifdef HAS_RAPIDJSON
    // 5. RapidJSON - parsing full structure
    {
        auto benchmark = [&]() -> bool {
            try {
                CITMCatalog catalog;
                rapidjson::Document d;
                d.Parse(json.c_str());
                if (d.HasParseError()) return false;
                
                // Parse events with all fields
                const auto& events = d["events"];
                for (auto it = events.MemberBegin(); it != events.MemberEnd(); ++it) {
                    CITMEvent event;
                    const auto& event_json = it->value;
                    
                    event.id = event_json["id"].GetUint64();
                    event.name = std::string(event_json["name"].GetString());
                    
                    if (event_json.HasMember("description") && !event_json["description"].IsNull()) {
                        event.description = std::string(event_json["description"].GetString());
                    }
                    
                    if (event_json.HasMember("logo") && !event_json["logo"].IsNull()) {
                        event.logo = std::string(event_json["logo"].GetString());
                    }
                    
                    const auto& subTopicIds = event_json["subTopicIds"];
                    for (rapidjson::SizeType i = 0; i < subTopicIds.Size(); i++) {
                        event.subTopicIds.push_back(subTopicIds[i].GetUint64());
                    }
                    
                    if (event_json.HasMember("subjectCode") && !event_json["subjectCode"].IsNull()) {
                        event.subjectCode = std::string(event_json["subjectCode"].GetString());
                    }
                    
                    if (event_json.HasMember("subtitle") && !event_json["subtitle"].IsNull()) {
                        event.subtitle = std::string(event_json["subtitle"].GetString());
                    }
                    
                    const auto& topicIds = event_json["topicIds"];
                    for (rapidjson::SizeType i = 0; i < topicIds.Size(); i++) {
                        event.topicIds.push_back(topicIds[i].GetUint64());
                    }
                    
                    catalog.events[it->name.GetString()] = std::move(event);
                }
                
                // Parse performances with all nested structures
                const auto& performances = d["performances"];
                for (rapidjson::SizeType i = 0; i < performances.Size(); i++) {
                    const auto& perf_json = performances[i];
                    CITMPerformance perf;
                    
                    perf.id = perf_json["id"].GetUint64();
                    perf.eventId = perf_json["eventId"].GetUint64();
                    
                    if (perf_json.HasMember("logo") && !perf_json["logo"].IsNull()) {
                        perf.logo = std::string(perf_json["logo"].GetString());
                    }
                    
                    if (perf_json.HasMember("name") && !perf_json["name"].IsNull()) {
                        perf.name = std::string(perf_json["name"].GetString());
                    }
                    
                    const auto& prices = perf_json["prices"];
                    for (rapidjson::SizeType j = 0; j < prices.Size(); j++) {
                        const auto& price_json = prices[j];
                        CITMPrice price;
                        price.amount = price_json["amount"].GetUint64();
                        price.audienceSubCategoryId = price_json["audienceSubCategoryId"].GetUint64();
                        price.seatCategoryId = price_json["seatCategoryId"].GetUint64();
                        perf.prices.push_back(price);
                    }
                    
                    const auto& seatCategories = perf_json["seatCategories"];
                    for (rapidjson::SizeType j = 0; j < seatCategories.Size(); j++) {
                        const auto& seat_json = seatCategories[j];
                        CITMSeatCategory sc;
                        sc.seatCategoryId = seat_json["seatCategoryId"].GetUint64();
                        
                        const auto& areas = seat_json["areas"];
                        for (rapidjson::SizeType k = 0; k < areas.Size(); k++) {
                            const auto& area_json = areas[k];
                            CITMArea area;
                            area.areaId = area_json["areaId"].GetUint64();
                            
                            const auto& blockIds = area_json["blockIds"];
                            for (rapidjson::SizeType m = 0; m < blockIds.Size(); m++) {
                                area.blockIds.push_back(blockIds[m].GetUint64());
                            }
                            sc.areas.push_back(area);
                        }
                        perf.seatCategories.push_back(sc);
                    }
                    
                    if (perf_json.HasMember("seatMapImage") && !perf_json["seatMapImage"].IsNull()) {
                        perf.seatMapImage = std::string(perf_json["seatMapImage"].GetString());
                    }
                    
                    perf.start = perf_json["start"].GetUint64();
                    perf.venueCode = std::string(perf_json["venueCode"].GetString());
                    
                    catalog.performances.push_back(std::move(perf));
                }
                
                return true;
            } catch (...) {
                return false;
            }
        };
        
        results.push_back(run_benchmark("RapidJSON (full extraction)", json, iterations, benchmark));
    }
#endif
    
    return results;
}

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

int main() {
    std::cout << "=== Unified JSON Performance Benchmark ===\n";
    std::cout << "Comparing: Traditional vs Reflection vs Other Libraries\n\n";
    
    // Load Twitter test data
    std::string twitter_json;
    {
        std::ifstream file("jsonexamples/twitter.json", std::ios::binary);
        if (file) {
            twitter_json = std::string((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
            std::cout << "Loaded Twitter dataset from jsonexamples/twitter.json\n";
        } else {
            // Fallback to synthetic data if file not found
            std::stringstream ss;
            ss << R"({"statuses":[)";
            for (int i = 0; i < 100; i++) {
                if (i > 0) ss << ",";
                ss << R"({)"
                   << R"("created_at":"Mon Sep 24 03:35:21 +0000 2012",)"
                   << R"("id":)" << (505874924095815700ULL + i) << ","
                   << R"("text":"This is tweet )" << i << R"( with content @mention #hashtag https://example.com",)"
                   << R"("in_reply_to_status_id":)" << (i > 0 ? std::to_string(505874924095815700ULL + i - 1) : "null") << ","
                   << R"("user":{)"
                   << R"("id":)" << (1186275104 + i) << ","
                   << R"("screen_name":"user_)" << i << R"(",)"
                   << R"("name":"User Name )" << i << R"(",)"
                   << R"("location":"San Francisco, CA",)"
                   << R"("description":"User description with some text",)"
                   << R"("verified":)" << (i % 2 == 0 ? "true" : "false") << ","
                   << R"("followers_count":)" << (1000 + i * 10) << ","
                   << R"("friends_count":)" << (500 + i * 5) << ","
                   << R"("statuses_count":)" << (10000 + i * 100)
                   << R"(},)"
                   << R"("retweet_count":)" << (i * 2) << ","
                   << R"("favorite_count":)" << (i * 5)
                   << R"(})";
            }
            ss << R"(]})";
            twitter_json = ss.str();
            std::cout << "Using synthetic Twitter data (jsonexamples/twitter.json not found)\n";
        }
    }
    
    std::cout << "Twitter JSON size: " << twitter_json.size() << " bytes\n";
    
    // Run Twitter benchmarks
    auto twitter_results = benchmark_twitter_parsing(twitter_json);
    print_results_table("TWITTER PARSING PERFORMANCE", twitter_results);
    
    // Load CITM test data
    std::string citm_json;
    {
        std::ifstream file("jsonexamples/citm_catalog.json", std::ios::binary);
        if (file) {
            citm_json = std::string((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
            std::cout << "Loaded CITM dataset from jsonexamples/citm_catalog.json\n";
        } else {
            // Fallback to synthetic data if file not found
            std::stringstream citm_ss;
            citm_ss << R"({"events":{)";
            for (int i = 0; i < 50; i++) {
                if (i > 0) citm_ss << ",";
                citm_ss << R"("event_)" << i << R"(":{)"
                       << R"("id":)" << (339420000 + i) << ","
                       << R"("name":"Event Name )" << i << R"(",)"
                       << R"("description":)" << (i % 2 == 0 ? R"("Event description with details")" : "null") << ","
                       << R"("logo":"http://example.com/logo_)" << i << R"(.png")"
                       << R"(})";
            }
            citm_ss << R"(}})";
            citm_json = citm_ss.str();
            std::cout << "Using synthetic CITM data (jsonexamples/citm_catalog.json not found)\n";
        }
    }
    
    std::cout << "\nCITM JSON size: " << citm_json.size() << " bytes\n";
    
    // Run CITM benchmarks
    auto citm_results = benchmark_citm_parsing(citm_json);
    print_results_table("CITM CATALOG PARSING PERFORMANCE", citm_results);
    
    return 0;
}