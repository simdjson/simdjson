#ifndef YYJSON_TWITTER_DATA_H
#define YYJSON_TWITTER_DATA_H

#include "twitter_data.h"
#include <yyjson.h>
#include <string>
#include <stdexcept>

// yyjson deserialization for simplified Twitter data
TwitterData yyjson_deserialize(const std::string &json_str) {
    TwitterData data;

    yyjson_doc *doc = yyjson_read(json_str.c_str(), json_str.size(), 0);
    if (!doc) {
        throw std::runtime_error("yyjson parse error");
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root) {
        yyjson_doc_free(doc);
        return data;
    }

    // Get statuses array
    yyjson_val *statuses_val = yyjson_obj_get(root, "statuses");
    if (!statuses_val || !yyjson_is_arr(statuses_val)) {
        yyjson_doc_free(doc);
        return data;
    }

    size_t idx, max;
    yyjson_val *status_val;
    yyjson_arr_foreach(statuses_val, idx, max, status_val) {
        Status status;

        // Parse status fields
        yyjson_val *val;

        val = yyjson_obj_get(status_val, "created_at");
        if (val && yyjson_is_str(val)) status.created_at = yyjson_get_str(val);

        val = yyjson_obj_get(status_val, "id");
        if (val && yyjson_is_uint(val)) status.id = yyjson_get_uint(val);

        val = yyjson_obj_get(status_val, "text");
        if (val && yyjson_is_str(val)) status.text = yyjson_get_str(val);

        val = yyjson_obj_get(status_val, "retweet_count");
        if (val && yyjson_is_uint(val)) status.retweet_count = yyjson_get_uint(val);

        val = yyjson_obj_get(status_val, "favorite_count");
        if (val && yyjson_is_uint(val)) status.favorite_count = yyjson_get_uint(val);

        // Parse user
        yyjson_val *user_val = yyjson_obj_get(status_val, "user");
        if (user_val && yyjson_is_obj(user_val)) {
            User user;

            val = yyjson_obj_get(user_val, "id");
            if (val && yyjson_is_uint(val)) user.id = yyjson_get_uint(val);

            val = yyjson_obj_get(user_val, "name");
            if (val && yyjson_is_str(val)) user.name = yyjson_get_str(val);

            val = yyjson_obj_get(user_val, "screen_name");
            if (val && yyjson_is_str(val)) user.screen_name = yyjson_get_str(val);

            val = yyjson_obj_get(user_val, "location");
            if (val && yyjson_is_str(val)) user.location = yyjson_get_str(val);

            val = yyjson_obj_get(user_val, "description");
            if (val && yyjson_is_str(val)) user.description = yyjson_get_str(val);

            val = yyjson_obj_get(user_val, "verified");
            if (val && yyjson_is_bool(val)) user.verified = yyjson_get_bool(val);

            val = yyjson_obj_get(user_val, "followers_count");
            if (val && yyjson_is_uint(val)) user.followers_count = yyjson_get_uint(val);

            val = yyjson_obj_get(user_val, "friends_count");
            if (val && yyjson_is_uint(val)) user.friends_count = yyjson_get_uint(val);

            val = yyjson_obj_get(user_val, "statuses_count");
            if (val && yyjson_is_uint(val)) user.statuses_count = yyjson_get_uint(val);

            status.user = user;
        }

        data.statuses.push_back(status);
    }

    yyjson_doc_free(doc);
    return data;
}

// yyjson serialization for simplified Twitter data
std::string yyjson_serialize(const TwitterData &data) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    // Create statuses array
    yyjson_mut_val *statuses_array = yyjson_mut_arr(doc);

    for (const auto& status : data.statuses) {
        yyjson_mut_val *status_obj = yyjson_mut_obj(doc);

        // Add status fields
        yyjson_mut_obj_add_str(doc, status_obj, "created_at", status.created_at.c_str());
        yyjson_mut_obj_add_uint(doc, status_obj, "id", status.id);
        yyjson_mut_obj_add_str(doc, status_obj, "text", status.text.c_str());

        // User object
        yyjson_mut_val *user_obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_uint(doc, user_obj, "id", status.user.id);
        yyjson_mut_obj_add_str(doc, user_obj, "name", status.user.name.c_str());
        yyjson_mut_obj_add_str(doc, user_obj, "screen_name", status.user.screen_name.c_str());
        yyjson_mut_obj_add_str(doc, user_obj, "location", status.user.location.c_str());
        yyjson_mut_obj_add_str(doc, user_obj, "description", status.user.description.c_str());
        yyjson_mut_obj_add_bool(doc, user_obj, "verified", status.user.verified);
        yyjson_mut_obj_add_uint(doc, user_obj, "followers_count", status.user.followers_count);
        yyjson_mut_obj_add_uint(doc, user_obj, "friends_count", status.user.friends_count);
        yyjson_mut_obj_add_uint(doc, user_obj, "statuses_count", status.user.statuses_count);
        yyjson_mut_obj_add_val(doc, status_obj, "user", user_obj);

        // Other fields
        yyjson_mut_obj_add_uint(doc, status_obj, "retweet_count", status.retweet_count);
        yyjson_mut_obj_add_uint(doc, status_obj, "favorite_count", status.favorite_count);

        yyjson_mut_arr_append(statuses_array, status_obj);
    }

    // Add statuses array to root
    yyjson_mut_obj_add_val(doc, root, "statuses", statuses_array);

    // Write to string
    char *json_output = yyjson_mut_write(doc, 0, NULL);
    std::string result(json_output);
    free(json_output);
    yyjson_mut_doc_free(doc);

    return result;
}

#endif // YYJSON_TWITTER_DATA_H