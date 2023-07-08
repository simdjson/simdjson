#include "simdjson/fallback/implementation.h"

#define SIMDJSON_IMPLEMENTATION fallback
#define SIMDJSON_IMPLEMENTATION_MASK (1<<0)

#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/fallback/bitmanipulation.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/fallback/stringparsing.h"
#include "simdjson/fallback/numberparsing.h"
