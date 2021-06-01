#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "partial_tweets.h"
#include <string.h>
#include <fstream>

namespace partial_tweets {

using namespace rapidjson;

struct rapidjson_sax {
    using StringType=std::string_view;

    struct Handler {
        bool keys[9] = { false };
        bool values[9] = { false };
        uint64_t user_id = 0;
        uint64_t id = 0;
        uint64_t rt = 0;
        uint64_t fav = 0;
        uint64_t reply_status = 0;
        bool inretweet = false;
        char* name;
        char* date;
        char* text;
        std::vector<tweet<std::string_view>>& result;

        Handler(std::vector<tweet<std::string_view>> &r) : result(r) { }

        bool Key(const char* key, SizeType length, bool copy) {
            if (!inretweet) {
                if (strcmp(key,"retweeted_status") == 0) { inretweet = true; }
                else if (strcmp(key,"metadata") == 0) {
                    values[0] = values[1] = values[2] = values[3] = values[4] = values[5] = values[6] = values[7] = values[8] = false;
                    keys[0] = keys[1] = keys[2] = keys[3] = keys[4] = keys[5] = keys[6] = keys[7] = keys[8] = false;
                }
                else if (strcmp(key,"user") == 0) { keys[0] = true; }
                else if (keys[0] && strcmp(key,"id") == 0) { keys[1] = true; }
                else if (keys[0] && strcmp(key,"screen_name") == 0) { keys[2] = true; }
                else if (strcmp(key,"created_at") == 0) { keys[3] = true; }
                else if (strcmp(key,"id") == 0) { keys[4] = true; }
                else if (strcmp(key,"text") == 0) { keys[5] = true; }
                else if (strcmp(key,"in_reply_to_status_id") == 0) { keys[6] = true; }
                else if (strcmp(key,"retweet_count") == 0) { keys[7] = true; }
                else if (strcmp(key,"favorite_count") == 0) { keys[8] = true; }
            }

            // Assume that a tweet/retweet always finishes with a unique key "retweeted"
            else if (strcmp(key,"retweeted") == 0) { inretweet = false; }
            return true;
        }
        bool Uint(unsigned i) {
            if (!values[1] && keys[1]) {
                user_id = i;
                //std::cout << user_id << std::endl;
                keys[1] = false;
                values[1] = true;
            }
            else if (!values[7] && keys[7]) {
                rt = i;
                //std::cout << rt << std::endl;
                keys[7] = false;
                values[7] = true;
            }
            else if (!values[8] && keys[8]) {
                fav = i;
                //std::cout << fav << std::endl;
                keys[8] = false;
                values[8] = true;

                // Reset
                result.emplace_back(partial_tweets::tweet<std::string_view>{
                date,id,text,reply_status,{user_id,name},rt,fav});
                //std::cout << date << ' ' << id << std::endl << text << std::endl << reply_status << ' ' << user_id << ' ' << name << std::endl;
                //std::cout << rt << ' ' << fav << std::endl;
            }
            return true;
        }
        bool Uint64(uint64_t i) {
            if (!values[4] && keys[4]) {
                id = i;
                //std::cout << i << std::endl;
                keys[4] = false;
                values[4] = true;
            }
            else if (!values[6] && keys[6]) {
                reply_status = i;
                //std::cout << i << std::endl;
                values[6] = true;
                keys[6] = false;
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            if (!values[2] && keys[2]) {
                name = strdup(str);
                //std::cout << name << std::endl;
                keys[0] = keys[2] = false;
                values[2] = true;
            }
            else if (!values[3] && keys[3]) {
                date = strdup(str);
                //std::cout << date << std::endl;
                keys[3] = false;
                values[3] = true;
            }
            else if (!values[5] && keys[5]) {
                //text = str;
                text = strdup(str);
                //std::cout << text << std::endl;
                keys[5] = false;
                values[5] = true;
            }
            return true;
        }
        bool Null() {
            if (!values[6] && keys[6]) {
                reply_status = 0;
                //std::cout << reply_status << std::endl;
                values[6] = true;
                keys[6] = false;
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
        StringStream ss(json.data());
        reader.Parse(ss,handler);
        return true;
    }

}; // rapid_jason_sax
BENCHMARK_TEMPLATE(partial_tweets, rapidjson_sax)->UseManualTime();
} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_RAPIDJSON