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
        enum state {    // Bitset to store state of search
            key_date = (1<<0),
            key_id = (1<<1),
            key_text = (1<<2),
            key_reply = (1<<3),
            key_userid = (1<<4),
            key_screenname = (1<<5),
            key_rt = (1<<6),
            key_fav = (1<<7),
            found_date = (1<<8),
            found_id = (1<<9),
            found_text = (1<<10),
            found_reply = (1<<11),
            found_userid = (1<<12),
            found_screenname = (1<<13),
            found_rt = (1<<14),
            found_fav = (1<<15)
        };
        int values = state::key_date;
        bool userobject_id = false; // If in a user object (to find user.id)
        bool userobject_screen_name = false;    // If in a user object (to find user.screen_name)
        bool inretweet = false; // If in a retweet (all keys irrelevant in retweet object)
        // Fields to store partial tweet info
        uint64_t user_id;
        uint64_t id;
        uint64_t rt;
        uint64_t fav;
        uint64_t reply_status;
        std::string_view screen_name;
        std::string_view date;
        std::string_view text;
        std::vector<tweet<std::string_view>>& result;

        Handler(std::vector<tweet<std::string_view>> &r) : result(r) { }

        bool Key(const char* key, SizeType length, bool copy) {
            if (!inretweet) {   // If not in a retweet object, find relevant keys
                if ((length == 16) && (memcmp(key,"retweeted_status",16) == 0)) { inretweet = true; }   // Check if entering retweet
                else if ((length == 8) && (memcmp(key,"metadata",8) == 0)) { values = 0; }  // Reset
                // Check if key has been found and if key matches a valid key
                else if (!(values & found_date) && (length == 10) && (memcmp(key,"created_at",10) == 0)) { values |= (key_date); }
                // Must also check if not in a user object
                else if (!(values & found_id) && !userobject_id && (length == 2) && (memcmp(key,"id",2) == 0)) { values |= (key_id); }
                else if (!(values & found_text) && (length == 4) && (memcmp(key,"text",4) == 0)) { values |= (key_text); }
                else if (!(values & found_reply) && (length == 21) && (memcmp(key,"in_reply_to_status_id",21) == 0)) { values |= (key_reply); }
                // Check if entering user object
                else if ((length == 4) && (memcmp(key,"user",4) == 0)) { userobject_id = userobject_screen_name = true; }
                // Must also check if in a user object
                else if (!(values & found_userid) && userobject_id && (length == 2) && (memcmp(key,"id",2) == 0)) { values |= (key_userid); }
                // Must also check if in a user object
                else if (!(values & found_screenname) && userobject_screen_name && (length == 11) && (memcmp(key,"screen_name",11) == 0)) { values |= (key_screenname); }
                else if (!(values & found_rt) && (length == 13) && (memcmp(key,"retweet_count",13) == 0)) { values |= (key_rt); }
                else if (!(values & found_fav) && (length == 14) && (memcmp(key,"favorite_count",14) == 0)) { values |= (key_fav); }
            }
            else if ((length == 9) && (memcmp(key,"retweeted",9) == 0)) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool Uint(unsigned i) {
            if (values & key_userid && !(values & found_userid)) {    // user.id
                user_id = i;
                userobject_id = false;
                values &= ~(key_userid);
                values |= (found_userid);
            }
            else if (values & key_rt && !(values & found_rt)) {   // retweet_count
                rt = i;
                values &= ~(key_rt);
                values |= (found_rt);
            }
            else if (values & key_fav && !(values & found_fav)) {   // favorite_count
                fav = i;
                values &= ~(key_fav);
                values |= (found_fav);
                // Assume that this is last key required, so add the partial_tweet to result
                result.emplace_back(partial_tweets::tweet<std::string_view>{
                date,id,text,reply_status,{user_id,screen_name},rt,fav});
            }
            return true;
        }
        bool Uint64(uint64_t i) {
            if (values & key_id && !(values & found_id)) {    // id
                id = i;
                values &= ~(key_id);
                values |= (found_id);
            }
            else if (values & key_reply && !(values & found_reply)) {   // in_reply_status_id
                reply_status = i;
                values &= ~(key_reply);
                values |= (found_reply);
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            if (values & key_date && !(values & found_date)) {   //  created_at
                date = {str,length};
                values &= ~(key_date);
                values |= (found_date);
            }
            else if (values & key_text && !(values & found_text)) {   // text
                text = {str,length};
                values &= ~(key_text);
                values |= (found_text);
            }
            else if (values & key_screenname && !(values & found_screenname)) {    // user.screen_name
                screen_name = {str,length};
                userobject_screen_name = false;
                values &= ~(key_screenname);
                values |= (found_screenname);
            }
            return true;
        }
        bool Null() {
            if (values & key_reply && !(values & found_reply)) {    // in_reply_status (null case)
                reply_status = 0;
                values &= ~(key_reply);
                values |= (found_reply);
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