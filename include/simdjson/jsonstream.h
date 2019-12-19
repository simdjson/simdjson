#ifndef SIMDJSON_JSONSTREAM_H
#define SIMDJSON_JSONSTREAM_H


#include <algorithm>
#include <thread>
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include "simdjson/simdjson.h"

namespace simdjson {
    /*************************************************************************************
     * The main motivation for this piece of software is to achieve maximum speed and offer
     * good quality of life while parsing files containing multiple JSON documents.
     *
     * Since we want to offer flexibility and not restrict ourselves to a specific file
     * format, we support any file that contains any valid JSON documents separated by one
     * or more character that is considered a whitespace by the JSON spec.
     * Namely: space, nothing, linefeed, carriage return, horizontal tab.
     * Anything that is not whitespace will be parsed as a JSON document and could lead
     * to failure.
     *
     * To offer maximum parsing speed, our implementation processes the data inside the
     * buffer by batches and their size is defined by the parameter "batch_size".
     * By loading data in batches, we can optimize the time spent allocating data in the
     * ParsedJson and can also open the possibility of multi-threading.
     * The batch_size must be at least as large as the biggest document in the file, but
     * not too large in order to submerge the chached memory.  We found that 1MB is
     * somewhat a sweet spot for now.  Eventually, this batch_size could be fully
     * automated and be optimal at all times.
     ************************************************************************************/
    class JsonStream {
    public:
        /* Create a JsonStream object that can be used to parse sequentially the valid
         * JSON documents found in the buffer "buf".
         *
         * The batch_size must be at least as large as the biggest document in the file, but
         * not too large to submerge the cached memory.  We found that 1MB is
         * somewhat a sweet spot for now.
         *
         * The user is expected to call the following json_parse method to parse the next
         * valid JSON document found in the buffer.  This method can and is expected to be
         * called in a loop.
         *
         * Various methods are offered to keep track of the status, like get_current_buffer_loc,
         * get_n_parsed_docs, get_n_bytes_parsed, etc.
         *
         * */
        JsonStream(const char *buf, size_t len, size_t batch_size = 1000000);

        /* Create a JsonStream object that can be used to parse sequentially the valid
         * JSON documents found in the buffer "buf".
         *
         * The batch_size must be at least as large as the biggest document in the file, but
         * not too large to submerge the cached memory.  We found that 1MB is
         * somewhat a sweet spot for now.
         *
         * The user is expected to call the following json_parse method to parse the next
         * valid JSON document found in the buffer.  This method can and is expected to be
         * called in a loop.
         *
         * Various methods are offered to keep track of the status, like get_current_buffer_loc,
         * get_n_parsed_docs, get_n_bytes_parsed, etc.
         *
         * */
        JsonStream(const std::string &s, size_t batch_size = 1000000) : JsonStream(s.data(), s.size(), batch_size) {};

        /* Parse the next document found in the buffer previously given to JsonStream.

         * The content should be a valid JSON document encoded as UTF-8. If there is a
         * UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
         * discouraged.
         *
         * You do NOT need to pre-allocate ParsedJson.  This function takes care of
         * pre-allocating a capacity defined by the batch_size defined when creating the
         * JsonStream object.
         *
         * The function returns simdjson::SUCCESS_AND_HAS_MORE (an integer = 1) in case
         * of success and indicates that the buffer still contains more data to be parsed,
         * meaning this function can be called again to return the next JSON document
         * after this one.
         *
         * The function returns simdjson::SUCCESS (as integer = 0) in case of success
         * and indicates that the buffer has successfully been parsed to the end.
         * Every document it contained has been parsed without error.
         *
         * The function returns an error code from simdjson/simdjson.h in case of failure
         * such as simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
         * the simdjson::error_message function converts these error codes into a
         * string).
         *
         * You can also check validity by calling pj.is_valid(). The same ParsedJson can
         * and should be reused for the other documents in the buffer. */
        int json_parse(ParsedJson &pj);

        /* Sets a new buffer for this JsonStream.  Will also reinitialize all the variables,
         * which acts as a reset.  A new JsonStream without initializing again.
         * */
        void set_new_buffer(const char *buf, size_t len);

        /* Sets a new buffer for this JsonStream.  Will also reinitialize all the variables,
         * which is basically a reset.  A new JsonStream without initializing again.
         * */
        void set_new_buffer(const std::string &s) { set_new_buffer(s.data(), s.size()); }

        /* Returns the location (index) of where the next document should be in the buffer.
         * Can be used for debugging, it tells the user the position of the end of the last
         * valid JSON document parsed*/
        size_t get_current_buffer_loc() const;

        /* Returns the total amount of complete documents parsed by the JsonStream,
         * in the current buffer, at the given time.*/
        size_t get_n_parsed_docs() const;

        /* Returns the total amount of data (in bytes) parsed by the JsonStream,
         * in the current buffer, at the given time.*/
        size_t get_n_bytes_parsed() const;

    private:
        const char *_buf;
        size_t _len;
        size_t _batch_size;
        size_t block{0};
        bool error_on_last_attempt{false};
        bool load_next_batch{true};
        size_t current_buffer_loc{0};
        size_t last_json_buffer_loc{0};
        size_t n_parsed_docs{0};
        size_t n_bytes_parsed{0};

        std::thread stage_1_thread;
        simdjson::ParsedJson pj_thread;

#ifdef SIMDJSON_THREADS_ENABLED
        /* This algorithm is used to quickly identify the buffer position of
         * the last JSON document inside the current batch.
         *
         * It does it's work by finding the last pair of structural characters
         * that represent the end followed by the start of a document.
         *
         * Simply put, we iterate over the structural characters, starting from
         * the end. We consider that we found the end of a JSON document when the
         * first element of the pair is NOT one of these characters: '{' '[' ';' ','
         * and when the second element is NOT one of these characters: '}' '}' ';' ','.
         *
         * This simple comparison works most of the time, but it does not cover cases
         * where the batch's structural indexes contain a perfect amount of documents.
         * In such a case, we do not have access to the structural index which follows
         * the last document, therefore, we do not have access to the second element in
         * the pair, and means that we cannot identify the last document. To fix this
         * issue, we keep a count of the open and closed curly/square braces we found
         * while searching for the pair. When we find a pair AND the count of open and
         * closed curly/square braces is the same, we know that we just passed a complete
         * document, therefore the last json buffer location is the end of the batch
         * */
        size_t find_last_json_buf_loc(const ParsedJson &pj);

#endif
    };

}
#endif //SIMDJSON_JSONSTREAM_H
