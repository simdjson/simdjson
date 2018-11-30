#include "jsonparser/jsonparser.h"

// allocate a ParsedJson structure that can support document
// up to len bytes.
// returns NULL if memory cannot be allocated
// This structure is meant to be reused from document to document, as needed.
// you can use deallocate_ParsedJson to deallocate the memory.
ParsedJson *allocate_ParsedJson(size_t len, size_t maxdepth) {
  if((maxdepth == 0) || (len == 0)) {
    std::cerr << "capacities must be non-zero " << std::endl;
    return NULL;
  }
  ParsedJson *pj_ptr = new ParsedJson;
  if (pj_ptr == NULL) {
    std::cerr << "Could not allocate memory for core struct." << std::endl;
    return NULL;
  }
  ParsedJson &pj(*pj_ptr);
  pj.bytecapacity = 0; // will only set it to len after allocations are a success
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
  size_t tapecapacity = ROUNDUP_N(len, 64); 
  size_t stringcapacity = ROUNDUP_N(len, 64);
  pj.string_buf = new u8[stringcapacity];
  pj.tape = new u64[tapecapacity];
  pj.containing_scope_offset = new u32[maxdepth];
  pj.ret_address = new void*[maxdepth];

  if ((pj.string_buf == NULL) || (pj.tape == NULL) 
    || (pj.containing_scope_offset == NULL) || (pj.ret_address == NULL) )  {
    std::cerr << "Could not allocate memory"
              << std::endl;
    delete[] pj.ret_address;
    delete[] pj.containing_scope_offset;
    delete[] pj.tape;
    delete[] pj.string_buf;
    delete[] pj.structural_indexes;
    delete[] pj.structurals;
    delete pj_ptr;
    return NULL;
  }

  pj.bytecapacity = len;
  pj.depthcapacity =  maxdepth;
  pj.tapecapacity = tapecapacity;
  pj.stringcapacity = stringcapacity;
  return pj_ptr;
}

void deallocate_ParsedJson(ParsedJson *pj_ptr) {
  if (pj_ptr == NULL)
    return;
  delete[] pj_ptr->ret_address;
  delete[] pj_ptr->containing_scope_offset;
  delete[] pj_ptr->tape;
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

