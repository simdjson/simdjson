#include <iostream>
#include <string>
#include <vector>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Tool: DOM Parse CLI
// -------------------------------------------------------------------------
// Purpose:
// 1. Allow manual testing of the RVV backend against arbitrary JSON files.
//    Usage: ./rvv_smoke_dom_parse <filename.json>
// 2. If no file is provided, runs a default hardcoded correctness check.
// -------------------------------------------------------------------------

// Helper to select RVV
bool force_rvv() {
    for (auto implementation : simdjson::get_available_implementations()) {
        if (implementation->name() == "rvv") {
            simdjson::get_active_implementation() = implementation;
            return true;
        }
    }
    return false;
}

void print_doc_stats(simdjson::dom::element doc) {
    std::cout << "Document Type: ";
    switch (doc.type()) {
        case simdjson::dom::element_type::ARRAY:
            std::cout << "Array, Size: " << doc.get_array().value().size() << std::endl;
            break;
        case simdjson::dom::element_type::OBJECT:
            std::cout << "Object, Size: " << doc.get_object().value().size() << std::endl;
            break;
        case simdjson::dom::element_type::INT64:
            std::cout << "Int64: " << int64_t(doc) << std::endl;
            break;
        case simdjson::dom::element_type::UINT64:
            std::cout << "UInt64: " << uint64_t(doc) << std::endl;
            break;
        case simdjson::dom::element_type::DOUBLE:
            std::cout << "Double: " << double(doc) << std::endl;
            break;
        case simdjson::dom::element_type::STRING:
            std::cout << "String: \"" << std::string_view(doc) << "\"" << std::endl;
            break;
        case simdjson::dom::element_type::BOOL:
            std::cout << "Bool: " << (bool(doc) ? "true" : "false") << std::endl;
            break;
        case simdjson::dom::element_type::NULL_VALUE:
            std::cout << "Null" << std::endl;
            break;
    }
}

int main(int argc, char *argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV DOM Parse CLI                              " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    simdjson::error_code error;

    if (argc > 1) {
        // ---------------------------------------------------------------------
        // Mode 1: File Argument (Manual Bisect)
        // ---------------------------------------------------------------------
        const char* filename = argv[1];
        std::cout << "Loading file: " << filename << std::endl;

        error = parser.load(filename).get(doc);
        if (error) {
            std::cerr << "[FAIL] Failed to load/parse file: " << error << std::endl;
            return 1;
        }

        std::cout << "[PASS] Parse Successful." << std::endl;
        print_doc_stats(doc);

    } else {
        // ---------------------------------------------------------------------
        // Mode 2: Default Smoketest (Hardcoded)
        // ---------------------------------------------------------------------
        std::cout << "No file provided. Running default verification..." << std::endl;

        auto json = R"({ "status": "ok", "values": [1, 2, 3], "nested": { "a": true } })"_padded;

        error = parser.parse(json).get(doc);
        if (error) {
            std::cerr << "[FAIL] Default JSON parse failed: " << error << std::endl;
            return 1;
        }

        // Quick verify
        bool ok = true;
        std::string_view status;
        if (doc["status"].get(status) || status != "ok") ok = false;

        simdjson::dom::array arr;
        if (doc["values"].get(arr) || arr.size() != 3) ok = false;

        if (ok) {
            std::cout << "[PASS] Default JSON verification." << std::endl;
        } else {
            std::cerr << "[FAIL] Default JSON content mismatch." << std::endl;
            return 1;
        }
    }

    return 0;
}
