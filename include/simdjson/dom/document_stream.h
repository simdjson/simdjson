#ifndef SIMDJSON_DOCUMENT_STREAM_H
#define SIMDJSON_DOCUMENT_STREAM_H

#include "simdjson/common_defs.h"
#include "simdjson/dom/parser.h"
#include "simdjson/error.h"
#ifdef SIMDJSON_THREADS_ENABLED
#include <thread>
#endif

namespace simdjson {
namespace dom {

/**
 * A forward-only stream of documents.
 *
 * Produced by parser::parse_many.
 *
 */
class document_stream {
public:
  /** Move one document_stream to another. */
  really_inline document_stream(document_stream && other) noexcept = default;
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
    really_inline iterator(document_stream &s, bool finished) noexcept;
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

  /**
   * Construct a document_stream. Does not allocate or parse anything until the iterator is
   * used.
   */
  really_inline document_stream(
    dom::parser &parser,
    const uint8_t *buf,
    size_t len,
    size_t batch_size,
    error_code error = SUCCESS
  ) noexcept;

  /**
   * Parse the first document in the buffer. Used by begin(), to handle allocation and
   * initialization.
   */
  inline void start() noexcept;

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
   * The function returns simdjson::EMPTY if there is no more data to be parsed.
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
   * and should be reused for the other documents in the buffer.
   */
  inline void next() noexcept;

  /**
   * Pass the next batch through stage 1 and return when finished.
   * When threads are enabled, this may wait for the stage 1 thread to finish.
   */
  inline void load_batch() noexcept;

  /** Get the next document index. */
  inline size_t next_batch_start() const noexcept;

  /** Pass the next batch through stage 1 with the given parser. */
  inline error_code run_stage1(dom::parser &p, size_t batch_start) noexcept;

  dom::parser &parser;
  const uint8_t *buf;
  const size_t len;
  const size_t batch_size;
  size_t batch_start{0};
  /** The error (or lack thereof) from the current document. */
  error_code error;

#ifdef SIMDJSON_THREADS_ENABLED
  inline void load_from_stage1_thread() noexcept;

  /** Start a thread to run stage 1 on the next batch. */
  inline void start_stage1_thread() noexcept;

  /** Wait for the stage 1 thread to finish and capture the results. */
  inline void finish_stage1_thread() noexcept;

  /** The error returned from the stage 1 thread. */
  error_code stage1_thread_error{UNINITIALIZED};
  /** The thread used to run stage 1 against the next batch in the background. */
  std::thread stage1_thread{};

  /**
   * The parser used to run stage 1 in the background. Will be swapped
   * with the regular parser when finished.
   */
  dom::parser stage1_thread_parser{};
#endif // SIMDJSON_THREADS_ENABLED

  friend class dom::parser;
}; // class document_stream

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_STREAM_H
