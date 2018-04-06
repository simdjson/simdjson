#pragma once

#include "common_defs.h"

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

#include <algorithm>
#include <iostream>
#include <iterator>

// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
namespace Color {
    enum Code {
        FG_DEFAULT = 39, FG_BLACK = 30, FG_RED = 31, FG_GREEN = 32,
        FG_YELLOW = 33, FG_BLUE = 34, FG_MAGENTA = 35, FG_CYAN = 36,
        FG_LIGHT_GRAY = 37, FG_DARK_GRAY = 90, FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92, FG_LIGHT_YELLOW = 93, FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95, FG_LIGHT_CYAN = 96, FG_WHITE = 97,
        BG_RED = 41, BG_GREEN = 42, BG_BLUE = 44, BG_DEFAULT = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}

void colorfuldisplay(ParsedJson & pj, const u8 * buf) {
    Color::Modifier greenfg(Color::FG_GREEN);
    Color::Modifier yellowfg(Color::FG_YELLOW);
    Color::Modifier deffg(Color::FG_DEFAULT);
    size_t i = 0;
    // skip initial fluff
    while((i+1< pj.n_structural_indexes) && (pj.structural_indexes[i]==pj.structural_indexes[i+1])){
      i++;
    }
    for (; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        u8 c = buf[idx];
        if (((c & 0xdf) == 0x5b)) { // meaning 7b or 5b, { or [
            std::cout << greenfg <<  buf[idx] << deffg;
        } else if (((c & 0xdf) == 0x5d)) { // meaning 7d or 5d, } or ]
            std::cout << greenfg <<  buf[idx] << deffg;
        } else {
            std::cout << yellowfg <<  buf[idx] << deffg;
        }
        if(i + 1 < pj.n_structural_indexes) {
          u32 nextidx = pj.structural_indexes[i + 1];
          for(u32 pos = idx + 1 ; pos < nextidx; pos++) {
            std::cout << buf[pos];
          }
        }
    }
    std::cout << std::endl;
}

void debugdisplay(ParsedJson & pj, const u8 * buf) {
    for (u32 i = 0; i < pj.n_structural_indexes; i++) {
        u32 idx = pj.structural_indexes[i];
        JsonNode & n = pj.nodes[i];
        std::cout << "i: " << i;
        std::cout << " n.up: " << n.up;
        std::cout << " n.next: " << n.next;
        std::cout << " n.prev: " << n.prev;
        std::cout << " idx: " << idx << " buf[idx] " << buf[idx] << std::endl;
    }
}
