#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * The default batch size for document_stream instances for this On Demand kernel.
 * Note that different On Demand kernel may use a different DEFAULT_BATCH_SIZE value
 * in the future.
 */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;
/**
 * Some adversary might try to set the batch size to 0 or 1, which might cause problems.
 * We set a minimum of 32B since anything else is highly likely to be an error. In practice,
 * most users will want a much larger batch size.
 *
 * All non-negative MINIMAL_BATCH_SIZE values should be 'safe' except that, obviously, no JSON
 * document can ever span 0 or 1 byte and that very large values would create memory allocation issues.
 */
static constexpr size_t MINIMAL_BATCH_SIZE = 32;

class parser;
class json_iterator;
class document;

/**
 * A forward-only stream of documents.
 *
 * Produced by parser::iterate_many.
 *
 */
class document_stream {
public:
  /**
   * Construct an uninitialized document_stream.
   *
   *  ```c++
   *  document_stream docs;
   *  auto error = parser.iterate_many(json).get(docs);
   *  ```
   */
  simdjson_really_inline document_stream() noexcept;
  /** Move one document_stream to another. */
  simdjson_really_inline document_stream(document_stream &&other) noexcept = default;
  /** Move one document_stream to another. */
  simdjson_really_inline document_stream &operator=(document_stream &&other) noexcept = default;

  simdjson_really_inline ~document_stream() noexcept;

  /**
   * Returns the input size in bytes.
   */
  inline size_t size_in_bytes() const noexcept;

  /**
   * After iterating through the stream, this method
   * returns the number of bytes that were not parsed at the end
   * of the stream. If truncated_bytes() differs from zero,
   * then the input was truncated maybe because incomplete JSON
   * documents were found at the end of the stream. You
   * may need to process the bytes in the interval [size_in_bytes()-truncated_bytes(), size_in_bytes()).
   *
   * You should only call truncated_bytes() after streaming through all
   * documents, like so:
   *
   *   document_stream stream = parser.iterate_many(json,window);
   *   for(auto & doc : stream) {
   *      // do something with doc
   *   }
   *   size_t truncated = stream.truncated_bytes();
   *
   */
  inline size_t truncated_bytes() const noexcept;

  class iterator {
  public:
    using value_type = simdjson_result<document>;
    using reference  = value_type;

    using difference_type   = std::ptrdiff_t;

    using iterator_category = std::input_iterator_tag;

    /**
     * Default constructor.
     */
    simdjson_really_inline iterator() noexcept;
    /**
     * Get the current document (or error).
     */
    simdjson_really_inline ondemand::document& operator*() noexcept;
    /**
     * Advance to the next document (prefix).
     */
    inline iterator& operator++() noexcept;
    /**
     * Check if we're at the end yet.
     * @param other the end iterator to compare to.
     */
    simdjson_really_inline bool operator!=(const iterator &other) const noexcept;
    /**
     * @private
     *
     * Gives the current index in the input document in bytes.
     *
     *   document_stream stream = parser.parse_many(json,window);
     *   for(auto i = stream.begin(); i != stream.end(); ++i) {
     *      auto doc = *i;
     *      size_t index = i.current_index();
     *   }
     *
     * This function (current_index()) is experimental and the usage
     * may change in future versions of simdjson: we find the API somewhat
     * awkward and we would like to offer something friendlier.
     */
     simdjson_really_inline size_t current_index() const noexcept;

     /**
     * @private
     *
     * Gives a view of the current document at the current position.
     *
     *   document_stream stream = parser.iterate_many(json,window);
     *   for(auto i = stream.begin(); i != stream.end(); ++i) {
     *      std::string_view v = i.source();
     *   }
     *
     * The returned string_view instance is simply a map to the (unparsed)
     * source string: it may thus include white-space characters and all manner
     * of padding.
     *
     * This function (source()) is experimental and the usage
     * may change in future versions of simdjson: we find the API somewhat
     * awkward and we would like to offer something friendlier.
     *
     */
     simdjson_really_inline std::string_view source() const noexcept;

    /**
     * Returns error of the stream (if any).
     */
     inline error_code error() const noexcept;

  private:
    simdjson_really_inline iterator(document_stream *s, bool finished) noexcept;
    /** The document_stream we're iterating through. */
    document_stream* stream;
    /** Whether we're finished or not. */
    bool finished;

    friend class document;
    friend class document_stream;
    friend class json_iterator;
  };

  /**
   * Start iterating the documents in the stream.
   */
  simdjson_really_inline iterator begin() noexcept;
  /**
   * The end of the stream, for iterator comparison purposes.
   */
  simdjson_really_inline iterator end() noexcept;

private:

  document_stream &operator=(const document_stream &) = delete; // Disallow copying
  document_stream(const document_stream &other) = delete; // Disallow copying

  /**
   * Construct a document_stream. Does not allocate or parse anything until the iterator is
   * used.
   *
   * @param parser is a reference to the parser instance used to generate this document_stream
   * @param buf is the raw byte buffer we need to process
   * @param len is the length of the raw byte buffer in bytes
   * @param batch_size is the size of the windows (must be strictly greater or equal to the largest JSON document)
   */
  simdjson_really_inline document_stream(
    ondemand::parser &parser,
    const uint8_t *buf,
    size_t len,
    size_t batch_size
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

  /** Move the json_iterator of the document to the location of the next document in the stream. */
  inline void next_document() noexcept;

  /** Get the next document index. */
  inline size_t next_batch_start() const noexcept;

  /** Pass the next batch through stage 1 with the given parser. */
  inline error_code run_stage1(ondemand::parser &p, size_t batch_start) noexcept;

  // Fields
  ondemand::parser *parser;
  const uint8_t *buf;
  size_t len;
  size_t batch_size;
  /**
   * We are going to use just one document instance. The document owns
   * the json_iterator. It implies that we only ever pass a reference
   * to the document to the users.
   */
  document doc{};
  /** The error (or lack thereof) from the current document. */
  error_code error;
  size_t batch_start{0};
  size_t doc_index{};

  friend class parser;
  friend class document;
  friend class json_iterator;
  friend struct simdjson_result<ondemand::document_stream>;
  friend struct internal::simdjson_result_base<ondemand::document_stream>;
};  // document_stream

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {
template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_stream> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_stream> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document_stream &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
};

} // namespace simdjson