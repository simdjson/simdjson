// Runs the annotation tests with the ordered per-member deserialization path
// (one obj[key] lookup per member) instead of the default key_selector path.
#define SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION 1
#include "static_reflection_annotations_tests.cpp"
