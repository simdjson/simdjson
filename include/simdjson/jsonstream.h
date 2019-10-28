//
// Created by jpiotte on 10/23/19.
//

#ifndef SIMDJSON_JSONSTREAM_H
#define SIMDJSON_JSONSTREAM_H


#include <string>
#include <vector>
#include "simdjson/parsedjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"

namespace simdjson {

    class JsonStream {
    public:
        JsonStream(const char *buf, size_t len, size_t batch_size = 4096);

        JsonStream(const std::string &s, size_t len, size_t batch_size = 4096) : JsonStream(s.data(), len, batch_size) {};

        virtual ~JsonStream();

        int json_parse(ParsedJson &pj,bool realloc_if_needed = true);

    private:


        Architecture best_implementation;
        const char *buf;
        uint32_t next_json{0};
        size_t batch_size;
        size_t len{0};
        size_t error_count{0};
        bool load_next_batch{true};
    };

}
#endif //SIMDJSON_JSONSTREAM_H
