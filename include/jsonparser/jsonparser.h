#pragma once

#include "common_defs.h"
#include "jsonioutil.h"
#include "simdjson_internal.h"
#include "stage1_find_marks.h"
#include "stage2_flatten.h"
#include "stage34_unified.h"

// Allocate a ParsedJson structure that can support document
// up to len bytes.
// Return NULL if memory cannot be allocated.
// This structure is meant to be reused from document to document, as needed.
// you can use deallocate_ParsedJson to deallocate the memory.
ParsedJson *allocate_ParsedJson(size_t len);

// deallocate a ParsedJson struct (see allocate_ParsedJson)
void deallocate_ParsedJson(ParsedJson *pj_ptr);

// Parse a document found in buf, need to preallocate ParsedJson.
// Return false in case of a failure.
bool json_parse(const u8 *buf, size_t len, ParsedJson &pj);
