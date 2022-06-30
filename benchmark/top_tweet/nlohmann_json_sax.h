#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "top_tweet.h"

namespace top_tweet {

using json = nlohmann::json;

struct nlohmann_json_sax {
    using StringType=std::string;

    struct Handler : json::json_sax_t
    {
        // Assume every tweet/retweet starts with "metadata" key and ends with "retweeted" key. Ignore everything in a retweet.
        // Assume that the first valid key encountered outside a retweet is the correct key.
        enum state {    // Bitset to store state of search
            key_text = (1<<0),
            key_screen_name = (1<<1),
            key_rt = (1<<2),
            found_text = (1<<3),
            found_screen_name = (1<<4),
            found_rt = (1<<5)
        };
        int values = state::key_text;
        bool userobject = false;    // If in a user object
        bool inretweet = false;
        int64_t max_rt;
        int rt;
        string_t screen_name;
        string_t text;
        top_tweet_result<StringType>& result;

        Handler(top_tweet_result<StringType> &r,int64_t m) : result(r), max_rt(m) { }

        bool key(string_t& val) override {
            if (!inretweet) {   // If not in a retweet object, find relevant keys
                if (val.compare("retweeted_status") == 0) { inretweet = true; }   // Check if entering retweet
                else if (val.compare("metadata") == 0) { values = 0; }  // Reset
                else if (!(values & found_text) && (val.compare("text") == 0)) { values |= (key_text); }
                else if ((val.compare("user") == 0)) { userobject = true; }
                else if (!(values & found_screen_name) && userobject && (val.compare("screen_name") == 0)) { values |= (key_screen_name); }
                else if (!(values & found_rt) && (val.compare("retweet_count") == 0)) { values |= (key_rt); }
            }
            else if (val.compare("retweeted") == 0) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool number_unsigned(number_unsigned_t val) override {
            if (values & key_rt && !(values & found_rt)) {   // retweet_count
                rt = int(val);
                values &= ~(key_rt);
                values |= (found_rt);
                if (rt <= max_rt && rt >= result.retweet_count) {    // Check if current tweet has more retweet than previous top tweet
                    result.retweet_count = rt;
                    result.text = text;
                    result.screen_name  = screen_name;
                }
            }
            return true;
        }
        bool string(string_t& val) override {
            if (values & key_text && !(values & found_text)) {   // text
                text = val;
                values &= ~(key_text);
                values |= (found_text);
            }
            else if (values & key_screen_name && !(values & found_screen_name)) {    // user.screen_name
                screen_name = val;
                userobject = false;
                values &= ~(key_screen_name);
                values |= (found_screen_name);
            }
            return true;
        }
        // Irrelevant events
        bool null() override { return true; }
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

    bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
        result.retweet_count = -1;
        Handler handler(result,max_retweet_count);
        json::sax_parse(json.data(), &handler);
        return true;
    }
}; // nlohmann_json_sax
BENCHMARK_TEMPLATE(top_tweet, nlohmann_json_sax)->UseManualTime();
} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON