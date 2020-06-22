namespace stage1 {
/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool utf8_validate(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    c.check_next_input(in);
    reader.advance();
    return c.errors() == error_code::SUCCESS;
}

bool utf8_validate(const char * input, size_t length) {
    return utf8_validate<utf8_checker>((const uint8_t *)input,length);
}

} // namespace stage1