#include "jsonstruct.h"

#include "scalarprocessing.h"
#include "avxprocessing.h"
#include "benchmark.h"
#include "util.h"

//colorfuldisplay(ParsedJson & pj, const u8 * buf)
//BEST_TIME_NOCHECK(dividearray32(array, N), , repeat,  N, timings,true);


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

}
