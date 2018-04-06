#include "common_defs.h"
#include "jsonstruct.h"

bool scalar_json_parse(const u8 * buf,  size_t len, ParsedJson & pj) {
  // this is a naive attempt at this point
  // it will probably be subject to failures given adversarial inputs
  size_t pos = 0;
  size_t last = 0;
  size_t up = 0;
  for(size_t i = 0; i < len; i++) {
    JsonNode & n = pj.nodes[pos];
    switch buf[i] {
      case '[':
      case '{':
            n.prev = last;
            n.up = up;
            up = pos;
            last = 0;
            pos += 1;
            break;
      case ']':
      case '}':
            n.prev = up;
            n.up = pj.nodes[up].up;
            up = pj.nodes[up].up;
            last = pos;
            pos += 1;
            break;

      case ':':
      case ',':
          n.prev = last;
          n.up = up;
          last = pos;
          pos += 1;
          break;
      default:
          // nothing
    }
    n.next = 0;
    nodes[n.prev].next = pos;
  }
  pj.n_structural_indexes = pos;
}
