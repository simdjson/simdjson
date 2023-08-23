#ifndef SIMDJSON_GENERIC_SIMD_LOOKUP_TABLE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_SIMD_LOOKUP_TABLE_H
#include "simdjson/generic/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {

template <typename T = uint8_t>
struct simd8_buffer {
  T buf[simd8<T>::LANES] = {0};
  simdjson_consteval T& operator[](size_t i) noexcept { return buf[i]; }
  simdjson_consteval T operator[](size_t i) const noexcept { return buf[i]; }
  simdjson_inline operator simd8<T>() const noexcept { return buf; }
};


struct nibble_range;

struct byte_range {
  struct _exclusive{};
  static simdjson_constinit _exclusive exclusive{};

  simdjson_consteval byte_range(uint8_t start, uint16_t end, const _exclusive&) noexcept : _start{start}, _end{end} {
    SIMDJSON_ASSUME(start < end && end <= 256);
  }
  simdjson_consteval byte_range(uint8_t first, uint8_t last) noexcept : byte_range(first, static_cast<uint16_t>(last+1), exclusive) {}
  simdjson_consteval byte_range(uint8_t first) noexcept : byte_range(first, first) {}

  simdjson_consteval size_t size() const noexcept { return static_cast<size_t>(_end - _start); }
  simdjson_consteval byte_range operator|(const byte_range& other) const noexcept {
    SIMDJSON_ASSUME((_end + 1 >= other._start) || (other._end + 1 >= _start));
    return {std::min(_start, other._start), std::max(_end, other._end), exclusive};
  }

  simdjson_consteval bool includes(uint8_t byte) const noexcept { return _start <= byte && byte < _end; }

  struct nibble_iter {
    int nibble;
    const int last_nibble;
    simdjson_consteval nibble_iter& operator++() noexcept {
      if (nibble == last_nibble) {
        nibble = -1;
      } else {
        ++nibble;
        nibble %= 16;
      }
      return *this;
    }
    simdjson_consteval nibble_iter operator++(int) noexcept { auto copy = *this; ++*this; return copy; }
    simdjson_consteval bool operator==(const nibble_iter& other) const noexcept { return nibble == other.nibble; }
    simdjson_consteval bool operator!=(const nibble_iter& other) const noexcept { return nibble != other.nibble; }
    simdjson_consteval uint8_t operator*() const noexcept { return static_cast<uint8_t>(nibble); }
    simdjson_consteval nibble_iter begin() const noexcept { return *this; }
    simdjson_consteval nibble_iter end() const noexcept { return {-1, last_nibble}; }
  };

  simdjson_consteval nibble_iter nibble(int shift) const noexcept {
    SIMDJSON_ASSUME(_start < _end);
    auto first_nibble = _start >> shift;
    auto last_nibble = (_end-1) >> shift;
    if ((last_nibble - first_nibble) >= 16) { return {0x00, 0x0F}; }
    return {static_cast<uint8_t>(first_nibble & 0x0F), static_cast<uint8_t>(last_nibble & 0x0F)};
  }

  struct _iter {
    uint16_t value;
    simdjson_consteval _iter& operator++() noexcept { ++value; return *this; }
    simdjson_consteval _iter operator++(int) noexcept { auto copy = *this; ++*this; return copy; }
    simdjson_consteval bool operator==(const _iter& other) const noexcept { return value == other.value; }
    simdjson_consteval bool operator!=(const _iter& other) const noexcept { return value != other.value; }
    simdjson_consteval uint8_t operator*() const noexcept { return static_cast<uint8_t>(value); }
  };

  simdjson_consteval _iter begin() const noexcept { return _iter{_start}; }
  simdjson_consteval _iter end() const noexcept { return _iter{_end}; }

  uint8_t _start;
  const uint16_t _end;
};

namespace {

struct _lookup_entry_range;

struct _lookup_entry : byte_range {
  const uint8_t value;

  simdjson_consteval _lookup_entry(const byte_range& bytes, uint8_t value) noexcept
          : byte_range{bytes}, value{value} {}
};

simdjson_consteval simd8_buffer<uint8_t> _make_nibble_lookup_table(
      std::initializer_list<_lookup_entry> entries, int shift) noexcept {
  // Make the buffer
  simd8_buffer<uint8_t> buf;
  for (auto entry : entries) {
    for (auto key : entry.nibble(shift)) {
      // Repeat the value over and over for longer simd types.
      for (uint8_t k = key; k < sizeof(buf); k += 16) { buf[k] |= entry.value; }
    }
  }
  return buf;
}

} // unnamed namespace

/**
 * Byte lookup table where the key is the high 4 bits of the input, and the value is an
 * arbitrary byte.
 *
 * - Unmatched values yield 0.
 * - Multiple keys may yield the same value.
 * - Multiple bytes with the same high 4 bits may NOT yield different values.
 *
 *     ```
 *     enum ops_t : uint8_t {
 *       COMMA   = 1,
 *       COLON   = 2,
 *       BRACKET = 3,
 *       CURLY   = 4
 *     };
 *     static constinit const high_nibble_lookup OPS(
 *       {',', COMMA},
 *       {':', COLON},
 *       {'[', BRACKET},
 *       {']', BRACKET},
 *       {'{', CURLY},
 *       {'}', CURLY}
 *     );
 *     simd8<uint8_t> lookup_ops(simd8<uint8_t>& operators) { return OPS[operators]; }
 *     ```
 */
struct high_nibble_lookup {
  const simd8_buffer<uint8_t> table;

  /**
   * Construct a nibble lookup table from the high bits of the input to the output.
   *
   * @param entries A list of {key, value} pairs (e.g. {'a', 10}).
   * @error asserts if multiple keys have the same high 4 bits but different values.
   */
  simdjson_consteval high_nibble_lookup(std::initializer_list<_lookup_entry> entries) noexcept
                      : table{_make_nibble_lookup_table(entries, 4)} {}
  simdjson_consteval high_nibble_lookup(const simd8_buffer<uint8_t>& table) noexcept : table(table) {}

  /** Look up the value corresponding the higher 4 bits of each input byte, and return it. */
  simdjson_inline simd8<uint8_t> operator[](const simd8<uint8_t>& keys) const noexcept { return lookup(keys); }
  /** Look up the value corresponding the higher 4 bits of each input byte, and return it. */
  simdjson_inline simd8<uint8_t> lookup(const simd8<uint8_t>& keys) const noexcept { return lookup_low(keys.shr<4>()); }
  /**
   * Look up the value in the table assuming the high 4 key bits are stored in the lower 4 bits.
   * @pre all indexes be less than 16.
   */
  simdjson_inline simd8<uint8_t> lookup_low(const simd8<uint8_t>& shifted_keys) const noexcept {
    return shifted_keys.lookup_16(table);
  }

  /**
   * Look up the value in the table assuming the high 4 key bits are stored in the lower 4 bits.
   * @pre all indexes be less than 16.
   */
  simdjson_inline simd8x64<uint8_t> lookup_low(const simd8x64<uint8_t>& shifted_keys) const noexcept {
    return shifted_keys.lookup_16(table);
  }
  /** Look up the value corresponding the higher 4 bits of each input byte, and return it. */
  simdjson_inline simd8x64<uint8_t> lookup(const simd8x64<uint8_t>& keys) const noexcept { return lookup_low(keys.shr<4>()); }
  /** Look up the value corresponding the higher 4 bits of each input byte, and return it. */
  simdjson_inline simd8x64<uint8_t> operator[](const simd8x64<uint8_t>& keys) const noexcept { return lookup(keys); }

  simdjson_consteval uint8_t operator[](uint8_t key) const noexcept { return lookup(key); }
  simdjson_consteval uint8_t lookup(uint8_t key) const noexcept { return table[key >> 4]; }
};

/**
 * Byte lookup table where the key is the low 4 bits of the input, and the value is an
 * arbitrary byte.
 *
 * - Unmatched values yield 0.
 * - Multiple keys may yield the same value.
 * - Multiple bytes with the same low 4 bits may NOT yield different values.
 *
 *     ```
 *     enum ops_t : uint8_t {
 *       COMMA   = 1,
 *       COLON   = 2,
 *       BRACKET = 3,
 *       CURLY   = 4
 *     };
 *     static constinit const high_nibble_lookup OPS(
 *       {',', COMMA},
 *       {':', COLON},
 *       {'[', BRACKET},
 *       {']', BRACKET},
 *       {'{', CURLY},
 *       {'}', CURLY}
 *     );
 *     simd8<uint8_t> lookup_ops(simd8<uint8_t>& operators) { return OPS[operators]; }
 *     ```
 */
struct low_nibble_lookup {
  const simd8_buffer<uint8_t> table;

  /**
   * Construct a nibble lookup table from the low bits of the input to the output.
   *
   * @param entries A list of {key, value} pairs (e.g. {'a', 0}).
   * @error asserts if multiple keys have the same low 4 bits but different values.
   */
  simdjson_consteval low_nibble_lookup(std::initializer_list<_lookup_entry> entries) noexcept
                      : table{_make_nibble_lookup_table(entries, 0)} {}
  simdjson_consteval low_nibble_lookup(const simd8_buffer<uint8_t>& table) noexcept : table(table) {}

  /** Look up the value corresponding the lower 4 bits of each input byte, and return it. */
  simdjson_inline simd8<uint8_t> operator[](const simd8<uint8_t>& keys) const noexcept { return lookup(keys); }
  /** Look up the value corresponding the lower 4 bits of each input byte, and return it. */
  simdjson_inline simd8<uint8_t> lookup(const simd8<uint8_t>& keys) const noexcept {
    return keys.lookup_low_nibble_ascii(table);
  }
  /**
   * Look up the value in the table. Behavior is system-dependent for indexes greater than 16.
   *
   * - On some platforms like arm64, indexes greater than 16 will not match anything in the table.
   * - On platforms like Intel, index bits 4-6 will be ignored, but if the high bit is set, it
   *   will not match anything in the table. greater than 16 will be ignored, *except* if the high bit is 1,
   */
  simdjson_inline simd8<uint8_t> lookup_unsafe(const simd8<uint8_t>& keys) const noexcept {
    return keys.lookup_16(table);
  }

  /** Look up the value corresponding the lower 4 bits of each input byte, and return it. */
  simdjson_inline simd8x64<uint8_t> operator[](const simd8x64<uint8_t>& keys) const noexcept { return lookup(keys); }
  /** Look up the value corresponding the lower 4 bits of each input byte, and return it. */
  simdjson_inline simd8x64<uint8_t> lookup(const simd8x64<uint8_t>& keys) const noexcept {
    return keys.lookup_low_nibble_ascii(table);
  }
  /**
   * Look up the value in the table. Behavior is system-dependent for indexes greater than 16.
   *
   * - On some platforms like arm64, indexes greater than 16 will not match anything in the table.
   * - On platforms like Intel, index bits 4-6 will be ignored, but if the high bit is set, it
   *   will not match anything in the table. greater than 16 will be ignored, *except* if the high bit is 1,
   */
  simdjson_inline simd8x64<uint8_t> lookup_unsafe(const simd8x64<uint8_t>& low_keys) const noexcept {
    return low_keys.lookup_16(table);
  }

  simdjson_consteval uint8_t operator[](uint8_t key) const noexcept { return lookup(key); }
  simdjson_consteval uint8_t lookup(uint8_t key) const noexcept { return table[key & 0x0F]; }
};

/**
 * Classifies bytes by looking up their lower 4 bits, then their high 4 bits, and &'ing the
 * results together.
 *
 * Pass the bytes you want to match, and the classifications you want for them.
 */
struct byte_classifier {
  const low_nibble_lookup low;
  const high_nibble_lookup high;
  simdjson_consteval byte_classifier(std::initializer_list<_lookup_entry> entries)
        : low{entries}, high{entries} {}

  simdjson_inline simd8<uint8_t> classify(const simd8<uint8_t>& bytes) const noexcept {
    return low.lookup(bytes) & high.lookup(bytes);
  }
  simdjson_inline simd8x64<uint8_t> classify(const simd8x64<uint8_t>& bytes) const noexcept {
    auto low_lookup  = low.lookup(bytes);  // 3 (+simd:N)
    auto high_lookup = high.lookup(bytes); // 6 (+simd:2N)
    return low_lookup & high_lookup;                               //   3 (+simd:N)
    // critical path: 9 (+simd:4N)
  }
  simdjson_consteval uint8_t classify(uint8_t byte) const noexcept {
    return low.lookup(byte) & high.lookup(byte);
  }

  simdjson_inline simd8<uint8_t> operator[](const simd8<uint8_t>& bytes) const noexcept { return classify(bytes); }
  simdjson_inline simd8x64<uint8_t> operator[](const simd8x64<uint8_t>& bytes) const noexcept { return classify(bytes); }
  simdjson_consteval uint8_t operator[](uint8_t byte) const noexcept { return classify(byte); }

  simdjson_inline bool matches_correctly(std::initializer_list<_lookup_entry> entries) const noexcept {
    uint8_t expected_output[256] = {};
    for (auto entry : entries) {
      for (uint8_t byte : entry) {
        expected_output[byte] |= entry.value;
      }
    }
    for (uint8_t byte = 0; byte <= 0xFF; byte++) {
      if (expected_output[byte] != classify(byte)) { return false; }
    }
    return true;
  }
};


} // namespace simd
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_SIMD_LOOKUP_TABLE_H