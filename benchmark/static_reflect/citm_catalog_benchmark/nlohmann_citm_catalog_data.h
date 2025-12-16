// nlohmann_citm_catalog_data.h
#ifndef NLOHMANN_CITM_CATALOG_DATA_H
#define NLOHMANN_CITM_CATALOG_DATA_H

#include "citm_catalog_data.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

// ---- CITMPrice ----
inline void to_json(json &j, const CITMPrice &p) {
    j = json{
      {"amount", p.amount},
      {"audienceSubCategoryId", p.audienceSubCategoryId},
      {"seatCategoryId", p.seatCategoryId}
    };
}

// ---- CITMArea ----
inline void to_json(json &j, const CITMArea &a) {
    j = json{
      {"areaId", a.areaId},
      {"blockIds", a.blockIds}
    };
}

// ---- CITMSeatCategory ----
inline void to_json(json &j, const CITMSeatCategory &s) {
    j = json{
      {"areas", s.areas},
      {"seatCategoryId", s.seatCategoryId}
    };
}

// ---- CITMPerformance ----
inline void to_json(json &j, const CITMPerformance &p) {
    j = json{
      {"id", p.id},
      {"eventId", p.eventId},
      {"logo", p.logo},
      {"name", p.name},
      {"prices", p.prices},
      {"seatCategories", p.seatCategories},
      {"seatMapImage", p.seatMapImage},
      {"start", p.start},
      {"venueCode", p.venueCode}
    };
}

// ---- CITMEvent ----
inline void to_json(json &j, const CITMEvent &e) {
    j = json{
      {"id", e.id},
      {"name", e.name},
      {"description", e.description},
      {"logo", e.logo},
      {"subTopicIds", e.subTopicIds},
      {"subjectCode", e.subjectCode},
      {"subtitle", e.subtitle},
      {"topicIds", e.topicIds}
    };
}

// ---- CitmCatalog ----
inline void to_json(json &j, const CitmCatalog &c) {
    j = json{
      {"events", c.events},
      {"performances", c.performances}
    };
}

// Serialization function
inline std::string nlohmann_serialize(const CitmCatalog &catalog) {
    json j = catalog;
    return j.dump();
}

#endif // NLOHMANN_CITM_CATALOG_DATA_H