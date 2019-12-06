namespace simd {
  namespace stream {
    namespace prev_input {
      struct mid {
        const uint8_t* buf;
        really_inline operator simd8<uint8_t>() { return buf-sizeof(simd8<uint8_t>); }
      };
      struct mid_cached: mid {
        const simd8<uint8_t> bytes;
        really_inline operator simd8<uint8_t>() { return bytes; }
      };
      struct start {
        really_inline operator simd8<uint8_t>() { return uint8_t(' '); }
      };
      struct single: start {};
      struct end {
        const uint8_t* prev_buf_end;
        really_inline operator simd8<uint8_t>() { return prev_buf_end-sizeof(simd8<uint8_t>); }
      };
      struct start_or_end {
        const simd8<uint8_t> bytes;
        really_inline start_or_end(start other): bytes(other) {}
        really_inline start_or_end(single other): bytes(other) {}
        really_inline start_or_end(end other): bytes(other) {}
        really_inline operator simd8<uint8_t>() { return bytes; }
      };
    }
  }
}