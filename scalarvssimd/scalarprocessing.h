#include "common_defs.h"
#include "jsonstruct.h"

bool is_valid_escape(char c) {
  return (c == '"') ||  (c == '\\') ||  (c == '/') ||  (c == 'b') ||  (c == 'f') ||  (c == 'n') ||  (c == 'r') ||  (c == 't') ||  (c == 'u');
}

bool scalar_json_parse(const u8 * buf,  size_t len, ParsedJson & pj) {
  // this is a naive attempt at this point
  // it will probably be subject to failures given adversarial inputs
  size_t pos = 0;
  size_t last = 0;
  size_t up = 0;

  const u32 DUMMY_NODE = 0;
  const u32 ROOT_NODE = 1;
  pj.structural_indexes[DUMMY_NODE] = 0;
  pj.structural_indexes[ROOT_NODE] = 0;
  JsonNode & dummy = pj.nodes[DUMMY_NODE];
  JsonNode & root = pj.nodes[ROOT_NODE];
  dummy.prev = dummy.up = DUMMY_NODE;
  dummy.next = 0;
  root.prev = DUMMY_NODE;
  root.up = ROOT_NODE;
  root.next = 0;

  last = up = ROOT_NODE;

  pos = 2;
  for(size_t i = 0; i < len; i++) {
    JsonNode & n = pj.nodes[pos];
    switch (buf[i]) {
      case '[':
      case '{':
            pj.structural_indexes[pos] = i;
            n.prev = last;
            pj.nodes[last].next = pos;// two-way linked list
            n.up = up;
            up = pos;// new possible up
            last = 0;
            pos += 1;

            break;
      case ']':
      case '}':
            pj.structural_indexes[pos] = i;
            n.prev = up;
            n.next = 0;// necessary?
            pj.nodes[up].next = pos;// two-way linked list
            n.up = pj.nodes[up].up;
            up = pj.nodes[up].up;
            last = pos;// potential previous
            pos += 1;
            break;
      case '"':
      case ':':
      case ',':
          pj.structural_indexes[pos] = i;
          n.prev = last;
          n.next = 0;// necessary
          pj.nodes[last].next = pos;// two-way linked list
          n.up = up;
          last = pos;// potential previous
          pos += 1;
          break;
      case '\\':
          if(i == len - 1) return false;
          if(!is_valid_escape(buf[i+1])) return false;
          i = i + 1; // skip valid escape
      default:
          // nothing
          break;
    }

  }
  pj.n_structural_indexes = pos;
  dummy.next = DUMMY_NODE; // dummy.next is a sump for meaningless 'nexts', clear it
  return true;
}
