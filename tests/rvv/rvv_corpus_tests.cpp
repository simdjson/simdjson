#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Corpus Tests
// -------------------------------------------------------------------------
// This suite attempts to parse standard JSON corpus files (like twitter.json)
// if they are available in the runtime environment.
//
// Purpose:
// 1. Verify stability on large, real-world datasets.
// 2. Ensure no segfaults/errors occur on complex, deeply nested, or 
//    large-volume data.
//
// Note: This test is "soft" - if the corpus files are missing, it skips 
// them rather than failing, making it safe for minimal CI environments.
// -------------------------------------------------------------------------

namespace fs = std::filesystem;

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
bool force_rvv() {
    for (auto implementation : simdjson::get_available_implementations()) {
        if (implementation->name() == "rvv") {
            simdjson::get_active_implementation() = implementation;
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------
// Test Logic
// -------------------------------------------------------------------------
bool test_file(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cout << "[SKIP] File not found: " << filename << std::endl;
        return true; // Not a failure, just missing data
    }

    std::cout << "Testing file: " << filename << " ... ";
    
    simdjson::padded_string json;
    auto load_err = simdjson::padded_string::load(filename).get(json);
    if (load_err) {
        std::cout << "FAILED to load (Error: " << load_err << ")" << std::endl;
        return false;
    }

    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    
    // Parse
    auto parse_err = parser.parse(json).get(doc);
    if (parse_err) {
        std::cout << "FAILED to parse (Error: " << parse_err << ")" << std::endl;
        return false;
    }

    // Basic structural sanity check (iterate top level)
    size_t count = 0;
    if (doc.is_array()) {
        for (auto _ : doc.get_array()) { (void)_; count++; }
        std::cout << "OK (Array size: " << count << ")" << std::endl;
    } else if (doc.is_object()) {
        for (auto _ : doc.get_object()) { (void)_; count++; }
        std::cout << "OK (Object size: " << count << ")" << std::endl;
    } else {
        std::cout << "OK (Scalar/Other)" << std::endl;
    }

    return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Corpus Tests                               " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    // Standard corpus files often found in jsonexamples or adjacent directories
    std::vector<std::string> corpus_files = {
        "twitter.json",
        "gsoc-2018.json",
        "github_events.json",
        "citm_catalog.json",
        "canada.json",
        "mesh.json",
        "apache_builds.json",
        "../jsonexamples/twitter.json",      // Common path if running from build
        "../jsonexamples/gsoc-2018.json",
        "../../jsonexamples/twitter.json"    // Common path if running from build/rvv
    };

    // Also allow passing a specific file via CLI
    if (argc > 1) {
        corpus_files.clear();
        corpus_files.push_back(argv[1]);
    }

    bool all_passed = true;
    bool any_found = false;

    for (const auto& file : corpus_files) {
        // Simple de-dupe/exists check logic is inside test_file for skipping
        if (fs::exists(file)) {
            any_found = true;
            if (!test_file(file)) {
                all_passed = false;
            }
        }
    }

    if (!any_found && argc == 1) {
        std::cout << "[NOTE] No corpus files found in standard locations." << std::endl;
        std::cout << "       (This is expected in minimal CI environments)" << std::endl;
        std::cout << "       Usage: ./rvv_corpus_tests <path_to_json>" << std::endl;
    }

    if (all_passed) {
        std::cout << "SUCCESS: All present corpus tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more corpus files failed to parse." << std::endl;
        return 1;
    }
}