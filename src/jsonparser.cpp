#include "simdjson/jsonparser.h"


// parse a document found in buf, need to preallocate ParsedJson.
bool json_parse(const u8 *buf, size_t len, ParsedJson &pj) {
  if (pj.bytecapacity < len) {
    std::cerr << "Your ParsedJson cannot support documents that big: " << len
              << std::endl;
    return false;
  }
  bool isok = find_structural_bits(buf, len, pj);
  if (isok) {
    isok = flatten_indexes(len, pj);
  } else {
    return false;
  }
  if (isok) {
    isok = unified_machine(buf, len, pj);
  } else {
    return false;
  }
  return isok;
}

