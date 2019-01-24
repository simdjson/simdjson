#include "simdjson/jsonparser.h"
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif


extern bool json_parse(const char * buf, size_t len, ParsedJson &pj, bool reallocifneeded);
extern bool json_parse(const std::string_view &s, ParsedJson &pj, bool reallocifneeded);
extern ParsedJson build_parsed_json(const char * buf, size_t len, bool reallocifneeded);
extern ParsedJson build_parsed_json(const std::string_view &s, bool reallocifneeded);


// parse a document found in buf, need to preallocate ParsedJson.
WARN_UNUSED
bool json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  if (pj.bytecapacity < len) {
    std::cerr << "Your ParsedJson cannot support documents that big: " << len
              << std::endl;
    return false;
  }
  bool reallocated = false;
  if(reallocifneeded) {
      // realloc is needed if the end of the memory crosses a page
#ifdef _MSC_VER
	  SYSTEM_INFO sysInfo; 
	  GetSystemInfo(&sysInfo); 
	  long pagesize = sysInfo.dwPageSize;
#else
     long pagesize = sysconf (_SC_PAGESIZE); 
#endif
	 if ( (reinterpret_cast<uintptr_t>(buf + len - 1) % pagesize ) < SIMDJSON_PADDING ) {
       const uint8_t *tmpbuf  = buf;
       buf = (uint8_t *) allocate_padded_buffer(len);
       if(buf == NULL) return false;
       memcpy((void*)buf,tmpbuf,len);
       reallocated = true;
     }
  }
  bool isok = find_structural_bits(buf, len, pj);
  if (isok) {
    isok = unified_machine(buf, len, pj);
  } else {
    if(reallocated) free((void*)buf);
    return false;
  }
  if(reallocated) free((void*)buf);
  return isok;
}

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    ok = json_parse(buf, len, pj, reallocifneeded);
    assert(ok == pj.isValid());
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
