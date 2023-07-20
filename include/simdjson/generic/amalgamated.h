#if defined(SIMDJSON_CONDITIONAL_INCLUDE) && !defined(SIMDJSON_GENERIC_DEPENDENCIES_H)
#error simdjson/generic/dependencies.h must be included before simdjson/generic/amalgamated.h!
#endif

#include "simdjson/generic/base.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/numberparsing.h"

#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
