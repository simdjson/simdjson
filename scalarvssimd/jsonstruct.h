#pragma once


struct JsonNode {
    u32 up;
    u32 next;
    u32 prev;
};

struct ParsedJson {
    u8 * structurals;
    u32 n_structural_indexes;
    u32 * structural_indexes;
    JsonNode * nodes;
};
