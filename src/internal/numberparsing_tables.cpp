#ifndef SIMDJSON_SRC_NUMBERPARSING_TABLES_CPP
#define SIMDJSON_SRC_NUMBERPARSING_TABLES_CPP

#include <simdjson/base.h>
#include <simdjson/internal/numberparsing_tables.h>

// Precomputed powers of ten from 10^0 to 10^22. These
// can be represented exactly using the double type.
SIMDJSON_DLLIMPORTEXPORT const double simdjson::internal::power_of_ten[] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

/**
 * When mapping numbers from decimal to binary,
 * we go from w * 10^q to m * 2^p but we have
 * 10^q = 5^q * 2^q, so effectively
 * we are trying to match
 * w * 2^q * 5^q to m * 2^p. Thus the powers of two
 * are not a concern since they can be represented
 * exactly using the binary notation, only the powers of five
 * affect the binary significand.
 */



#endif // SIMDJSON_SRC_NUMBERPARSING_TABLES_CPP