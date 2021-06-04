#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "find_tweet.h"

namespace find_tweet {

using json = nlohmann::json;

struct nlohmann_json_sax {
    using StringType=std::string;

    struct Handler : json::json_sax_t
    {
        bool text_key = false;
        bool id_key = false;
        bool found_id = false;
        uint64_t find_id;
        std::string &result;

        Handler(std::string &r,uint64_t id): result(r), find_id(id) { }

        // We assume id is found before text
        bool key(string_t& val) override {
            if (found_id) { // If have found id, find text key
                if (val.compare("text") == 0) { text_key = true; }
            }
            else if (val.compare("id") == 0) { id_key = true; } // Otherwise, find id key
            return true;
        }
        bool number_unsigned(number_unsigned_t val) override {
            if (id_key && (val == find_id)) {  // If id key, check if id value matches find_id
                found_id = true;
            }
            return true;
        }
        bool string(string_t& val) override {
            if (text_key) {
                result = val;
                return false;   // End parsing
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

    bool run(simdjson::padded_string &json, uint64_t find_id, std::string &result) {
        Handler handler(result,find_id);
        json::sax_parse(json.data(), &handler);

        return true;
    }
}; // nlohmann_json_sax
BENCHMARK_TEMPLATE(find_tweet, nlohmann_json_sax)->UseManualTime();
} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON