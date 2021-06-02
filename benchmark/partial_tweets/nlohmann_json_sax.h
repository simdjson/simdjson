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
        string_t screen_name;
        string_t date;
        string_t text;
        std::vector<tweet<std::string>>& result;

        Handler(std::vector<tweet<std::string>> &r) : result(r) { }

        bool key(string_t& val) override {
            if (!inretweet) {   // If not in a retweet object, find relevant keys
                if (val.compare("retweeted_status") == 0) { inretweet = true; }   // Check if entering retweet
                else if (val.compare("metadata") == 0) { values = 0; }  // Reset
                // Check if key has been found and if key matches a valid key
                else if (!(values & found_date) && (val.compare("created_at") == 0)) { values |= (key_date); }
                // Must also check if not in a user object
                else if (!(values & found_id) && !userobject_id && (val.compare("id") == 0)) { values |= (key_id); }
                else if (!(values & found_text) && (val.compare("text") == 0)) { values |= (key_text); }
                else if (!(values & found_reply) && (val.compare("in_reply_to_status_id") == 0)) { values |= (key_reply); }
                // Check if entering user object
                else if ((val.compare("user") == 0)) { userobject_id = userobject_screen_name = true; }
                // Must also check if in a user object
                else if (!(values & found_userid) && userobject_id && (val.compare("id") == 0)) { values |= (key_userid); }
                // Must also check if in a user object
                else if (!(values & found_screenname) && userobject_screen_name && (val.compare("screen_name") == 0)) { values |= (key_screenname); }
                else if (!(values & found_rt) && (val.compare("retweet_count") == 0)) { values |= (key_rt); }
                else if (!(values & found_fav) && (val.compare("favorite_count") == 0)) { values |= (key_fav); }
            }
            else if (val.compare("retweeted") == 0) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool number_unsigned(number_unsigned_t val) override {
            if (values & key_id && !(values & found_id)) {    // id
                id = val;
                values &= ~(key_id);
                values |= (found_id);
            }
            else if (values & key_reply && !(values & found_reply)) {   // in_reply_status_id
                reply_status = val;
                values &= ~(key_reply);
                values |= (found_reply);
            }
            else if (values & key_userid && !(values & found_userid)) {    // user.id
                user_id = val;
                userobject_id = false;
                values &= ~(key_userid);
                values |= (found_userid);
            }
            else if (values & key_rt && !(values & found_rt)) {   // retweet_count
                rt = val;
                values &= ~(key_rt);
                values |= (found_rt);
            }
            else if (values & key_fav && !(values & found_fav)) {   // favorite_count
                fav = val;
                values &= ~(key_fav);
                values |= (found_fav);
                // Assume that this is last key required, so add the partial_tweet to result
                result.emplace_back(partial_tweets::tweet<std::string>{
                date,id,text,reply_status,{user_id,screen_name},rt,fav});
            }
            return true;
        }
        bool string(string_t& val) override {
            if (values & key_date && !(values & found_date)) {   //  created_at
                date = val;
                values &= ~(key_date);
                values |= (found_date);
            }
            else if (values & key_text && !(values & found_text)) {   // text
                text = val;
                values &= ~(key_text);
                values |= (found_text);
            }
            else if (values & key_screenname && !(values & found_screenname)) {    // user.screen_name
                screen_name = val;
                userobject_screen_name = false;
                values &= ~(key_screenname);
                values |= (found_screenname);
            }
            return true;
        }
        bool null() override {
            if (values & key_reply && !(values & found_reply)) {    // in_reply_status (null case)
                reply_status = 0;
                values &= ~(key_reply);
                values |= (found_reply);
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