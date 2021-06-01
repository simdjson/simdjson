#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "partial_tweets.h"

namespace partial_tweets {

using json = nlohmann::json;

struct nlohmann_json_sax {
    using StringType=std::string;

    struct Handler : json::json_sax_t
    {
        // 8 keys to parse for each tweet (in order of appearance): "created_at", "id", "text", "in_reply_status_id", "id"(user),
        // "screen_name"(user), "retweet_count" and "favorite_count".
        // Assume that the first valid key encountered will be the correct key to parse.
        // Assume that each tweet/retweet start with a key "metadata" and has a key "retweeted" towards the end
        // The previous assumption will be used to check for the beginning of a new tweet and the end of a retweet
        bool keys[8] = { false };   // Stores if previous parsed key was a valid key (in same order as above)
        bool found[8] = { false }; // Stores if ith key has already been found once
        bool userobject_id = false; // If in a user object (to find user.id)
        bool userobject_screen_name = false;    // If in a user object (to find user.screen_name)
        bool inretweet = false; // If in a retweet (all keys irrelevant in retweet object)
        // Fields to store partial tweet info
        uint64_t user_id;
        uint64_t id;
        uint64_t rt;
        uint64_t fav;
        uint64_t reply_status;
        string_t screen_name;
        string_t date;
        string_t text;
        std::vector<tweet<std::string>>& result;

        Handler(std::vector<tweet<std::string>> &r) : result(r) { }

        bool key(string_t& val) override {
            if (!inretweet) {   // If not in a retweet object, find relevant keys
                if (val.compare("retweeted_status") == 0) { inretweet = true; }   // Check if entering retweet
                else if (val.compare("metadata") == 0) {    // Reset at beginning of tweet
                    found[0] = found[1] = found[2] = found[3] = found[4] = found[5] = found[6] = found[7] = found[8] = false;
                    keys[0] = keys[1] = keys[2] = keys[3] = keys[4] = keys[5] = keys[6] = keys[7] = keys[8] = false;
                }
                // Check if key has been found and if key matches a valid key
                else if (!found[0] && (val.compare("created_at") == 0)) { keys[0] = true; }
                // Must also check if not in a user object
                else if (!found[1] && !userobject_id && (val.compare("id") == 0)) { keys[1] = true; }
                else if (!found[2] && (val.compare("text") == 0)) { keys[2] = true; }
                else if (!found[3] && (val.compare("in_reply_to_status_id") == 0)) { keys[3] = true; }
                // Check if entering user object
                else if (!found[4] && (val.compare("user") == 0)) { userobject_id = userobject_screen_name = true; }
                // Must also check if in a user object
                else if (!found[5] && userobject_id && (val.compare("id") == 0)) { keys[4] = true; }
                // Must also check if in a user object
                else if (!found[6] && userobject_screen_name && (val.compare("screen_name") == 0)) { keys[5] = true; }
                else if (!found[7] && (val.compare("retweet_count") == 0)) { keys[6] = true; }
                else if (!found[8] && (val.compare("favorite_count") == 0)) { keys[7] = true; }
            }
            else if (val.compare("retweeted") == 0) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool number_unsigned(number_unsigned_t val) override {
           if (!found[4] && keys[4]) {    // user.id
                user_id = val;
                userobject_id = keys[4] = false;
                found[4] = true;
            }
            else if (!found[6] && keys[6]) {   // retweet_count
                rt = val;
                keys[6] = false;
                found[6] = true;
            }
            else if (!found[7] && keys[7]) {   // favorite_count
                fav = val;
                keys[7] = false;
                found[7] = true;
                // Assume that this is last key required, so add the partial_tweet to result
                result.emplace_back(partial_tweets::tweet<std::string>{
                date,id,text,reply_status,{user_id,screen_name},rt,fav});
            }
            else if (!found[1] && keys[1]) {    // id
                id = val;
                keys[1] = false;
                found[1] = true;
            }
            else if (!found[3] && keys[3]) {   // in_reply_status_id
                reply_status = val;
                found[3] = true;
                keys[3] = false;
            }
            return true;
        }
        bool string(string_t& val) override {
            if (!found[5] && keys[5]) {    // user.screen_name
                screen_name = val;
                userobject_screen_name = keys[5] = false;
                found[5] = true;
            }
            else if (!found[0] && keys[0]) {   //  created_at
                date = val;
                keys[0] = false;
                found[0] = true;
            }
            else if (!found[2] && keys[2]) {   // text
                text = val;
                keys[2] = false;
                found[2] = true;
            }
            return true;
        }
        bool null() override {
            if (!found[3] && keys[3]) {    // in_reply_status (null case)
                reply_status = 0;
                found[3] = true;
                keys[3] = false;
            }
            return true;
        }
        // Irrelevant events
        bool boolean(bool val) override { return true; }
        bool number_float(number_float_t val, const string_t& s) override { return true; }
        bool number_integer(number_integer_t val) override { return true; }
        bool start_object(std::size_t elements) override { return true; }
        bool end_object() override { return true; }
        bool start_array(std::size_t elements) override { return true; }
        bool end_array() override { return true; }
        bool binary(json::binary_t& val) override { return true; }
        bool parse_error(std::size_t position, const std::string& last_token, const json::exception& ex) override { return false; }
    }; // Handler

    bool run(simdjson::padded_string &json, std::vector<tweet<std::string>> &result) {
        Handler handler(result);
        json::sax_parse(json.data(), &handler);

        return true;
    }
}; // nlohmann_json_sax
BENCHMARK_TEMPLATE(partial_tweets, nlohmann_json_sax)->UseManualTime();
} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON