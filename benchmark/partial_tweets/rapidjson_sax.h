#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "partial_tweets.h"
#include <string.h>
#include <fstream>

namespace partial_tweets {

using namespace rapidjson;

struct rapidjson_sax {
    using StringType=std::string_view;

    // 8 keys to parse for each tweet (in order of appearance): "created_at", "id", "text", "in_reply_status_id", "id"(user),
    // "screen_name"(user), "retweet_count" and "favorite_count".
    // Assume that the first valid key encountered will be the correct key to parse.
    // Assume that each tweet/retweet start with a key "metadata" and has a key "retweeted" towards the end
    // The previous assumption will be used to check for the beginning of a new tweet and the end of a retweet
    struct Handler {
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
        char* screen_name;
        char* date;
        char* text;
        std::vector<tweet<std::string_view>>& result;

        Handler(std::vector<tweet<std::string_view>> &r) : result(r) { }

        bool Key(const char* key, SizeType length, bool copy) {
            if (!inretweet) {   // If not in a retweet object, find relevant keys
                if ((length == 16) && (memcmp(key,"retweeted_status",16) == 0)) { inretweet = true; }   // Check if entering retweet
                else if ((length == 8) && (memcmp(key,"metadata",8) == 0)) {    // Reset at beginning of tweet
                    found[0] = found[1] = found[2] = found[3] = found[4] = found[5] = found[6] = found[7] = found[8] = false;
                    keys[0] = keys[1] = keys[2] = keys[3] = keys[4] = keys[5] = keys[6] = keys[7] = keys[8] = false;
                }
                // Check if key has been found and if key matches a valid key
                else if (!found[0] && (length == 10) && (memcmp(key,"created_at",10) == 0)) { keys[0] = true; }
                // Must also check if not in a user object
                else if (!found[1] && !userobject_id && (length == 2) && (memcmp(key,"id",2) == 0)) { keys[1] = true; }
                else if (!found[2] && (length == 4) && (memcmp(key,"text",4) == 0)) { keys[2] = true; }
                else if (!found[3] && (length == 21) && (memcmp(key,"in_reply_to_status_id",21) == 0)) { keys[3] = true; }
                // Check if entering user object
                else if (!found[4] && (length == 4) && (memcmp(key,"user",4) == 0)) { userobject_id = userobject_screen_name = true; }
                // Must also check if in a user object
                else if (!found[5] && userobject_id && (length == 2) && (memcmp(key,"id",2) == 0)) { keys[4] = true; }
                // Must also check if in a user object
                else if (!found[6] && userobject_screen_name && (length == 11) && (memcmp(key,"screen_name",11) == 0)) { keys[5] = true; }
                else if (!found[7] && (length == 13) && (memcmp(key,"retweet_count",13) == 0)) { keys[6] = true; }
                else if (!found[8] && (length == 14) && (memcmp(key,"favorite_count",14) == 0)) { keys[7] = true; }
            }
            else if ((length == 9) && (memcmp(key,"retweeted",9) == 0)) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool Uint(unsigned i) {
            if (!found[4] && keys[4]) {    // user.id
                user_id = i;
                userobject_id = keys[4] = false;
                found[4] = true;
            }
            else if (!found[6] && keys[6]) {   // retweet_count
                rt = i;
                keys[6] = false;
                found[6] = true;
            }
            else if (!found[7] && keys[7]) {   // favorite_count
                fav = i;
                keys[7] = false;
                found[7] = true;
                // Assume that this is last key required, so add the partial_tweet to result
                result.emplace_back(partial_tweets::tweet<std::string_view>{
                date,id,text,reply_status,{user_id,screen_name},rt,fav});
            }
            return true;
        }
        bool Uint64(uint64_t i) {
            if (!found[1] && keys[1]) {    // id
                id = i;
                keys[1] = false;
                found[1] = true;
            }
            else if (!found[3] && keys[3]) {   // in_reply_status_id
                reply_status = i;
                found[3] = true;
                keys[3] = false;
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            if (!found[5] && keys[5]) {    // user.screen_name
                screen_name = strdup(str);
                userobject_screen_name = keys[5] = false;
                found[5] = true;
            }
            else if (!found[0] && keys[0]) {   //  created_at
                date = strdup(str);
                keys[0] = false;
                found[0] = true;
            }
            else if (!found[2] && keys[2]) {   // text
                text = strdup(str);
                keys[2] = false;
                found[2] = true;
            }
            return true;
        }
        bool Null() {
            if (!found[3] && keys[3]) {    // in_reply_status (null case)
                reply_status = 0;
                found[3] = true;
                keys[3] = false;
            }
            return true;
        }
        // Irrelevant events
        bool Bool(bool b) { return true; }
        bool Double(double d) { return true; }
        bool Int(int i) { return true; }
        bool Int64(int64_t i) { return true; }
        bool RawNumber(const char* str, SizeType length, bool copy) { return true; }
        bool StartObject() { return true; }
        bool EndObject(SizeType memberCount) { return true; }
        bool StartArray() { return true; }
        bool EndArray(SizeType elementCount) { return true; }
    }; // handler

    bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
        Reader reader;
        Handler handler(result);
        InsituStringStream ss(json.data());
        reader.Parse<kParseInsituFlag | kParseValidateEncodingFlag | kParseFullPrecisionFlag>(ss,handler);
        return true;
    }

}; // rapid_jason_sax
BENCHMARK_TEMPLATE(partial_tweets, rapidjson_sax)->UseManualTime();
} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_RAPIDJSON