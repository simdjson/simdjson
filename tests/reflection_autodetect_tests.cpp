/**
 * Checks that static reflection is detected consistently.
 *
 * simdjson decides whether the reflection-based APIs are available from the
 * compiler's feature-test macros (see simdjson/compiler_check.h). This test
 * verifies two things:
 *
 *  - the value the headers computed matches what the build system expected,
 *    passed in as SIMDJSON_EXPECT_STATIC_REFLECTION;
 *  - whichever way the detection went, the corresponding API actually works.
 *
 * It is also compiled directly against the single-header amalgamation in CI,
 * where the expected value is given on the command line instead, so that the
 * "compile simdjson.cpp with -std=c++26 -freflection and reflection just
 * works" promise is checked without any build system at all.
 */
#include "simdjson.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if SIMDJSON_STATIC_REFLECTION
struct autodetect_address {
  std::string city;
  int64_t zip;
};

struct autodetect_person {
  std::string name;
  int64_t age;
  [[= simdjson::rename<"e_mail">]] std::string email;
  [[= simdjson::skip]] int64_t internal;
  autodetect_address address;
  std::vector<int64_t> scores;
};
#endif

// Round-trips a document through the reflection-based APIs.
static bool reflection_api_works() {
#if SIMDJSON_STATIC_REFLECTION
  std::string json = R"({"name":"Ada","age":36,"e_mail":"ada@example.com",
      "address":{"city":"London","zip":12345},"scores":[1,2,3]})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  if (parser.iterate(simdjson::pad(json)).get(doc)) {
    printf("could not parse\n");
    return false;
  }
  autodetect_person person{};
  if (doc.get<autodetect_person>().get(person)) {
    printf("could not deserialize\n");
    return false;
  }
  if (person.name != "Ada" || person.age != 36 ||
      person.email != "ada@example.com" || person.address.city != "London" ||
      person.address.zip != 12345 || person.scores.size() != 3) {
    printf("deserialized the wrong values\n");
    return false;
  }
  // The skipped member is neither read nor written.
  if (person.internal != 0) {
    printf("skipped member was populated\n");
    return false;
  }
  std::string out;
  if (simdjson::to_json(person).get(out)) {
    printf("could not serialize\n");
    return false;
  }
  if (out.find("\"e_mail\"") == std::string::npos) {
    printf("renamed key missing from %s\n", out.c_str());
    return false;
  }
  if (out.find("internal") != std::string::npos) {
    printf("skipped key present in %s\n", out.c_str());
    return false;
  }
  return true;
#else
  return true;
#endif
}

// The ordinary API has to keep working when reflection is unavailable (and
// when it is available, for that matter).
static bool plain_api_works() {
  std::string json = R"({"a":[1,2,3]})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  if (parser.iterate(simdjson::pad(json)).get(doc)) {
    printf("could not parse\n");
    return false;
  }
  // Initialized only to keep GCC quiet: it cannot see that get() leaves value
  // untouched exactly on the path that returns early, so -Wmaybe-uninitialized
  // fires, and the CI builds use -Werror.
  int64_t value{};
  if (doc["a"].at(1).get(value)) {
    printf("could not read a[1]\n");
    return false;
  }
  if (value != 2) {
    printf("a[1] is %lld, expected 2\n", static_cast<long long>(value));
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  int expected = -1;
#ifdef SIMDJSON_EXPECT_STATIC_REFLECTION
  expected = SIMDJSON_EXPECT_STATIC_REFLECTION;
#endif
  if (argc > 1) {
    expected = std::atoi(argv[1]);
  }
  printf("SIMDJSON_STATIC_REFLECTION = %d\n", SIMDJSON_STATIC_REFLECTION);
  if (expected >= 0 && expected != SIMDJSON_STATIC_REFLECTION) {
    printf("FAIL: expected SIMDJSON_STATIC_REFLECTION = %d\n", expected);
    return EXIT_FAILURE;
  }
  if (!plain_api_works()) {
    printf("FAIL: the ordinary API is broken\n");
    return EXIT_FAILURE;
  }
  if (!reflection_api_works()) {
    printf("FAIL: the reflection API is broken\n");
    return EXIT_FAILURE;
  }
  printf("Success!\n");
  return EXIT_SUCCESS;
}
