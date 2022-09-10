namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;

  /** Write a signed 64-bit value to tape. */
  simdjson_inline void append_s64(int64_t value, uint32_t index) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_inline void append_u64(uint64_t value, uint32_t index) noexcept;

  /** Write a double value to tape. */
  simdjson_inline void append_double(double value, uint32_t index) noexcept;

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
  simdjson_inline void append_number(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct number_writer

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
simdjson_inline void tape_writer::append_s64(int64_t value, uint32_t index) noexcept {
#pragma clang diagnostic pop
  append_number(0, value, internal::tape_type::INT64);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
simdjson_inline void tape_writer::append_u64(uint64_t value, uint32_t index) noexcept {
#pragma clang diagnostic pop
  append_number(0, value, internal::tape_type::UINT64);
}

/** Write a double value to tape. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
simdjson_inline void tape_writer::append_double(double value, uint32_t index) noexcept {
#pragma clang diagnostic pop
  append_number(0, value, internal::tape_type::DOUBLE);
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
simdjson_inline void tape_writer::append_number(uint64_t val, T val2, internal::tape_type t, uint32_t index) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56) | index;
  next_tape_loc++;
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

} // namespace stage2
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
