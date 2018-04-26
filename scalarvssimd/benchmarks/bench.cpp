#include "jsonstruct.h"
#include "rapidjson/reader.h" // you have to check in the submodule
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "scalarprocessing.h"
#include "avxprocessing.h"
#include "benchmark.h"
#include "util.h"
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

//colorfuldisplay(ParsedJson & pj, const u8 * buf)
//BEST_TIME_NOCHECK(dividearray32(array, N), , repeat,  N, timings,true);


using namespace rapidjson;
using namespace std;

size_t bogus1 = 0;
size_t bogus2 = 0;
size_t bogus3 = 0;

struct MyHandler {
    bool Null() { bogus1++; return true; }
    bool Bool(bool b) { bogus2++; return true; }
    bool Int(int i) { bogus3++; return true; }
    bool Uint(unsigned u) { bogus2++; return true; }
    bool Int64(int64_t i) { bogus2++; return true; }
    bool Uint64(uint64_t u) { bogus2++; return true; }
    bool Double(double d) { bogus2++; return true; }
    bool RawNumber(const char* str, SizeType length, bool copy) {
        bogus3++;
        return true;
    }
    bool String(const char* str, SizeType length, bool copy) {
        bogus2++;
        return true;
    }
    bool StartObject() { bogus1++; return true; }
    bool Key(const char* str, SizeType length, bool copy) {
        bogus2++;
        return true;
    }
    bool EndObject(SizeType memberCount) { bogus2++; return true; }
    bool StartArray() { bogus2++; return true; }
    bool EndArray(SizeType elementCount) { bogus1++; return true; }
};


int main(int argc, char * argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <jsonfile>\n";
        cerr << "Or " << argv[0] << " -v <jsonfile>\n";
        exit(1);
    }
    bool verbose = false;
    if (argc > 2) {
      if(strcmp(argv[1],"-v")) verbose = true;
    }
    pair<u8 *, size_t> p = get_corpus(argv[argc - 1]);
    ParsedJson pj;
    std::cout << "Input has ";
    if(p.second > 1024 * 1024)
      std::cout << p.second / (1024*1024) << " MB ";
    else if (p.second > 1024)
      std::cout << p.second / 1024 << " KB ";
    else
      std::cout << p.second << " B ";
    std::cout << std::endl;

    if (posix_memalign( (void **)&pj.structurals, 8, ROUNDUP_N(p.second, 64)/8)) {
        throw "Allocation failed";
    };

    pj.n_structural_indexes = 0;
    // we have potentially 1 structure per byte of input
    // as well as a dummy structure and a root structure
    // we also potentially write up to 7 iterations beyond
    // in our 'cheesy flatten', so make some worst-case
    // sapce for that too
    u32 max_structures = ROUNDUP_N(p.second, 64) + 2 + 7;
    pj.structural_indexes = new u32[max_structures];
    pj.nodes = new JsonNode[max_structures];
    if(verbose) {
      std::cout << "Parsing SIMD (once) " << std::endl;
      avx_json_parse(p.first,  p.second, pj);
      colorfuldisplay(pj, p.first);
      debugdisplay(pj,p.first);
      std::cout << "Parsing scalar (once) " << std::endl;
      scalar_json_parse(p.first,  p.second, pj);
      colorfuldisplay(pj, p.first);
      debugdisplay(pj,p.first);
    }

    int repeat = 10;
    int volume = p.second;
    BEST_TIME_NOCHECK(avx_json_parse(p.first,  p.second, pj), , repeat, volume, true);
    BEST_TIME_NOCHECK(scalar_json_parse(p.first,  p.second, pj), , repeat, volume, true);

        rapidjson::Document d;
        char buffer[p.second+1024];
        memcpy(buffer, p.first, p.second);
        buffer[p.second]='\0';
    BEST_TIME(d.Parse((const char *)p.first).HasParseError(), false, memcpy(buffer, p.first, p.second) , repeat, volume, true);
}
