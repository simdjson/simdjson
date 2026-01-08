#ifndef SIMDJSON_RVV_END_H
#define SIMDJSON_RVV_END_H

// -------------------------------------------------------------------------
// Macro Cleanup
// -------------------------------------------------------------------------
// We must undefine the implementation macro so that subsequent includes
// (e.g., for other backends like AVX2 or NEON) can define it fresh.
// -------------------------------------------------------------------------

#undef SIMDJSON_IMPLEMENTATION
#undef SIMDJSON_TARGET_RVV

#endif // SIMDJSON_RVV_END_H