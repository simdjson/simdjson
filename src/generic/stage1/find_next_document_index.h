/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ';' ','
  * and when the second element is NOT one of these characters: '}' '}' ';' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
really_inline static uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // TODO don't count separately, just figure out depth
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  return 0;
}

// Skip the last character if it is partial
really_inline static size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0b11110000) { return len-3; } // 4-byte characters with only 3 bytes left
  return len;
}
