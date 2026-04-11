// On Windows, padded_memory_map is an opt-in feature gated on the
// SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS macro. When that macro is set, the
// consumer must also include <windows.h> before <simdjson.h>. We include
// the Win32 header here so that -- in configurations that turned the
// feature on -- the test actually exercises the Windows path.
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "simdjson.h"
#include "test_macros.h"

// When SIMDJSON_HAS_PADDED_MEMORY_MAP is 0 (e.g. Windows builds without
// SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS, or MinGW configurations that lack
// the required SDK gating), compile the test body out and make main()
// report success so the test suite still runs as a no-op. This is not a
// silent downgrade: users who want the Windows path must explicitly
// enable the CMake option `SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS`.
#if SIMDJSON_HAS_PADDED_MEMORY_MAP

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

// Verifies that padded_memory_map can feed a streaming parser (iterate_many)
// with JSON documents read from a file. This exercises the API that parse_many
// / iterate_many users typically want: no extra copy on POSIX, portable fallback
// on Windows. The AMAZON_CELLPHONES_NDJSON resource is an NDJSON file so it is
// a realistic stress-test for streaming from a memory-mapped file.
bool test_memory_map_iterate_many() {
    TEST_START();
    simdjson::padded_memory_map map(AMAZON_CELLPHONES_NDJSON);
    if (!map.is_valid()) {
        std::cerr << "Failed to memory-map the file " << AMAZON_CELLPHONES_NDJSON << std::endl;
        return false;
    }
    simdjson::padded_string_view view = map.view();
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document_stream stream;
    ASSERT_SUCCESS( parser.iterate_many(view).get(stream) );
    size_t count = 0;
    for (auto doc : stream) {
        ASSERT_SUCCESS( doc.error() );
        count++;
    }
    if (count == 0) {
        std::cerr << "Expected at least one document in " << AMAZON_CELLPHONES_NDJSON << std::endl;
        return false;
    }
    TEST_SUCCEED();
}

// Verifies that padded_memory_map also works with the DOM streaming parser
// (parse_many). Same rationale as the ondemand variant above.
bool test_memory_map_parse_many() {
    TEST_START();
    simdjson::padded_memory_map map(AMAZON_CELLPHONES_NDJSON);
    if (!map.is_valid()) {
        std::cerr << "Failed to memory-map the file " << AMAZON_CELLPHONES_NDJSON << std::endl;
        return false;
    }
    simdjson::padded_string_view view = map.view();
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(view).get(stream) );
    size_t count = 0;
    for (auto doc : stream) {
        ASSERT_SUCCESS( doc.error() );
        count++;
    }
    if (count == 0) {
        std::cerr << "Expected at least one document in " << AMAZON_CELLPHONES_NDJSON << std::endl;
        return false;
    }
    TEST_SUCCEED();
}

// Ensures that trying to memory-map a file that does not exist leaves the map
// in the "invalid" state rather than crashing. This is important on Windows
// where the underlying implementation path differs from POSIX.
bool test_memory_map_missing_file() {
    TEST_START();
    simdjson::padded_memory_map map("this_file_definitely_does_not_exist_123456789.json");
    if (map.is_valid()) {
        std::cerr << "Expected is_valid() == false for missing file" << std::endl;
        return false;
    }
    TEST_SUCCEED();
}

#endif // SIMDJSON_HAS_PADDED_MEMORY_MAP

int main() {
#if SIMDJSON_HAS_PADDED_MEMORY_MAP
    bool ok = true;
#if SIMDJSON_EXCEPTIONS
    ok = ok && test_memory_map_exception();
#endif
    ok = ok && test_memory_map_noexception();
    ok = ok && test_memory_map_iterate_many();
    ok = ok && test_memory_map_parse_many();
    ok = ok && test_memory_map_missing_file();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    std::cout << "padded_memory_map is disabled in this configuration; "
                 "set SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS=ON in CMake to "
                 "enable it on Windows. Test skipped." << std::endl;
    return EXIT_SUCCESS;
#endif
}
