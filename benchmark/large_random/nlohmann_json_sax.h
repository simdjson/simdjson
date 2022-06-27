#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "large_random.h"

namespace large_random {

using json = nlohmann::json;

struct nlohmann_json_sax {
    static constexpr diff_flags DiffFlags = diff_flags::NONE;

    struct Handler : json::json_sax_t
    {
        size_t k{0};
        double buffer[3];
        std::vector<point>& result;

        Handler(std::vector<point> &r) : result(r) {  }

        bool key(string_t& val) override {
            switch(val[0]) {
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
        bool number_unsigned(number_unsigned_t val) override {
            buffer[k] = double(val);
            if (k == 2) {
                result.emplace_back(json_benchmark::point{buffer[0],buffer[1],buffer[2]});
                k = 0;
            }
            return true;
        }
        bool number_float(number_float_t val, const string_t& s) override {
            buffer[k] = val;
            if (k == 2) {
                result.emplace_back(json_benchmark::point{buffer[0],buffer[1],buffer[2]});
                k = 0;
            }
            return true;
        }
        // Irrelevant events
        bool null() override { return true; }
        bool boolean(bool val) override { return true; }
        bool number_integer(number_integer_t val) override { return true; }
        bool string(string_t& val) override { return true; }
        bool start_object(std::size_t elements) override { return true; }
        bool end_object() override { return true; }
        bool start_array(std::size_t elements) override { return true; }
        bool end_array() override { return true; }
        bool binary(json::binary_t& val) override { return true; }
        bool parse_error(std::size_t position, const std::string& last_token, const json::exception& ex) override { return false; }
    }; // Handler

    bool run(simdjson::padded_string &json, std::vector<point> &result) {
        Handler handler(result);
        json::sax_parse(json.data(), &handler);
        return true;
    }
}; // nlohmann_json_sax
BENCHMARK_TEMPLATE(large_random, nlohmann_json_sax)->UseManualTime();
} // namespace large_random
#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON