#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "distinct_user_id.h"
#include <string.h>
namespace distinct_user_id {

using namespace rapidjson;

struct rapidjson_sax {
    struct Handler {
        std::vector<uint64_t>& result;
        bool user = false;
        bool user_id = false;
        Handler(std::vector<uint64_t> &r) : result(r) { }

        bool Key(const char* key, SizeType length, bool copy) {
            // Assume that valid user/id pairs appear only once in main array of user objects
            if (user) { // If already found user object, find id key
                if ((length == 2) && memcmp(key,"id",2) == 0) { user_id = true; }
            }
            else if ((length == 4) && memcmp(key,"user",4) == 0) { user = true; } // Otherwise, find user object
            return true;
        }
        bool Uint(unsigned i) {     // id values are treated as Uint (not Uint64) by the reader
            if (user_id) {  // Getting id if previous key was "id" for a user
                result.emplace_back(i);
                user_id = false;
                user = false;
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
        bool String(const char* str, SizeType length, bool copy) { return true; }
        bool StartObject() { return true; }
        bool EndObject(SizeType memberCount) { return true; }
        bool StartArray() { return true; }
        bool EndArray(SizeType elementCount) { return true; }
    }; // handler

    bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
        Reader reader;
        Handler handler(result);
        InsituStringStream ss(json.data());
        reader.Parse<kParseInsituFlag | kParseValidateEncodingFlag | kParseFullPrecisionFlag>(ss,handler);
        return true;
    }

}; // rapid_jason_sax
BENCHMARK_TEMPLATE(distinct_user_id, rapidjson_sax)->UseManualTime();
} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_RAPIDJSON