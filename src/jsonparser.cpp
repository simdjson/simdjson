#include "simdjson/jsonparser.h"
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif
#include "simdjson/simdjson.h"

// parse a document found in buf, need to preallocate ParsedJson.
WARN_UNUSED
int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  if (pj.bytecapacity < len) {
    return simdjson::CAPACITY;
  }
  bool reallocated = false;
  if(reallocifneeded) {
#ifdef ALLOW_SAME_PAGE_BUFFER_OVERRUN
	  // realloc is needed if the end of the memory crosses a page
#ifdef _MSC_VER
	  SYSTEM_INFO sysInfo; 
	  GetSystemInfo(&sysInfo); 
	  long pagesize = sysInfo.dwPageSize;
#else
    long pagesize = sysconf (_SC_PAGESIZE); 
#endif
  //////////////
  // We want to check that buf + len - 1 and buf + len - 1 + SIMDJSON_PADDING
  // are in the same page.
  // That is, we want to check that  
  // (buf + len - 1) / pagesize == (buf + len - 1 + SIMDJSON_PADDING) / pagesize
  // That's true if (buf + len - 1) % pagesize + SIMDJSON_PADDING < pagesize.
  ///////////
	 if ( (reinterpret_cast<uintptr_t>(buf + len - 1) % pagesize ) + SIMDJSON_PADDING < static_cast<uintptr_t>(pagesize) ) {
#else // SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN
     if(true) { // if not SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN, we always reallocate
#endif
	     const uint8_t *tmpbuf  = buf;
       buf = (uint8_t *) allocate_padded_buffer(len);
       if(buf == NULL) return simdjson::MEMALLOC;
       memcpy((void*)buf,tmpbuf,len);
       reallocated = true;
     }
  }
  bool stage1_is_ok = find_structural_bits(buf, len, pj);
  if(!stage1_is_ok) {
    return simdjson::UTF8_ERROR;
  }
  int res = unified_machine(buf, len, pj);
  if(reallocated) { aligned_free((void*)buf);}
  return res;
}

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    int res = json_parse(buf, len, pj, reallocifneeded);
    ok = res == simdjson::SUCCESS;
    assert(ok == pj.isValid());
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
