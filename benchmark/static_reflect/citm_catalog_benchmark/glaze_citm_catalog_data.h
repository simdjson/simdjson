#ifndef GLAZE_CITM_CATALOG_DATA_H
#define GLAZE_CITM_CATALOG_DATA_H

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <glaze/glaze.hpp>

// Glaze-specific shadow types. We mirror the Rust serde struct here (rather
// than the C++ CitmCatalog) so that fields the source JSON encodes as `null`
// (e.g. CITMEvent.name) parse cleanly into std::optional. This matches what
// the Rust benchmark does, and the resulting JSON output volume is therefore
// directly comparable to the Rust numbers.

struct GlazeCITMPrice {
    uint64_t amount;
    uint64_t audienceSubCategoryId;
    uint64_t seatCategoryId;
};

struct GlazeCITMArea {
    uint64_t areaId;
    std::vector<uint64_t> blockIds;
};

struct GlazeCITMSeatCategory {
    std::vector<GlazeCITMArea> areas;
    uint64_t seatCategoryId;
};

struct GlazeCITMPerformance {
    uint64_t id;
    uint64_t eventId;
    std::optional<std::string> logo;
    std::optional<std::string> name;
    std::vector<GlazeCITMPrice> prices;
    std::vector<GlazeCITMSeatCategory> seatCategories;
    std::optional<std::string> seatMapImage;
    uint64_t start;
    std::string venueCode;
};

struct GlazeCITMEvent {
    uint64_t id;
    std::optional<std::string> name;
    std::optional<std::string> description;
    std::optional<std::string> logo;
    std::vector<uint64_t> subTopicIds;
    std::optional<std::string> subjectCode;
    std::optional<std::string> subtitle;
    std::vector<uint64_t> topicIds;
};

struct GlazeCitmCatalog {
    std::map<std::string, GlazeCITMEvent> events;
    std::vector<GlazeCITMPerformance> performances;
};

inline GlazeCitmCatalog glaze_deserialize_citm(const std::string &json_str) {
    GlazeCitmCatalog data;
    constexpr glz::opts opts{.error_on_unknown_keys = false};
    auto err = glz::read<opts>(data, json_str);
    if (err) {
        throw std::runtime_error("glaze citm parse error: " +
                                 glz::format_error(err, json_str));
    }
    return data;
}

inline std::string glaze_serialize_citm(const GlazeCitmCatalog &data) {
    std::string out;
    // skip_null_members = false: emit `"field":null` for unset optionals so the
    // output has the same field count as simdjson's (which writes `"field":""`).
    // Glaze still produces 4-char `null` vs simdjson's 2-char `""`, so it's not
    // byte-identical, but the work-per-field is comparable.
    constexpr glz::opts opts{.skip_null_members = false};
    auto err = glz::write<opts>(data, out);
    if (err) {
        throw std::runtime_error("glaze citm write error");
    }
    return out;
}

#endif // GLAZE_CITM_CATALOG_DATA_H
