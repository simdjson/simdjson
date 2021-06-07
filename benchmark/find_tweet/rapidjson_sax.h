#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "find_tweet.h"
#include <string.h>

namespace find_tweet {

using namespace rapidjson;

struct rapidjson_sax {
using StringType=std::string_view;

struct Handler {
        bool text_key = false;
        bool id_key = false;
        bool found_id = false;
        uint64_t find_id;
        std::string_view &result;

        Handler(std::string_view &r,uint64_t id): result(r), find_id(id) { }

        // We assume id is found before text
        bool Key(const char* key, SizeType length, bool copy) {
            if (found_id) { // If have found id, find text key
                if ((length == 4) && (memcmp(key,"text",4) == 0)) { text_key = true; }
            }
            else if ((length == 2) && (memcmp(key,"id",2) == 0)) { id_key = true; } // Otherwise, find id key
            return true;
        }
        bool Uint64(uint64_t i) {
            if (id_key && (i == find_id)) {  // If id key, check if id value matches find_id
                found_id = true;
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            if (text_key) {
                result = {str,length};
                return false;   // End parsing
            }
            return true;
        }
        // Irrelevant events
        bool Null() { return true; }
        bool Bool(bool b) { return true; }
        bool Double(double d) { return true; }
        bool Int(int i) { return true; }
        bool Int64(int64_t i) { return true; }
        bool Uint(unsigned i) { return true; }
        bool RawNumber(const char* str, SizeType length, bool copy) { return true; }
        bool StartObject() { return true; }
        bool EndObject(SizeType memberCount) { return true; }
        bool StartArray() { return true; }
        bool EndArray(SizeType elementCount) { return true; }
    }; // handler

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
        Reader reader;
        Handler handler(result,find_id);
        InsituStringStream ss(json.data());
        reader.Parse<kParseInsituFlag | kParseValidateEncodingFlag | kParseFullPrecisionFlag>(ss,handler);
        return true;
    }
}; // rapidjson_sax
BENCHMARK_TEMPLATE(find_tweet, rapidjson_sax)->UseManualTime();
} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_RAPIDJSON
