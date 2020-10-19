
#include "simdjson.h"

#include <cstdlib>

// this test is needed, because memcpy may be invoked on a null pointer
// otherwise
static void testNullString() {
  std::string_view empty;
  simdjson::padded_string blah(empty);
}

int main() {

  testNullString();
}
