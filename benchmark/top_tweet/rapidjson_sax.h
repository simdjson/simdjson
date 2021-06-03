#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "top_tweet.h"
#include <string.h>

namespace top_tweet {

using namespace rapidjson;

struct rapidjson_sax {
    using StringType=std::string_view;
    struct Handler {
        // Assume every tweet/retweet starts with "metadata" key and ends with "retweeted" key. Ignore everything in a retweet.
        // Assume that the first valid key encountered outside a retweet is the correct key.
        enum state {    // Bit set to keep track of state of search for keys
            key_text = (1<<0),
            key_screen_name = (1<<1),
            key_rt = (1<<2),
            found_text = (1<<3),
            found_screen_name = (1<<4),
            found_rt = (1<<5)
        };
        int values = state::key_text;
        int rt;
        StringType text;
        StringType screen_name;
        bool inretweet = false;
        bool userobject = false;
        top_tweet_result<StringType>& result;
        int64_t max_rt;

        Handler(top_tweet_result<StringType> &r,int64_t m) : result(r), max_rt(m) { }

        bool Key(const char* key, SizeType length, bool copy) {
            if (!inretweet) {
                if ((length == 16) && (memcmp(key,"retweeted_status",16) == 0)) { inretweet = true; }   // Check if entering retweet
                else if ((length == 8) && (memcmp(key,"metadata",8) == 0)) { values = 0; }  // Reset
                else if (!(values & found_text) && (length == 4) && (memcmp(key,"text",4) == 0)) { values |= (key_text); }
                else if ((length == 4) && (memcmp(key,"user",4) == 0)) { userobject = true; }
                else if (!(values & found_screen_name) && userobject && (length == 11) && memcmp(key,"screen_name",11) == 0) { values |= (key_screen_name); }
                else if (!(values & found_rt) && (length == 13) && (memcmp(key,"retweet_count",13) == 0)) { values |= (key_rt); }
            }
            else if ((length == 9) && (memcmp(key,"retweeted",9) == 0)) { inretweet = false; }  // Check if end of retweet
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            if (values & key_text && !(values & found_text)) {   // text
                text = {str,length};
                values &= ~(key_text);
                values |= (found_text);
            }
            else if (values & key_screen_name && !(values & found_screen_name)) {    // user.screen_name
                screen_name = {str,length};
                values &= ~(key_screen_name);
                values |= (found_screen_name);
                userobject = false;
            }
            return true;
        }
        bool Uint(unsigned i) {
            if (values & key_rt && !(values & found_rt)) {   // retweet_count
                rt = i;
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
        // Irrelevant events
        bool Null() { return true; }
        bool Bool(bool b) { return true; }
        bool Double(double d) { return true; }
        bool Int(int i) { return true; }
        bool Int64(int64_t i) { return true; }
        bool Uint64(uint64_t i) { return true; }
        bool RawNumber(const char* str, SizeType length, bool copy) { return true; }
        bool StartObject() { return true; }
        bool EndObject(SizeType memberCount) { return true; }
        bool StartArray() { return true; }
        bool EndArray(SizeType elementCount) { return true; }
    }; // handler

    bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
        result.retweet_count = -1;
        Reader reader;
        Handler handler(result,max_retweet_count);
        InsituStringStream ss(json.data());
        reader.Parse<kParseInsituFlag | kParseValidateEncodingFlag | kParseFullPrecisionFlag>(ss,handler);
        return true;
    }
}; // rapidjson_sax
BENCHMARK_TEMPLATE(top_tweet, rapidjson_sax)->UseManualTime();
} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_RAPIDJSON
