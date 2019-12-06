namespace simd {
  namespace stream {
    namespace prev_input {
      struct mid_uncached {
        const uint8_t* buf;
        really_inline operator simd8<uint8_t>() { return buf-sizeof(simd8<uint8_t>); }
        template<int N=3>
        really_inline simd8<uint8_t> try_prev_mem(simd8<uint8_t> input) { return buf-N; }
      };
      struct mid: mid_uncached {
        const simd8<uint8_t> bytes;
        really_inline operator simd8<uint8_t>() { return bytes; }
      };
      struct start {
        really_inline operator simd8<uint8_t>() { return uint8_t(' '); }
        template<int N=3>
        really_inline simd8<uint8_t> try_prev_mem(simd8<uint8_t> input) { return input.prev<N>(*this); }
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
        template<int N=3>
        really_inline simd8<uint8_t> try_prev_mem(simd8<uint8_t> input) { return input.prev<N>(*this); }
      };

      template<int N>
      really_inline simd8<uint8_t> try_prev_mem(mid_uncached prev, UNUSED simd8<uint8_t> input) { return prev.buf - N; }
      template<int N>
      really_inline simd8<uint8_t> try_prev_mem(start_or_end prev, simd8<uint8_t> input) { return input.prev<N>(prev); }
    }
  }
}