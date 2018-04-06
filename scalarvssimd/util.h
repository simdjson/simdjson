#include "common_defs.h"

// get a corpus; pad out to cache line so we can always use SIMD
static pair<u8 *, size_t> get_corpus(string filename) {
    ifstream is(filename, ios::binary);
    if (is) {
        stringstream buffer;
        buffer << is.rdbuf();
        size_t length = buffer.str().size();
        char * aligned_buffer;
        if (posix_memalign( (void **)&aligned_buffer, 64, ROUNDUP_N(length, 64))) {
            throw "Allocation failed";
        };
        memset(aligned_buffer, 0x20, ROUNDUP_N(length, 64));
        memcpy(aligned_buffer, buffer.str().c_str(), length);
        is.close();
        return make_pair((u8 *)aligned_buffer, length);
    }
    throw "No corpus";
    return make_pair((u8 *)0, (size_t)0);
}
