#ifndef SIMDJSON_SRC_GENERIC_STAGE2_TAPE_WRITER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE2_TAPE_WRITER_H
#include <generic/stage2/base.h>
#include <simdjson/internal/tape_type.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;

  /** Write a signed 64-bit value to tape. */
  simdjson_inline void append_s64(int64_t value) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_inline void append_u64(uint64_t value) noexcept;

  /** Write a double value to tape. */
  simdjson_inline void append_double(double value) noexcept;

  /** Write a big integer (as string) to tape. src points to first digit, len is byte count. */
  simdjson_inline void append_bigint(const uint8_t *src, size_t len, uint8_t *&string_buf) noexcept;

  /**
   * Append a tape entry (an 8-bit type,and 56 bits worth of value).
   */
  simdjson_inline void append(uint64_t val, internal::tape_type t) noexcept;

  /**
   * Skip the current tape entry without writing.
   *
   * Used to skip the start of the container, since we'll come back later to fill it in when the
   * container ends.
   */
  simdjson_inline void skip() noexcept;

  /**
   * Skip the number of tape entries necessary to write a large u64 or i64.
   */
  simdjson_inline void skip_large_integer() noexcept;

  /**
   * Skip the number of tape entries necessary to write a double.
   */
  simdjson_inline void skip_double() noexcept;

  /**
   * Write a value to a known location on tape.
   *
   * Used to go back and write out the start of a container after the container ends.
   */
  simdjson_inline static void write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept;

private:
  /**
   * Append both the tape entry, and a supplementary value following it. Used for types that need
   * all 64 bits, such as double and uint64_t.
   */
  template<typename T>
  simdjson_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct tape_writer

simdjson_inline void tape_writer::append_s64(int64_t value) noexcept {
  append2(0, value, internal::tape_type::INT64);
}

simdjson_inline void tape_writer::append_u64(uint64_t value) noexcept {
  append(0, internal::tape_type::UINT64);
  *next_tape_loc = value;
  next_tape_loc++;
}

/** Write a double value to tape. */
simdjson_inline void tape_writer::append_double(double value) noexcept {
  append2(0, value, internal::tape_type::DOUBLE);
}

simdjson_inline void tape_writer::skip() noexcept {
  next_tape_loc++;
}

simdjson_inline void tape_writer::skip_large_integer() noexcept {
  next_tape_loc += 2;
}

simdjson_inline void tape_writer::skip_double() noexcept {
  next_tape_loc += 2;
}

simdjson_inline void tape_writer::append(uint64_t val, internal::tape_type t) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56);
  next_tape_loc++;
}

template<typename T>
simdjson_inline void tape_writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
  append(val, t);
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

simdjson_inline void tape_writer::append_bigint(const uint8_t *src, size_t len, uint8_t *&string_buf) noexcept {
  // Write to string buffer: [4-byte LE length][digits][null]
  uint32_t str_len = uint32_t(len);
  memcpy(string_buf, &str_len, sizeof(uint32_t));
  memcpy(string_buf + sizeof(uint32_t), src, len);
  string_buf[sizeof(uint32_t) + len] = 0;
  // Tape entry: offset into string buffer
  // The caller must set the offset relative to doc.string_buf base
  append(0, internal::tape_type::BIGINT); // placeholder offset, caller patches
  string_buf += sizeof(uint32_t) + len + 1;
}

} // namespace stage2
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_TAPE_WRITER_H