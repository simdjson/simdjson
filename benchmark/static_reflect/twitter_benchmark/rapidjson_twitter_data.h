#ifndef RAPIDJSON_TWITTER_DATA_H
#define RAPIDJSON_TWITTER_DATA_H

#include "twitter_data.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

// RapidJSON deserialization for simplified Twitter data
TwitterData rapidjson_deserialize(const std::string& json_str) {
    Document doc;
    doc.Parse(json_str.c_str());

    if (doc.HasParseError()) {
        throw std::runtime_error("RapidJSON parse error");
    }

    TwitterData data;

    if (!doc.HasMember("statuses") || !doc["statuses"].IsArray()) {
        return data;
    }

    const Value& statuses = doc["statuses"];
    data.statuses.reserve(statuses.Size());

    for (SizeType i = 0; i < statuses.Size(); i++) {
        const Value& status_json = statuses[i];
        Status status;

        // Parse status fields
        if (status_json.HasMember("created_at") && status_json["created_at"].IsString())
            status.created_at = status_json["created_at"].GetString();
        if (status_json.HasMember("id") && status_json["id"].IsUint64())
            status.id = status_json["id"].GetUint64();
        if (status_json.HasMember("text") && status_json["text"].IsString())
            status.text = status_json["text"].GetString();
        if (status_json.HasMember("retweet_count") && status_json["retweet_count"].IsUint64())
            status.retweet_count = status_json["retweet_count"].GetUint64();
        if (status_json.HasMember("favorite_count") && status_json["favorite_count"].IsUint64())
            status.favorite_count = status_json["favorite_count"].GetUint64();

        // Parse user
        if (status_json.HasMember("user") && status_json["user"].IsObject()) {
            const Value& user_json = status_json["user"];
            User user;

            if (user_json.HasMember("id") && user_json["id"].IsUint64())
                user.id = user_json["id"].GetUint64();
            if (user_json.HasMember("name") && user_json["name"].IsString())
                user.name = user_json["name"].GetString();
            if (user_json.HasMember("screen_name") && user_json["screen_name"].IsString())
                user.screen_name = user_json["screen_name"].GetString();
            if (user_json.HasMember("location") && user_json["location"].IsString())
                user.location = user_json["location"].GetString();
            if (user_json.HasMember("description") && user_json["description"].IsString())
                user.description = user_json["description"].GetString();
            if (user_json.HasMember("verified") && user_json["verified"].IsBool())
                user.verified = user_json["verified"].GetBool();
            if (user_json.HasMember("followers_count") && user_json["followers_count"].IsUint64())
                user.followers_count = user_json["followers_count"].GetUint64();
            if (user_json.HasMember("friends_count") && user_json["friends_count"].IsUint64())
                user.friends_count = user_json["friends_count"].GetUint64();
            if (user_json.HasMember("statuses_count") && user_json["statuses_count"].IsUint64())
                user.statuses_count = user_json["statuses_count"].GetUint64();

            status.user = user;
        }

        data.statuses.push_back(status);
    }

    return data;
}

// RapidJSON serialization for simplified Twitter data
std::string rapidjson_serialize(const TwitterData& data) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();

    Value statuses_array(kArrayType);

    for (const auto& status : data.statuses) {
        Value status_obj(kObjectType);

        Value created_at;
        created_at.SetString(status.created_at.c_str(), allocator);
        status_obj.AddMember("created_at", created_at, allocator);

        status_obj.AddMember("id", status.id, allocator);

        Value text;
        text.SetString(status.text.c_str(), allocator);
        status_obj.AddMember("text", text, allocator);

        // Add user
        Value user_obj(kObjectType);
        user_obj.AddMember("id", status.user.id, allocator);

        Value name;
        name.SetString(status.user.name.c_str(), allocator);
        user_obj.AddMember("name", name, allocator);

        Value screen_name;
        screen_name.SetString(status.user.screen_name.c_str(), allocator);
        user_obj.AddMember("screen_name", screen_name, allocator);

        Value location;
        location.SetString(status.user.location.c_str(), allocator);
        user_obj.AddMember("location", location, allocator);

        Value description;
        description.SetString(status.user.description.c_str(), allocator);
        user_obj.AddMember("description", description, allocator);

        user_obj.AddMember("verified", status.user.verified, allocator);
        user_obj.AddMember("followers_count", status.user.followers_count, allocator);
        user_obj.AddMember("friends_count", status.user.friends_count, allocator);
        user_obj.AddMember("statuses_count", status.user.statuses_count, allocator);

        status_obj.AddMember("user", user_obj, allocator);

        status_obj.AddMember("retweet_count", status.retweet_count, allocator);
        status_obj.AddMember("favorite_count", status.favorite_count, allocator);

        statuses_array.PushBack(status_obj, allocator);
    }

    doc.AddMember("statuses", statuses_array, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

#endif // RAPIDJSON_TWITTER_DATA_H