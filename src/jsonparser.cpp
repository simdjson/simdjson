#include "simdjson/jsonparser.h"


// parse a document found in buf, need to preallocate ParsedJson.
WARN_UNUSED
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
  }//printf("ok\n");
  if (isok) {
    isok = unified_machine(buf, len, pj);
    //printf("ok %d \n",isok);
  } else {
    return false;
  }
  return isok;
}

WARN_UNUSED
ParsedJson build_parsed_json(const u8 *buf, size_t len) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    ok = json_parse(buf, len, pj);
    assert(ok == pj.isValid());
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
