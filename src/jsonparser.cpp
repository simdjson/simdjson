#include "jsonparser/jsonparser.h"

// allocate a ParsedJson structure that can support document
// up to len bytes.
// returns NULL if memory cannot be allocated
// This structure is meant to be reused from document to document, as needed.
// you can use deallocate_ParsedJson to deallocate the memory.
ParsedJson *allocate_ParsedJson(size_t len) {
  /*if (len > MAX_JSON_BYTES) {
    std::cerr << "Currently only support JSON files having up to "<<MAX_JSON_BYTES<<" bytes, requested length: "
              << len << std::endl;
    return NULL;
  }*/
  ParsedJson *pj_ptr = new ParsedJson;
  if (pj_ptr == NULL) {
    std::cerr << "Could not allocate memory for core struct." << std::endl;
    return NULL;
  }
  ParsedJson &pj(*pj_ptr);
  pj.bytecapacity = len;
  if (posix_memalign((void **)&pj.structurals, 8, ROUNDUP_N(len, 64) / 8)) {
    std::cerr << "Could not allocate memory for structurals" << std::endl;
    delete pj_ptr;
    return NULL;
  };
  pj.n_structural_indexes = 0;
  u32 max_structures = ROUNDUP_N(len, 64) + 2 + 7;
  pj.structural_indexes = new u32[max_structures];

  if (pj.structural_indexes == NULL) {
    std::cerr << "Could not allocate memory for structural_indexes"
              << std::endl;
    delete[] pj.structurals;
    delete pj_ptr;
    return NULL;
  }
  pj.string_buf = new u8[len];
  if (pj.string_buf == NULL) {
    std::cerr << "Could not allocate memory for string_buf"
              << std::endl;
    delete[] pj.structural_indexes;
    delete[] pj.structurals;
    delete pj_ptr;
    return NULL;
  }
  pj.number_buf = new u8[4 * len];
  if (pj.string_buf == NULL) {
    std::cerr << "Could not allocate memory for number_buf"
              << std::endl;
    delete[] pj.string_buf;
    delete[] pj.structural_indexes;
    delete[] pj.structurals;
    delete pj_ptr;
    return NULL;
  }

  return pj_ptr;
}

void deallocate_ParsedJson(ParsedJson *pj_ptr) {
  if (pj_ptr == NULL)
    return;
  delete[] pj_ptr->number_buf;
  delete[] pj_ptr->string_buf;
  delete[] pj_ptr->structural_indexes;
  free(pj_ptr->structurals);
  delete pj_ptr;
}

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
  }
  if (isok) {
    isok = unified_machine(buf, len, pj);
  }
  return isok;
}

