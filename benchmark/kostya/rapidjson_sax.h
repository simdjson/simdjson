#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "kostya.h"

namespace kostya {

using namespace rapidjson;

struct rapidjson_sax {
    static constexpr diff_flags DiffFlags = diff_flags::NONE;

    struct Handler {
        size_t k{0};
        double buffer[3];
        std::vector<point>& result;

        Handler(std::vector<point> &r) : result(r) { }

        bool Key(const char* key, SizeType length, bool copy) {
            switch(key[0]) {
            case 'x':
                k = 0;
                break;
            case 'y':
                k = 1;
                break;
            case 'z':
                k = 2;
                break;
            }
            return true;
        }
        bool Double(double d) {
            buffer[k] = d;
            if (k == 2) {
                result.emplace_back(json_benchmark::point{buffer[0],buffer[1],buffer[2]});
                k = 0;
            }
            return true;
        }
        bool Uint(unsigned i) {  return Double(i); }   // Need this event because coordinate value can be equal to 1
        // Irrelevant events
        bool Null() { return true; }
        bool Bool(bool b) { return true; }
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

    bool run(simdjson::padded_string &json, std::vector<point> &result) {
        Reader reader;
        Handler handler(result);
        InsituStringStream ss(json.data());
        reader.Parse<kParseInsituFlag | kParseValidateEncodingFlag | kParseFullPrecisionFlag>(ss,handler);
        return true;
    }

}; // rapid_jason_sax
BENCHMARK_TEMPLATE(kostya, rapidjson_sax)->UseManualTime();
} // namespace kostya

#endif // SIMDJSON_COMPETITION_RAPIDJSON