#ifdef _WIN32
#include <cstdlib>
// This test is not supported on Windows because it relies on POSIX APIs like
// mmap. Please run it on a POSIX-compliant system.
int main() { return EXIT_SUCCESS; }
#else

#include "simdjson.h"
#include "test_macros.h"

#if SIMDJSON_EXCEPTIONS
bool test_memory_map_exception() {
  TEST_START();
  simdjson::padded_memory_map map(TWITTER_JSON);
  ASSERT_TRUE(map.is_valid());
  simdjson::padded_string_view view = map.view();
  simdjson::ondemand::document doc = simdjson::ondemand::parser::get_parser().iterate(view);
  ASSERT_TRUE(doc["search_metadata"]["count"].get_uint64() == 100);
  TEST_SUCCEED();
}
#endif

bool test_memory_map_noexception() {
    TEST_START();
    simdjson::padded_memory_map map(TWITTER_JSON);
    if (!map.is_valid()) {
        std::cerr << "Failed to memory-map the file " << TWITTER_JSON << std::endl;
        return false;
    }
    simdjson::padded_string_view view = map.view();
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(view).get(doc) );
    uint64_t count;
    ASSERT_SUCCESS( doc["search_metadata"]["count"].get(count) );
    ASSERT_EQUAL(count, 100);
    TEST_SUCCEED();
}

int main() {
#if SIMDJSON_EXCEPTIONS
    return (test_memory_map_exception() && test_memory_map_noexception()) ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    return test_memory_map_noexception() ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
#endif