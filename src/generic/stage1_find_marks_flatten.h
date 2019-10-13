// This file contains a non-architecture-specific version of "flatten" used in stage1.
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

class bit_indexer {
public:
  uint32_t *tail;

  bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // this->tail[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so this->tail
  // needs to be large enough to handle this
  really_inline void write_indexes(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
      return;
    uint32_t cnt = hamming(bits);

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = bits & (bits - 1);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every 4 characters.
      if (unlikely(cnt > 16)) {
        uint32_t i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = bits & (bits - 1);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};