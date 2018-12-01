#pragma once

#include <cstddef>
#include <cstdint>

// Take input from buf and remove useless whitespace, write it to out; buf and
// out can be the same pointer. Result is null terminated, 
// return the string length (minus the null termination).
size_t jsonminify(const uint8_t *buf, size_t len, uint8_t *out);


static inline size_t jsonminify(const char *buf, size_t len, char *out) {
    return jsonminify((const uint8_t *)buf, len, (uint8_t *)out);
}


static inline size_t jsonminify(const simdjsonstring & p, char *out) {
    return jsonminify(p.c_str(), p.size(), out);
}