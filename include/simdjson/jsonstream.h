#ifndef SIMDJSON_JSONSTREAM_H
#define SIMDJSON_JSONSTREAM_H


#include <algorithm>
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include "simdjson/simdjson.h"

namespace simdjson {

    class JsonStream {
    public:
        JsonStream(const char *buf, size_t len, size_t batch_size = 1000000);

        JsonStream(const std::string &s, size_t batch_size = 1000000) : JsonStream(s.data(), s.size(), batch_size) {};

        int json_parse(ParsedJson &pj);

        void set_new_buffer(const char *buf, size_t len);

        void set_new_buffer(const std::string &s) { set_new_buffer(s.data(), s.size()); }

        size_t get_current_buffer_loc() const;

        size_t get_n_parsed_docs() const;

        size_t get_n_bytes_parsed() const;

    private:
        const char *_buf;
        size_t _len;
        size_t _batch_size;
        size_t next_json{0};
        bool error_on_last_attempt{false};
        bool load_next_batch{true};
        size_t current_buffer_loc{0};
        size_t n_parsed_docs{0};
        size_t n_bytes_parsed{0};
    };

}
#endif //SIMDJSON_JSONSTREAM_H
