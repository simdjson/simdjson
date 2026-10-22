#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"
#include <clocale>
#include <cstdio>
#include <cstring>
#include <string>

using namespace simdjson;

// simdjson parses numbers with a locale-independent hand-rolled parser
// (fast_float) and serializes them with a locale-independent formatter
// (Dragonbox). These tests lock in that behavior under a comma-decimal
// locale (see issue #1166): the decimal separator must stay '.' in both
// directions regardless of the C locale.

namespace {

// Restores the LC_NUMERIC locale when it goes out of scope, even if an
// assertion aborts the test. Only the numeric locale is touched, since the
// decimal separator is all these tests care about.
//
// setlocale returns a pointer into a static buffer that the next setlocale
// call overwrites, so we must copy it into a std::string; storing the raw
// pointer leaves a dangling pointer by the time the destructor runs.
class LocaleGuard {
  std::string saved;
  bool active;

public:
  explicit LocaleGuard(const char *name) : saved(), active(false) {
    const char *current = setlocale(LC_NUMERIC, nullptr);
    if (current != nullptr) {
      saved = current;
    }
    if (setlocale(LC_NUMERIC, name) != nullptr) {
      active = true;
    }
  }
  LocaleGuard(const LocaleGuard &) = delete;
  LocaleGuard &operator=(const LocaleGuard &) = delete;
  ~LocaleGuard() {
    if (!saved.empty()) {
      setlocale(LC_NUMERIC, saved.c_str());
    }
  }
  bool is_active() const { return active; }
};

// A successful setlocale does not guarantee a comma-decimal locale (the name
// may silently fall back); confirm the active locale really uses ',' before
// asserting comma-specific behavior.
bool comma_decimal_active() {
  const lconv *info = localeconv();
  return info != nullptr && info->decimal_point != nullptr &&
         std::strcmp(info->decimal_point, ",") == 0;
}

// Parsing must accept a '.' decimal separator even under a comma locale.
bool parsing_accepts_dot() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("1.5"_padded).get(doc));
  double value = 0;
  ASSERT_SUCCESS(doc.get_double().get(value));
  ASSERT_EQUAL(value, 1.5);
  TEST_SUCCEED();
}

// Parsing must NOT treat ',' as a decimal separator; "1,5" is invalid JSON.
bool parsing_rejects_comma() {
  TEST_START();
  dom::parser parser;
  ASSERT_ERROR(parser.parse("1,5"_padded), TAPE_ERROR);
  TEST_SUCCEED();
}

// Serialization must emit '.' as the decimal separator, never ','.
bool serialization_uses_dot() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("1.5"_padded).get(doc));
  std::string text = simdjson::to_string(doc);
  ASSERT(text.find('.') != std::string::npos,
         "serialized number must contain '.'");
  ASSERT(text.find(',') == std::string::npos,
         "serialized number must not contain ','");
  TEST_SUCCEED();
}

bool run() {
  // Try locales that use ',' as the decimal separator. The first one that
  // actually activates a comma-decimal locale is used; if none do, we skip
  // rather than fail (the locale simply is not available on this platform).
  const char *candidates[] = {"de_DE.UTF-8", "de_DE.utf8",
                              "de-DEU",      "German_Germany.1252",
                              "fr_FR.UTF-8", "French_France.1252"};
  for (const char *name : candidates) {
    LocaleGuard guard(name);
    if (!guard.is_active() || !comma_decimal_active()) {
      continue;
    }
    return parsing_accepts_dot() && parsing_rejects_comma() &&
           serialization_uses_dot();
  }
  std::printf("[SKIP] no comma-decimal locale available; "
              "skipping locale-independence tests\n");
  TEST_SUCCEED();
}

} // namespace

int main(int argc, char *argv[]) { return test_main(argc, argv, run); }
