#ifndef YYJSON_CITM_CATALOG_DATA_H
#define YYJSON_CITM_CATALOG_DATA_H

#include "citm_catalog_data.h"
#include <yyjson.h>
#include <string>
#include <stdexcept>

// yyjson deserialization for CITM Catalog data
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
            Event event;

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

    // Parse performances
    yyjson_val *performances_val = yyjson_obj_get(root, "performances");
    if (performances_val && yyjson_is_arr(performances_val)) {
        size_t idx, max;
        yyjson_val *perf_val;
        yyjson_arr_foreach(performances_val, idx, max, perf_val) {
            Performance perf;

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

            catalog.performances.push_back(perf);
        }
    }

    // Helper to parse string maps
    auto parseStringMap = [&root](const char* key, std::map<std::string, std::string>& target) {
        yyjson_val *map_val = yyjson_obj_get(root, key);
        if (map_val && yyjson_is_obj(map_val)) {
            size_t idx, max;
            yyjson_val *k, *v;
            yyjson_obj_foreach(map_val, idx, max, k, v) {
                if (yyjson_is_str(k) && yyjson_is_str(v)) {
                    target[yyjson_get_str(k)] = yyjson_get_str(v);
                }
            }
        }
    };

    parseStringMap("areaNames", catalog.areaNames);
    parseStringMap("audienceSubCategoryNames", catalog.audienceSubCategoryNames);
    parseStringMap("blockNames", catalog.blockNames);
    parseStringMap("seatCategoryNames", catalog.seatCategoryNames);
    parseStringMap("subTopicNames", catalog.subTopicNames);
    parseStringMap("subjectNames", catalog.subjectNames);
    parseStringMap("topicNames", catalog.topicNames);
    parseStringMap("venueNames", catalog.venueNames);

    yyjson_doc_free(doc);
    return catalog;
}

// yyjson serialization for CITM Catalog data
std::string yyjson_serialize_citm(const CitmCatalog &catalog) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    // Create events object
    yyjson_mut_val *events_obj = yyjson_mut_obj(doc);
    for (const auto& [key, event] : catalog.events) {
        yyjson_mut_val *event_obj = yyjson_mut_obj(doc);

        yyjson_mut_obj_add_str(doc, event_obj, "description", event.description.c_str());
        yyjson_mut_obj_add_uint(doc, event_obj, "id", event.id);
        yyjson_mut_obj_add_str(doc, event_obj, "logo", event.logo.c_str());
        yyjson_mut_obj_add_str(doc, event_obj, "name", event.name.c_str());
        yyjson_mut_obj_add_str(doc, event_obj, "subjectCode", event.subjectCode.c_str());
        yyjson_mut_obj_add_str(doc, event_obj, "subtitle", event.subtitle.c_str());

        // Add topicIds array
        yyjson_mut_val *topic_ids = yyjson_mut_arr(doc);
        for (uint64_t id : event.topicIds) {
            yyjson_mut_arr_add_uint(doc, topic_ids, id);
        }
        yyjson_mut_obj_add_val(doc, event_obj, "topicIds", topic_ids);

        // Add subTopicIds array
        yyjson_mut_val *subtopic_ids = yyjson_mut_arr(doc);
        for (uint64_t id : event.subTopicIds) {
            yyjson_mut_arr_add_uint(doc, subtopic_ids, id);
        }
        yyjson_mut_obj_add_val(doc, event_obj, "subTopicIds", subtopic_ids);

        yyjson_mut_obj_add(doc, events_obj, key.c_str(), event_obj);
    }
    yyjson_mut_obj_add_val(doc, root, "events", events_obj);

    // Create performances array
    yyjson_mut_val *performances_array = yyjson_mut_arr(doc);
    for (const auto& perf : catalog.performances) {
        yyjson_mut_val *perf_obj = yyjson_mut_obj(doc);

        yyjson_mut_obj_add_uint(doc, perf_obj, "id", perf.id);
        yyjson_mut_obj_add_uint(doc, perf_obj, "eventId", perf.eventId);
        yyjson_mut_obj_add_uint(doc, perf_obj, "start", perf.start);
        yyjson_mut_obj_add_str(doc, perf_obj, "venueCode", perf.venueCode.c_str());
        yyjson_mut_obj_add_str(doc, perf_obj, "name", perf.name.c_str());

        yyjson_mut_arr_append(performances_array, perf_obj);
    }
    yyjson_mut_obj_add_val(doc, root, "performances", performances_array);

    // Helper to add string maps
    auto addStringMap = [&doc, &root](const char* key, const std::map<std::string, std::string>& map) {
        yyjson_mut_val *map_obj = yyjson_mut_obj(doc);
        for (const auto& [k, v] : map) {
            yyjson_mut_obj_add_str(doc, map_obj, k.c_str(), v.c_str());
        }
        yyjson_mut_obj_add_val(doc, root, key, map_obj);
    };

    addStringMap("areaNames", catalog.areaNames);
    addStringMap("audienceSubCategoryNames", catalog.audienceSubCategoryNames);
    addStringMap("blockNames", catalog.blockNames);
    addStringMap("seatCategoryNames", catalog.seatCategoryNames);
    addStringMap("subTopicNames", catalog.subTopicNames);
    addStringMap("subjectNames", catalog.subjectNames);
    addStringMap("topicNames", catalog.topicNames);
    addStringMap("venueNames", catalog.venueNames);

    // Write to string
    char *json_output = yyjson_mut_write(doc, 0, NULL);
    std::string result(json_output);
    free(json_output);
    yyjson_mut_doc_free(doc);

    return result;
}

#endif // YYJSON_CITM_CATALOG_DATA_H