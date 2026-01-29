#ifndef YYJSON_CITM_CATALOG_DATA_H
#define YYJSON_CITM_CATALOG_DATA_H

#include "citm_catalog_data.h"
#include <yyjson.h>
#include <string>
#include <stdexcept>

// yyjson deserialization for CITM Catalog data
// Matches C++ CitmCatalog struct (only events + performances)
CitmCatalog yyjson_deserialize_citm(const std::string &json_str) {
    CitmCatalog catalog;

    yyjson_doc *doc = yyjson_read(json_str.c_str(), json_str.size(), 0);
    if (!doc) {
        throw std::runtime_error("yyjson parse error");
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root) {
        yyjson_doc_free(doc);
        return catalog;
    }

    // Parse events
    yyjson_val *events_val = yyjson_obj_get(root, "events");
    if (events_val && yyjson_is_obj(events_val)) {
        size_t idx, max;
        yyjson_val *key, *val;
        yyjson_obj_foreach(events_val, idx, max, key, val) {
            CITMEvent event;

            yyjson_val *v;
            v = yyjson_obj_get(val, "description");
            if (v && yyjson_is_str(v)) event.description = yyjson_get_str(v);

            v = yyjson_obj_get(val, "id");
            if (v && yyjson_is_uint(v)) event.id = yyjson_get_uint(v);

            v = yyjson_obj_get(val, "logo");
            if (v && yyjson_is_str(v)) event.logo = yyjson_get_str(v);

            v = yyjson_obj_get(val, "name");
            if (v && yyjson_is_str(v)) event.name = yyjson_get_str(v);

            v = yyjson_obj_get(val, "subjectCode");
            if (v && yyjson_is_str(v)) event.subjectCode = yyjson_get_str(v);

            v = yyjson_obj_get(val, "subtitle");
            if (v && yyjson_is_str(v)) event.subtitle = yyjson_get_str(v);

            // Parse topicIds array
            v = yyjson_obj_get(val, "topicIds");
            if (v && yyjson_is_arr(v)) {
                size_t arr_idx, arr_max;
                yyjson_val *arr_val;
                yyjson_arr_foreach(v, arr_idx, arr_max, arr_val) {
                    if (yyjson_is_uint(arr_val))
                        event.topicIds.push_back(yyjson_get_uint(arr_val));
                }
            }

            // Parse subTopicIds array
            v = yyjson_obj_get(val, "subTopicIds");
            if (v && yyjson_is_arr(v)) {
                size_t arr_idx, arr_max;
                yyjson_val *arr_val;
                yyjson_arr_foreach(v, arr_idx, arr_max, arr_val) {
                    if (yyjson_is_uint(arr_val))
                        event.subTopicIds.push_back(yyjson_get_uint(arr_val));
                }
            }

            if (yyjson_is_str(key))
                catalog.events[yyjson_get_str(key)] = event;
        }
    }

    // Parse performances (simplified - full parsing would need prices/seatCategories)
    yyjson_val *performances_val = yyjson_obj_get(root, "performances");
    if (performances_val && yyjson_is_arr(performances_val)) {
        size_t idx, max;
        yyjson_val *perf_val;
        yyjson_arr_foreach(performances_val, idx, max, perf_val) {
            CITMPerformance perf;

            yyjson_val *v;
            v = yyjson_obj_get(perf_val, "id");
            if (v && yyjson_is_uint(v)) perf.id = yyjson_get_uint(v);

            v = yyjson_obj_get(perf_val, "eventId");
            if (v && yyjson_is_uint(v)) perf.eventId = yyjson_get_uint(v);

            v = yyjson_obj_get(perf_val, "start");
            if (v && yyjson_is_uint(v)) perf.start = yyjson_get_uint(v);

            v = yyjson_obj_get(perf_val, "venueCode");
            if (v && yyjson_is_str(v)) perf.venueCode = yyjson_get_str(v);

            v = yyjson_obj_get(perf_val, "name");
            if (v && yyjson_is_str(v)) perf.name = yyjson_get_str(v);

            v = yyjson_obj_get(perf_val, "logo");
            if (v && yyjson_is_str(v)) perf.logo = yyjson_get_str(v);

            v = yyjson_obj_get(perf_val, "seatMapImage");
            if (v && yyjson_is_str(v)) perf.seatMapImage = yyjson_get_str(v);

            // Note: prices and seatCategories parsing omitted for brevity
            // The serialization benchmark uses data loaded by simdjson

            catalog.performances.push_back(perf);
        }
    }

    yyjson_doc_free(doc);
    return catalog;
}

// Helper to add optional string field
static inline void yyjson_add_optional_str(yyjson_mut_doc *doc, yyjson_mut_val *obj,
                                           const char *key, const std::optional<std::string> &val) {
    if (val.has_value()) {
        yyjson_mut_obj_add_str(doc, obj, key, val->c_str());
    } else {
        yyjson_mut_obj_add_null(doc, obj, key);
    }
}

// yyjson serialization for CITM Catalog data
// Matches C++ CitmCatalog struct exactly (only events + performances)
std::string yyjson_serialize_citm(const CitmCatalog &catalog) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    // Create events object
    yyjson_mut_val *events_obj = yyjson_mut_obj(doc);
    for (const auto& [key, event] : catalog.events) {
        yyjson_mut_val *event_obj = yyjson_mut_obj(doc);

        yyjson_add_optional_str(doc, event_obj, "description", event.description);
        yyjson_mut_obj_add_uint(doc, event_obj, "id", event.id);
        yyjson_add_optional_str(doc, event_obj, "logo", event.logo);
        // name is not optional in CITMEvent
        yyjson_mut_obj_add_str(doc, event_obj, "name", event.name.c_str());

        // Add subTopicIds array
        yyjson_mut_val *subtopic_ids = yyjson_mut_arr(doc);
        for (uint64_t id : event.subTopicIds) {
            yyjson_mut_arr_add_uint(doc, subtopic_ids, id);
        }
        yyjson_mut_obj_add_val(doc, event_obj, "subTopicIds", subtopic_ids);

        yyjson_add_optional_str(doc, event_obj, "subjectCode", event.subjectCode);
        yyjson_add_optional_str(doc, event_obj, "subtitle", event.subtitle);

        // Add topicIds array
        yyjson_mut_val *topic_ids = yyjson_mut_arr(doc);
        for (uint64_t id : event.topicIds) {
            yyjson_mut_arr_add_uint(doc, topic_ids, id);
        }
        yyjson_mut_obj_add_val(doc, event_obj, "topicIds", topic_ids);

        yyjson_mut_obj_add_val(doc, events_obj, key.c_str(), event_obj);
    }
    yyjson_mut_obj_add_val(doc, root, "events", events_obj);

    // Create performances array
    yyjson_mut_val *performances_array = yyjson_mut_arr(doc);
    for (const auto& perf : catalog.performances) {
        yyjson_mut_val *perf_obj = yyjson_mut_obj(doc);

        yyjson_mut_obj_add_uint(doc, perf_obj, "eventId", perf.eventId);
        yyjson_mut_obj_add_uint(doc, perf_obj, "id", perf.id);
        yyjson_add_optional_str(doc, perf_obj, "logo", perf.logo);
        yyjson_add_optional_str(doc, perf_obj, "name", perf.name);

        // Add prices array
        yyjson_mut_val *prices_array = yyjson_mut_arr(doc);
        for (const auto& price : perf.prices) {
            yyjson_mut_val *price_obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_uint(doc, price_obj, "amount", price.amount);
            yyjson_mut_obj_add_uint(doc, price_obj, "audienceSubCategoryId", price.audienceSubCategoryId);
            yyjson_mut_obj_add_uint(doc, price_obj, "seatCategoryId", price.seatCategoryId);
            yyjson_mut_arr_append(prices_array, price_obj);
        }
        yyjson_mut_obj_add_val(doc, perf_obj, "prices", prices_array);

        // Add seatCategories array
        yyjson_mut_val *seat_cats_array = yyjson_mut_arr(doc);
        for (const auto& seatCat : perf.seatCategories) {
            yyjson_mut_val *seat_cat_obj = yyjson_mut_obj(doc);

            // Add areas array
            yyjson_mut_val *areas_array = yyjson_mut_arr(doc);
            for (const auto& area : seatCat.areas) {
                yyjson_mut_val *area_obj = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_uint(doc, area_obj, "areaId", area.areaId);

                yyjson_mut_val *block_ids = yyjson_mut_arr(doc);
                for (uint64_t blockId : area.blockIds) {
                    yyjson_mut_arr_add_uint(doc, block_ids, blockId);
                }
                yyjson_mut_obj_add_val(doc, area_obj, "blockIds", block_ids);
                yyjson_mut_arr_append(areas_array, area_obj);
            }
            yyjson_mut_obj_add_val(doc, seat_cat_obj, "areas", areas_array);
            yyjson_mut_obj_add_uint(doc, seat_cat_obj, "seatCategoryId", seatCat.seatCategoryId);
            yyjson_mut_arr_append(seat_cats_array, seat_cat_obj);
        }
        yyjson_mut_obj_add_val(doc, perf_obj, "seatCategories", seat_cats_array);

        yyjson_add_optional_str(doc, perf_obj, "seatMapImage", perf.seatMapImage);
        yyjson_mut_obj_add_uint(doc, perf_obj, "start", perf.start);
        yyjson_mut_obj_add_str(doc, perf_obj, "venueCode", perf.venueCode.c_str());

        yyjson_mut_arr_append(performances_array, perf_obj);
    }
    yyjson_mut_obj_add_val(doc, root, "performances", performances_array);

    // Write to string
    char *json_output = yyjson_mut_write(doc, 0, NULL);
    std::string result(json_output);
    free(json_output);
    yyjson_mut_doc_free(doc);

    return result;
}

#endif // YYJSON_CITM_CATALOG_DATA_H