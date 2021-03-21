//example for a compile test

// keep as much code as possible outside of the conditional compilation
// so the thing that is to proven to not compile fails to compile because
// some unrelated thing (like forgetting to include a header)

#include <vector>

int main() {

#if COMPILATION_TEST_USE_FAILING_CODE
    // code that is supposed to fail compiling (keep it short!)
    xxx
#else
   // equivalent code that is supposed to compile (keep it short!)
#endif

    // common code

}
