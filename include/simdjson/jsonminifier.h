#pragma once

#include <cstddef>
#include <cstdint>

// Take input from buf and remove useless whitespace, write it to out; buf and
// out can be the same pointer.
size_t jsonminify(const uint8_t *buf, size_t len, uint8_t *out);
