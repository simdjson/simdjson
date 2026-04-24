#ifndef SIMDJSON_DOM_DOCUMENT_H
#define SIMDJSON_DOM_DOCUMENT_H

#include "simdjson/dom/base.h"
#include "simdjson/internal/allocated_buffer.h"

#include <memory>

namespace simdjson {
namespace dom {

/**
 * A parsed JSON document.
 *
 * This class cannot be copied, only moved, to avoid unintended allocations.
 */
class document {
public:
  /**
   * Create a document container with zero capacity.
   *
   * The parser will allocate capacity as needed.
   *
   * @param alloc The allocator used for the tape and string buffer. The allocator must
   *    outlive this document.
   */
  document(simdjson::allocator& alloc = get_default_allocator()) noexcept
      : _allocator{&alloc}, allocated_capacity{0} {}
  ~document() noexcept = default;

  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed and it is invalidated.
   */
  document(document &&other) noexcept = default;
  /** @private */
  document(const document &) = delete; // Disallow copying
  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed.
   */
  document &operator=(document &&other) noexcept = default;
  /** @private */
  document &operator=(const document &) = delete; // Disallow copying

  /**
   * Get the root element of this document as a JSON array.
   */
  element root() const noexcept;

  /**
   * @private Dump the raw tape for debugging.
   *
   * @param os the stream to output to.
   * @return false if the tape is likely wrong (e.g., you did not parse a valid JSON).
   */
  bool dump_raw_tape(std::ostream &os) const noexcept;

  /** @private Structural values. */
  internal::allocated_buffer<uint64_t> tape{};

  /** @private String values.
   *
   * Should be at least byte_capacity.
   */
  internal::allocated_buffer<uint8_t> string_buf{};
  /** @private Allocate memory to support
   * input JSON documents of up to len bytes.
   *
   * When calling this function, you lose
   * all the data.
   *
   * The memory allocation is strict: you
   * can you use this function to increase
   * or lower the amount of allocated memory.
   * Passing zero clears the memory.
   *
   * May propagate exceptions from the allocator (e.g. std::bad_alloc).
   */
  error_code allocate(size_t len);
  /** @private Capacity in bytes, in terms
   * of how many bytes of input JSON we can
   * support.
   */
  size_t capacity() const noexcept;


private:
  simdjson::allocator* _allocator;
  size_t allocated_capacity;
  friend class parser;
}; // class document

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
