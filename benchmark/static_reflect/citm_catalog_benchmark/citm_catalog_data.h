#ifndef CITM_CATALOG_DATA_H
#define CITM_CATALOG_DATA_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

// Price structure with simpler field names to avoid reflection issues
struct CITMPrice {
    uint64_t amount;
    uint64_t audience;  // was audienceSubCategoryId
    uint64_t seat;      // was seatCategoryId
    bool operator==(const CITMPrice&) const = default;
};

struct CITMArea {
    uint64_t areaId;
    std::vector<uint64_t> blockIds;
    bool operator==(const CITMArea&) const = default;
};

struct CITMSeatCategory {
    std::vector<CITMArea> areas;
    uint64_t seatCategoryId;
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

struct CitmCatalog {
    std::map<std::string, CITMEvent> events;
    std::vector<CITMPerformance> performances;
    bool operator==(const CitmCatalog&) const = default;
};

// Type aliases
using Event = CITMEvent;
using Performance = CITMPerformance;
using Price = CITMPrice;
using SeatArea = CITMArea;
using SeatCategoryInfo = CITMSeatCategory;

#endif