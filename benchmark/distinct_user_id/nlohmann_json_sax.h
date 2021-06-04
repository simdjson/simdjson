#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "distinct_user_id.h"

namespace distinct_user_id {

using json = nlohmann::json;

struct nlohmann_json_sax {
    struct Handler : json::json_sax_t
    {
        std::vector<uint64_t>& result;
        bool user = false;
        bool user_id = false;
        Handler(std::vector<uint64_t> &r) : result(r) { }

        bool key(string_t& val) override {
            // Assume that valid user/id pairs appear only once in main array of user objects
            if (user) { // If already found user object, find id key
                if (val.compare("id") == 0) { user_id = true; }
            }
            else if (val.compare("user") == 0) { user = true; } // Otherwise, find user object
            return true;
        }
        bool number_unsigned(number_unsigned_t val) override {
            if (user_id) {
                result.emplace_back(val);
                user = false;
                user_id = false;
            }
            return true;
        }
        // Irrelevant events
        bool null() override { return true; }
        bool boolean(bool val) override { return true; }
        bool number_float(number_float_t val, const string_t& s) override { return true; }
        bool number_integer(number_integer_t val) override { return true; }
        bool string(string_t& val) override { return true; }
        bool start_object(std::size_t elements) override { return true; }
        bool end_object() override { return true; }
        bool start_array(std::size_t elements) override { return true; }
        bool end_array() override { return true; }
        bool binary(json::binary_t& val) override { return true; }
        bool parse_error(std::size_t position, const std::string& last_token, const json::exception& ex) override { return false; }
    }; // Handler

    bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
        Handler handler(result);
        json::sax_parse(json.data(), &handler);

        return true;
    }
}; // nlohmann_json_sax
BENCHMARK_TEMPLATE(distinct_user_id, nlohmann_json_sax)->UseManualTime();
} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON