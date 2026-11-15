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

#include <cstdio>
#include <fstream>
#include <string>

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

// Maps files that end at or just before a page boundary, including the
// allocation granularity on Windows (16 pages), so that the padding past the
// file often does not fit in the file's last page. Every padding byte must
// read as zero, and the document must parse.
bool test_memory_map_padding_near_page_boundary() {
    TEST_START();
    const char *const tmpfilename = "memory_map_page_boundary.json";
    const size_t page_size = simdjson::get_page_size();
    for (size_t pages : {size_t(1), size_t(2), size_t(16), size_t(17)}) {
        for (size_t delta : {size_t(simdjson::SIMDJSON_PADDING), size_t(10), size_t(1), size_t(0)}) {
            const size_t size = pages * page_size - delta;
            // A single string that fills the file and ends 2 bytes before its end.
            std::string json("[\"");
            json.append(size - 4, 'a');
            json += "\"]";
            {
                std::ofstream out(tmpfilename, std::ios::binary);
                if (!out) {
                    std::cout << "Warning: I could not create temporary file " << tmpfilename << std::endl;
                    TEST_SUCCEED();
                }
                out.write(json.data(), std::streamsize(json.size()));
                ASSERT_TRUE(bool(out));
            }
            bool ok = true;
            {
                simdjson::padded_memory_map map(tmpfilename);
                if (!map.is_valid()) {
                    std::cerr << "Failed to memory-map a file of " << size << " bytes" << std::endl;
                    ok = false;
                } else {
                    simdjson::padded_string_view view = map.view();
                    ok = ok && view.size() == size;
                    for (size_t i = 0; i < simdjson::SIMDJSON_PADDING; i++) {
                        ok = ok && view.data()[size + i] == '\0';
                    }
                    simdjson::ondemand::parser parser;
                    simdjson::ondemand::document doc;
                    std::string_view str;
                    ok = ok && parser.iterate(view).get(doc) == simdjson::SUCCESS;
                    ok = ok && doc.at(0).get_string().get(str) == simdjson::SUCCESS;
                    ok = ok && str.size() == size - 4;
                    if (!ok) { std::cerr << "Bad padded view for a file of " << size << " bytes" << std::endl; }
                }
            }
            remove(tmpfilename);
            if (!ok) { return false; }
        }
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
    ok = ok && test_memory_map_padding_near_page_boundary();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    std::cout << "padded_memory_map is disabled in this configuration; "
                 "set SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS=ON in CMake to "
                 "enable it on Windows. Test skipped." << std::endl;
    return EXIT_SUCCESS;
#endif
}
