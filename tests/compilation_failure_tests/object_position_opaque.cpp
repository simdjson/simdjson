
// this tests https://github.com/simdjson/simdjson/issues/1952
//
// object_position must not be constructible outside of object: a hand-built
// instance would let object::revert_position() jump to an arbitrary,
// unvalidated position and depth. Only object::get_current_position() may
// produce a valid instance.

#include "simdjson.h"

int main() {
#if COMPILATION_TEST_USE_FAILING_CODE
    simdjson::ondemand::object_position bogus{nullptr, 1};
#else
    simdjson::ondemand::object_position bogus;
#endif
    (void)bogus;
}
