#ifndef RAPIDJSON_CITM_CATALOG_DATA_H
#define RAPIDJSON_CITM_CATALOG_DATA_H

#include "citm_catalog_data.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

// RapidJSON deserialization for CITM Catalog data
CitmCatalog rapidjson_deserialize_citm(const std::string& json_str) {
    Document doc;
    doc.Parse(json_str.c_str());

    if (doc.HasParseError()) {
        throw std::runtime_error("RapidJSON parse error");
    }

    CitmCatalog catalog;

    // Parse events
    if (doc.HasMember("events") && doc["events"].IsObject()) {
        const Value& events = doc["events"];
        for (auto it = events.MemberBegin(); it != events.MemberEnd(); ++it) {
            Event event;
            const Value& ev = it->value;

            if (ev.HasMember("description") && ev["description"].IsString())
                event.description = ev["description"].GetString();
            if (ev.HasMember("id") && ev["id"].IsUint64())
                event.id = ev["id"].GetUint64();
            if (ev.HasMember("logo") && ev["logo"].IsString())
                event.logo = ev["logo"].GetString();
            if (ev.HasMember("name") && ev["name"].IsString())
                event.name = ev["name"].GetString();
            if (ev.HasMember("subjectCode") && ev["subjectCode"].IsString())
                event.subjectCode = ev["subjectCode"].GetString();
            if (ev.HasMember("subtitle") && ev["subtitle"].IsString())
                event.subtitle = ev["subtitle"].GetString();

            if (ev.HasMember("topicIds") && ev["topicIds"].IsArray()) {
                const Value& topics = ev["topicIds"];
                for (SizeType j = 0; j < topics.Size(); j++) {
                    if (topics[j].IsUint64())
                        event.topicIds.push_back(topics[j].GetUint64());
                }
            }

            if (ev.HasMember("subTopicIds") && ev["subTopicIds"].IsArray()) {
                const Value& subtopics = ev["subTopicIds"];
                for (SizeType j = 0; j < subtopics.Size(); j++) {
                    if (subtopics[j].IsUint64())
                        event.subTopicIds.push_back(subtopics[j].GetUint64());
                }
            }

            catalog.events[it->name.GetString()] = event;
        }
    }

    // Parse performances
    if (doc.HasMember("performances") && doc["performances"].IsArray()) {
        const Value& performances = doc["performances"];
        for (SizeType i = 0; i < performances.Size(); i++) {
            Performance perf;
            const Value& p = performances[i];

            if (p.HasMember("id") && p["id"].IsUint64())
                perf.id = p["id"].GetUint64();
            if (p.HasMember("eventId") && p["eventId"].IsUint64())
                perf.eventId = p["eventId"].GetUint64();
            if (p.HasMember("start") && p["start"].IsUint64())
                perf.start = p["start"].GetUint64();
            if (p.HasMember("venueCode") && p["venueCode"].IsString())
                perf.venueCode = p["venueCode"].GetString();
            if (p.HasMember("name") && p["name"].IsString())
                perf.name = p["name"].GetString();

            catalog.performances.push_back(perf);
        }
    }

    // Parse other string maps
    auto parseStringMap = [&doc](const char* key, std::map<std::string, std::string>& target) {
        if (doc.HasMember(key) && doc[key].IsObject()) {
            const Value& obj = doc[key];
            for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
                if (it->value.IsString()) {
                    target[it->name.GetString()] = it->value.GetString();
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

    return catalog;
}

// RapidJSON serialization for CITM Catalog data
std::string rapidjson_serialize_citm(const CitmCatalog& catalog) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();

    // Serialize events
    Value events_obj(kObjectType);
    for (const auto& [key, event] : catalog.events) {
        Value event_obj(kObjectType);

        Value desc;
        desc.SetString(event.description.c_str(), allocator);
        event_obj.AddMember("description", desc, allocator);

        event_obj.AddMember("id", event.id, allocator);

        Value logo;
        logo.SetString(event.logo.c_str(), allocator);
        event_obj.AddMember("logo", logo, allocator);

        Value name;
        name.SetString(event.name.c_str(), allocator);
        event_obj.AddMember("name", name, allocator);

        Value subject;
        subject.SetString(event.subjectCode.c_str(), allocator);
        event_obj.AddMember("subjectCode", subject, allocator);

        Value subtitle;
        subtitle.SetString(event.subtitle.c_str(), allocator);
        event_obj.AddMember("subtitle", subtitle, allocator);

        Value topicIds(kArrayType);
        for (uint64_t id : event.topicIds) {
            topicIds.PushBack(id, allocator);
        }
        event_obj.AddMember("topicIds", topicIds, allocator);

        Value subTopicIds(kArrayType);
        for (uint64_t id : event.subTopicIds) {
            subTopicIds.PushBack(id, allocator);
        }
        event_obj.AddMember("subTopicIds", subTopicIds, allocator);

        Value key_val;
        key_val.SetString(key.c_str(), allocator);
        events_obj.AddMember(key_val, event_obj, allocator);
    }
    doc.AddMember("events", events_obj, allocator);

    // Serialize performances
    Value performances_array(kArrayType);
    for (const auto& perf : catalog.performances) {
        Value perf_obj(kObjectType);
        perf_obj.AddMember("id", perf.id, allocator);
        perf_obj.AddMember("eventId", perf.eventId, allocator);
        perf_obj.AddMember("start", perf.start, allocator);

        Value venue;
        venue.SetString(perf.venueCode.c_str(), allocator);
        perf_obj.AddMember("venueCode", venue, allocator);

        Value name;
        name.SetString(perf.name.c_str(), allocator);
        perf_obj.AddMember("name", name, allocator);

        performances_array.PushBack(perf_obj, allocator);
    }
    doc.AddMember("performances", performances_array, allocator);

    // Helper to serialize string maps
    auto addStringMap = [&doc, &allocator](const char* key, const std::map<std::string, std::string>& map) {
        Value map_obj(kObjectType);
        for (const auto& [k, v] : map) {
            Value key_val, val_val;
            key_val.SetString(k.c_str(), allocator);
            val_val.SetString(v.c_str(), allocator);
            map_obj.AddMember(key_val, val_val, allocator);
        }
        Value key_name;
        key_name.SetString(key, allocator);
        doc.AddMember(key_name, map_obj, allocator);
    };

    addStringMap("areaNames", catalog.areaNames);
    addStringMap("audienceSubCategoryNames", catalog.audienceSubCategoryNames);
    addStringMap("blockNames", catalog.blockNames);
    addStringMap("seatCategoryNames", catalog.seatCategoryNames);
    addStringMap("subTopicNames", catalog.subTopicNames);
    addStringMap("subjectNames", catalog.subjectNames);
    addStringMap("topicNames", catalog.topicNames);
    addStringMap("venueNames", catalog.venueNames);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

#endif // RAPIDJSON_CITM_CATALOG_DATA_H