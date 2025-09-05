// Unified serialization test for ablation study
// Tests both Twitter and CITM datasets using optimized string_builder

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <simdjson.h>

using namespace simdjson;

// Benchmark Twitter serialization with proper builder reuse
double benchmark_twitter(int iterations = 1000) {
    // Create synthetic Twitter-like data
    std::vector<std::string> tweets;
    for (int i = 0; i < 100; i++) {
        tweets.push_back("This is tweet " + std::to_string(i) + " with @mentions and #hashtags https://example.com/link and more content to make it realistic");
    }

    // Create reusable string_builder outside the loop
    simdjson::arm64::builder::string_builder sb;

    // Warmup
    for (int i = 0; i < 100; i++) {
        sb.clear();
        sb.append("{\"statuses\":[");

        for (size_t j = 0; j < tweets.size(); j++) {
            if (j > 0) sb.append(',');

            sb.append("{\"created_at\":\"Mon Sep 24 03:35:21 +0000 2012\",");
            sb.append("\"id\":");
            sb.append(uint64_t(505874924095815700ULL + j));
            sb.append(",\"text\":\"");
            sb.append(tweets[j]);
            sb.append("\",\"user\":{");
            sb.append("\"id\":");
            sb.append(uint64_t(1186275104 + j));
            sb.append(",\"screen_name\":\"user_");
            sb.append(uint64_t(j));
            sb.append("\",\"name\":\"User ");
            sb.append(uint64_t(j));
            sb.append("\",\"verified\":");
            sb.append(j % 2 == 0);
            sb.append(",\"followers_count\":");
            sb.append(uint64_t(1000 + j * 10));
            sb.append("},\"retweet_count\":");
            sb.append(uint64_t(j * 2));
            sb.append(",\"favorite_count\":");
            sb.append(uint64_t(j * 5));
            sb.append("}");
        }

        sb.append("]}");
        std::string_view result;
        sb.view().get(result);
    }

    // Benchmark
    auto start = std::chrono::steady_clock::now();

    size_t total_size = 0;
    for (int i = 0; i < iterations; i++) {
        sb.clear();  // Clear and reuse the builder
        sb.append("{\"statuses\":[");

        for (size_t j = 0; j < tweets.size(); j++) {
            if (j > 0) sb.append(',');

            sb.append("{\"created_at\":\"Mon Sep 24 03:35:21 +0000 2012\",");
            sb.append("\"id\":");
            sb.append(uint64_t(505874924095815700ULL + j));
            sb.append(",\"text\":\"");
            sb.append(tweets[j]);
            sb.append("\",\"user\":{");
            sb.append("\"id\":");
            sb.append(uint64_t(1186275104 + j));
            sb.append(",\"screen_name\":\"user_");
            sb.append(uint64_t(j));
            sb.append("\",\"name\":\"User ");
            sb.append(uint64_t(j));
            sb.append("\",\"verified\":");
            sb.append(j % 2 == 0);
            sb.append(",\"followers_count\":");
            sb.append(uint64_t(1000 + j * 10));
            sb.append("},\"retweet_count\":");
            sb.append(uint64_t(j * 2));
            sb.append(",\"favorite_count\":");
            sb.append(uint64_t(j * 5));
            sb.append("}");
        }

        sb.append("]}");
        std::string_view result;
        sb.view().get(result);
        total_size = result.size();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double seconds = duration.count() / 1000000.0;
    double mb_per_sec = (total_size * iterations / 1024.0 / 1024.0) / seconds;

    return mb_per_sec;
}

// Benchmark CITM serialization with proper builder reuse
double benchmark_citm(int iterations = 500) {
    // Create CITM-like data with nested structures
    std::vector<std::string> names;
    std::vector<std::string> descriptions;

    for (int i = 0; i < 200; i++) {
        names.push_back("Event " + std::to_string(i) + " - Concert Series");
        descriptions.push_back("Description for event " + std::to_string(i) + " with details");
    }

    // Create reusable string_builder outside the loop
    simdjson::arm64::builder::string_builder sb;

    // Warmup
    for (int i = 0; i < 50; i++) {
        sb.clear();
        sb.append("{\"events\":[],\"performances\":[]}");
        std::string_view result;
        sb.view().get(result);
    }

    // Benchmark
    auto start = std::chrono::steady_clock::now();

    size_t total_size = 0;
    for (int iter = 0; iter < iterations; iter++) {
        sb.clear();  // Clear and reuse the builder
        sb.append("{\"events\":[");

        for (size_t i = 0; i < names.size(); i++) {
            if (i > 0) sb.append(',');
            sb.append("{\"id\":");
            sb.append(uint64_t(138586341 + i));
            sb.append(",\"name\":\"");
            sb.append(names[i]);
            sb.append("\",\"description\":\"");
            sb.append(descriptions[i]);
            sb.append("\",\"topicIds\":[");
            sb.append(uint64_t(324846099 + i));
            sb.append(",");
            sb.append(uint64_t(107888604 + i));
            sb.append("]}");
        }

        sb.append("],\"performances\":[");

        for (int i = 0; i < 500; i++) {
            if (i > 0) sb.append(',');
            sb.append("{\"id\":");
            sb.append(uint64_t(339420000 + i));
            sb.append(",\"eventId\":");
            sb.append(uint64_t(138586341 + (i % 200)));
            sb.append(",\"start\":");
            sb.append(uint64_t(1572892800 + i * 3600));
            sb.append(",\"venueCode\":\"VENUE_");
            sb.append(uint64_t(i % 10));
            sb.append("\"}");
        }

        sb.append("],\"venues\":[");

        for (int i = 0; i < 50; i++) {
            if (i > 0) sb.append(',');
            sb.append("{\"id\":");
            sb.append(uint64_t(1000 + i));
            sb.append(",\"name\":\"Venue ");
            sb.append(uint64_t(i));
            sb.append("\",\"capacity\":");
            sb.append(uint64_t(5000 + i * 100));
            sb.append("}");
        }

        sb.append("]}");
        std::string_view result;
        sb.view().get(result);
        total_size = result.size();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double seconds = duration.count() / 1000000.0;
    double mb_per_sec = (total_size * iterations / 1024.0 / 1024.0) / seconds;

    return mb_per_sec;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <twitter|citm>" << std::endl;
        return 1;
    }

    std::string test_type = argv[1];

    if (test_type == "twitter") {
        double mb_per_sec = benchmark_twitter();
        std::cout << mb_per_sec << std::endl;
    } else if (test_type == "citm") {
        double mb_per_sec = benchmark_citm();
        std::cout << mb_per_sec << std::endl;
    } else {
        std::cerr << "Unknown test type: " << test_type << std::endl;
        return 1;
    }

    return 0;
}