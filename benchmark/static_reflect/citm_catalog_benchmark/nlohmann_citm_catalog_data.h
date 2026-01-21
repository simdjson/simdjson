// nlohmann_citm_catalog_data.h
#ifndef NLOHMANN_CITM_CATALOG_DATA_H
#define NLOHMANN_CITM_CATALOG_DATA_H

#include "citm_catalog_data.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

// ---- Area ----
inline void to_json(json &j, const Area &a) {
    j = json{
      {"id", a.id},
      {"name", a.name},
      {"parent", a.parent},
      {"childAreas", a.childAreas}
    };
}
inline void from_json(const json &j, Area &a) {
    j.at("id").get_to(a.id);
    j.at("name").get_to(a.name);
    j.at("parent").get_to(a.parent);
    j.at("childAreas").get_to(a.childAreas);
}

// ---- AudienceSubCategory ----
inline void to_json(json &j, const AudienceSubCategory &asc) {
    j = json{
      {"id", asc.id},
      {"name", asc.name},
      {"parent", asc.parent}
    };
}
inline void from_json(const json &j, AudienceSubCategory &asc) {
    j.at("id").get_to(asc.id);
    j.at("name").get_to(asc.name);
    j.at("parent").get_to(asc.parent);
}

// ---- Event ----
inline void to_json(json &j, const Event &e) {
    j = json{
      {"id", e.id},
      {"name", e.name},
      {"description", e.description},
      {"subTopic", e.subTopic},
      {"topic", e.topic},
      {"audience", e.audience}
    };
}
inline void from_json(const json &j, Event &e) {
    j.at("id").get_to(e.id);
    j.at("name").get_to(e.name);
    j.at("description").get_to(e.description);
    j.at("subTopic").get_to(e.subTopic);
    j.at("topic").get_to(e.topic);
    j.at("audience").get_to(e.audience);
}

// ---- Performance ----
inline void to_json(json &j, const Performance &p) {
    j = json{
      {"id", p.id},
      {"name", p.name},
      {"event", p.event},
      {"start", p.start},
      {"venueCode", p.venueCode}
    };
}
inline void from_json(const json &j, Performance &p) {
    j.at("id").get_to(p.id);
    j.at("name").get_to(p.name);
    j.at("event").get_to(p.event);
    j.at("start").get_to(p.start);
    j.at("venueCode").get_to(p.venueCode);
}

// ---- SeatCategory ----
inline void to_json(json &j, const SeatCategory &sc) {
    j = json{
      {"id", sc.id},
      {"name", sc.name},
      {"areas", sc.areas}
    };
}
inline void from_json(const json &j, SeatCategory &sc) {
    j.at("id").get_to(sc.id);
    j.at("name").get_to(sc.name);
    j.at("areas").get_to(sc.areas);
}

// ---- SubTopic ----
inline void to_json(json &j, const SubTopic &st) {
    j = json{
      {"id", st.id},
      {"name", st.name},
      {"parent", st.parent}
    };
}
inline void from_json(const json &j, SubTopic &st) {
    j.at("id").get_to(st.id);
    j.at("name").get_to(st.name);
    j.at("parent").get_to(st.parent);
}

// ---- Topic ----
inline void to_json(json &j, const Topic &t) {
    j = json{
      {"id", t.id},
      {"name", t.name}
    };
}
inline void from_json(const json &j, Topic &t) {
    j.at("id").get_to(t.id);
    j.at("name").get_to(t.name);
}

// ---- Venue ----
inline void to_json(json &j, const Venue &v) {
    j = json{
      {"id", v.id},
      {"name", v.name},
      {"address", v.address}
    };
}
inline void from_json(const json &j, Venue &v) {
    j.at("id").get_to(v.id);
    j.at("name").get_to(v.name);
    j.at("address").get_to(v.address);
}

// ---- CitmCatalog ----
inline void to_json(json &j, const CitmCatalog &c) {
    j = json{
      {"areas", c.areas},
      {"audienceSubCategory", c.audienceSubCategory},
      {"events", c.events},
      {"performances", c.performances},
      {"seatCategory", c.seatCategory},
      {"subTopic", c.subTopic},
      {"topic", c.topic},
      {"venue", c.venue}
    };
}
inline void from_json(const json &j, CitmCatalog &c) {
    j.at("areas").get_to(c.areas);
    j.at("audienceSubCategory").get_to(c.audienceSubCategory);
    j.at("events").get_to(c.events);
    j.at("performances").get_to(c.performances);
    j.at("seatCategory").get_to(c.seatCategory);
    j.at("subTopic").get_to(c.subTopic);
    j.at("topic").get_to(c.topic);
    j.at("venue").get_to(c.venue);
}

// Optional convenience functions for benchmarking
inline std::string nlohmann_serialize(const CitmCatalog &catalog) {
    json j = catalog;
    return j.dump();
}
inline bool nlohmann_deserialize(const std::string &json_in, CitmCatalog &catalog) {
    try {
        catalog = json::parse(json_in);
        return false; // success
    } catch(...) {
        return true;  // failure
    }
}

#endif // NLOHMANN_CITM_CATALOG_DATA_H