#ifndef SIMDJSON_DOM_DOCUMENT_H
#define SIMDJSON_DOM_DOCUMENT_H

#include "simdjson/common_defs.h"
#include <memory>
#include <ostream>

namespace simdjson {
namespace dom {

class element;

class deleter {
public:
  struct uint8_unique_ptr_deleter {
    std::allocator<uint8_t> uint8_allocator = decltype(uint8_allocator)();
    size_t allocated_count = 0;
    void operator()(uint8_t* p) {
      std::allocator_traits<decltype(uint8_allocator)>::destroy(uint8_allocator, p);
      std::allocator_traits<decltype(uint8_allocator)>::deallocate(uint8_allocator, p, allocated_count);
    }
  };

  struct uint64_unique_ptr_deleter {
    std::allocator<uint64_t> uint64_allocator = decltype(uint64_allocator)();
    size_t allocated_count = 0;
    void operator()(uint64_t* p) {
      std::allocator_traits<decltype(uint64_allocator)>::destroy(uint64_allocator, p);
      std::allocator_traits<decltype(uint64_allocator)>::deallocate(uint64_allocator, p, allocated_count);
    }
  };
};

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
   */
  document() noexcept = default;
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
  std::unique_ptr<uint64_t[], deleter::uint64_unique_ptr_deleter> tape{};

  /** @private String values.
   *
   * Should be at least byte_capacity.
   */
  std::unique_ptr<uint8_t[], deleter::uint8_unique_ptr_deleter> string_buf{};
  /** @private Allocate memory to support
   * input JSON documents of up to len bytes.
   *
   * When calling this function, you lose
   * all the data.
   *
   * The memory allocation is strict: you
   * can you use this function to increase
   * or lower the amount of allocated memory.
   * Passsing zero clears the memory.
   */
  error_code allocate(size_t len) noexcept;
  /** @private Capacity in bytes, in terms
   * of how many bytes of input JSON we can
   * support.
   */
  size_t capacity() const noexcept;


private:
  size_t allocated_capacity{0};
  friend class parser;
}; // class document

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
