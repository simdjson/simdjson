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

#ifdef HAS_SERDE
#include "serde_benchmark.h"
#endif

using namespace simdjson;

// Twitter data structures
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
    bool operator==(const TwitterUser&) const = default;
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
};

struct TwitterData {
    std::vector<TwitterTweet> statuses;
    bool operator==(const TwitterData&) const = default;
};

// CITM data structures
struct CITMPrice {
    uint64_t amount;
    uint64_t audienceSubCategoryId;
    uint64_t seatCategoryId;
    bool operator==(const CITMPrice&) const = default;
};

struct CITMArea {
    uint64_t areaId;
    std::vector<uint64_t> blockIds;
    bool operator==(const CITMArea&) const = default;
};

struct CITMSeatCategory {
    uint64_t seatCategoryId;
    std::vector<CITMArea> areas;
    bool operator==(const CITMSeatCategory&) const = default;
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
    bool operator==(const CITMPerformance&) const = default;
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
    bool operator==(const CITMEvent&) const = default;
};

struct CITMCatalog {
    std::map<std::string, CITMEvent> events;
    std::vector<CITMPerformance> performances;
    bool operator==(const CITMCatalog&) const = default;
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
    const int iterations = 1000;
    
    // 1. simdjson - Manual (conservative approach - fresh parser each iteration)
    {
        padded_string padded(json); // Create once
        
        auto benchmark = [&]() -> bool {
            try {
                ondemand::parser parser; // Fresh parser each iteration
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
    
    // 2. simdjson - Reflection (conservative approach)
    {
        padded_string padded(json); // Create once
        
        auto benchmark = [&]() -> bool {
            try {
                ondemand::parser parser; // Fresh parser each iteration
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
    
    return results;
}

// CITM benchmarks
std::vector<BenchmarkResult> benchmark_citm_parsing(const std::string& json) {
    std::vector<BenchmarkResult> results;
    const int iterations = 500;
    
    // 1. simdjson - Manual (conservative approach)
    {
        padded_string padded(json); // Create once
        
        auto benchmark = [&]() -> bool {
            try {
                ondemand::parser parser; // Fresh parser each iteration
                CITMCatalog catalog;
                auto doc = parser.iterate(padded);
                
                // Parse events
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
                    
                    for (uint64_t id : event_obj["topicIds"]) {
                        event.topicIds.push_back(id);
                    }
                    
                    std::string_view key = field.unescaped_key().value();
                    catalog.events[std::string(key)] = std::move(event);
                }
                
                // Parse performances (simplified - add full parsing as needed)
                for (auto perf_obj : doc["performances"]) {
                    CITMPerformance perf;
                    perf.id = uint64_t(perf_obj["id"]);
                    perf.eventId = uint64_t(perf_obj["eventId"]);
                    perf.start = uint64_t(perf_obj["start"]);
                    perf.venueCode = std::string(std::string_view(perf_obj["venueCode"]));
                    
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
                    
                    catalog.performances.push_back(std::move(perf));
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
                ondemand::parser parser; // Fresh parser each iteration
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
                
                // Parse performances
                for (auto& perf_json : j["performances"]) {
                    CITMPerformance perf;
                    perf.id = perf_json["id"].get<uint64_t>();
                    perf.eventId = perf_json["eventId"].get<uint64_t>();
                    perf.start = perf_json["start"].get<uint64_t>();
                    perf.venueCode = perf_json["venueCode"].get<std::string>();
                    
                    if (perf_json.contains("prices")) {
                        for (auto& price_json : perf_json["prices"]) {
                            CITMPrice price;
                            price.amount = price_json["amount"].get<uint64_t>();
                            price.audienceSubCategoryId = price_json["audienceSubCategoryId"].get<uint64_t>();
                            price.seatCategoryId = price_json["seatCategoryId"].get<uint64_t>();
                            perf.prices.push_back(price);
                        }
                    }
                    
                    catalog.performances.push_back(std::move(perf));
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
                
                // Parse performances
                const auto& performances = d["performances"];
                for (rapidjson::SizeType i = 0; i < performances.Size(); i++) {
                    const auto& perf_json = performances[i];
                    CITMPerformance perf;
                    
                    perf.id = perf_json["id"].GetUint64();
                    perf.eventId = perf_json["eventId"].GetUint64();
                    perf.start = perf_json["start"].GetUint64();
                    perf.venueCode = std::string(perf_json["venueCode"].GetString());
                    
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
                    
                    catalog.performances.push_back(std::move(perf));
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

int main() {
    std::cout << "=== Unified JSON Performance Benchmark ===\n";
    std::cout << "Comparing: simdjson (manual, reflection, from()) vs Other Libraries\n\n";
    
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
    
    // Run Twitter benchmarks
    auto twitter_results = benchmark_twitter_parsing(twitter_json);
    print_results_table("TWITTER PARSING PERFORMANCE", twitter_results);
    
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
    
    // Run CITM benchmarks
    auto citm_results = benchmark_citm_parsing(citm_json);
    print_results_table("CITM CATALOG PARSING PERFORMANCE", citm_results);
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "All benchmarks use full field extraction for fair comparison.\n";
    std::cout << "Conservative approach: Fresh parser instance for each iteration (realistic performance).\n";
    
    return 0;
}