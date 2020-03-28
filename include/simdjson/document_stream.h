#ifndef SIMDJSON_DOCUMENT_STREAM_H
#define SIMDJSON_DOCUMENT_STREAM_H

#include <thread>
#include "simdjson/document.h"

namespace simdjson::dom {

/**
 * A forward-only stream of documents.
 *
 * Produced by parser::parse_many.
 *
 */
class document_stream {
public:
  really_inline ~document_stream() noexcept;

  /**
   * An iterator through a forward-only stream of documents.
   */
  class iterator {
  public:
    /**
     * Get the current document (or error).
     */
    really_inline simdjson_result<element> operator*() noexcept;
    /**
     * Advance to the next document.
     */
    inline iterator& operator++() noexcept;
    /**
     * Check if we're at the end yet.
     * @param other the end iterator to compare to.
     */
    really_inline bool operator!=(const iterator &other) const noexcept;

  private:
    iterator(document_stream& stream, bool finished) noexcept;
    /** The document_stream we're iterating through. */
    document_stream& stream;
    /** Whether we're finished or not. */
    bool finished;
    friend class document_stream;
  };

  /**
   * Start iterating the documents in the stream.
   */
  really_inline iterator begin() noexcept;
  /**
   * The end of the stream, for iterator comparison purposes.
   */
  really_inline iterator end() noexcept;

private:

  document_stream &operator=(const document_stream &) = delete; // Disallow copying

  document_stream(document_stream &other) = delete;    // Disallow copying

  really_inline document_stream(dom::parser &parser, const uint8_t *buf, size_t len, size_t batch_size, error_code error = SUCCESS) noexcept;

  /**
   * Parse the next document found in the buffer previously given to document_stream.
   *
   * The content should be a valid JSON document encoded as UTF-8. If there is a
   * UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
   * discouraged.
   *
   * You do NOT need to pre-allocate a parser.  This function takes care of
   * pre-allocating a capacity defined by the batch_size defined when creating the
   * document_stream object.
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
   * the simdjson::error_message function converts these error codes into a string).
   *
   * You can also check validity by calling parser.is_valid(). The same parser can
   * and should be reused for the other documents in the buffer. */
  inline error_code json_parse() noexcept;

  /**
   * Returns the location (index) of where the next document should be in the
   * buffer.
   * Can be used for debugging, it tells the user the position of the end of the
   * last
   * valid JSON document parsed
   */
  inline size_t get_current_buffer_loc() const { return current_buffer_loc; }

  /**
   * Returns the total amount of complete documents parsed by the document_stream,
   * in the current buffer, at the given time.
   */
  inline size_t get_n_parsed_docs() const { return n_parsed_docs; }

  /**
   * Returns the total amount of data (in bytes) parsed by the document_stream,
   * in the current buffer, at the given time.
   */
  inline size_t get_n_bytes_parsed() const { return n_bytes_parsed; }

  inline const uint8_t *buf() const { return _buf + buf_start; }

  inline void advance(size_t offset) { buf_start += offset; }

  inline size_t remaining() const { return _len - buf_start; }

  dom::parser &parser;
  const uint8_t *_buf;
  const size_t _len;
  size_t _batch_size; // this is actually variable!
  size_t buf_start{0};
  size_t next_json{0};
  bool load_next_batch{true};
  size_t current_buffer_loc{0};
#ifdef SIMDJSON_THREADS_ENABLED
  size_t last_json_buffer_loc{0};
#endif
  size_t n_parsed_docs{0};
  size_t n_bytes_parsed{0};
  error_code error{SUCCESS_AND_HAS_MORE};
#ifdef SIMDJSON_THREADS_ENABLED
  error_code stage1_is_ok_thread{SUCCESS};
  std::thread stage_1_thread;
  dom::parser parser_thread;
#endif
  friend class dom::parser;
}; // class document_stream

} // end of namespace simdjson::dom

#endif // SIMDJSON_DOCUMENT_STREAM_H
