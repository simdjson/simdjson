#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
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

  ondemand::parser *parser;
  const uint8_t *buf;
  size_t len;
  size_t batch_size;
  /** The error (or lack thereof) from the current document. */
  error_code error;
  size_t batch_start{0};
  size_t doc_index{};

  friend class ondemand::parser;
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