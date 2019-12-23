#include "simdjson/parsedjson.h"
#include "simdjson/jsonformatutils.h"

namespace simdjson {

WARN_UNUSED
bool ParsedJson::allocate_capacity(size_t len, size_t max_depth) {
  if (max_depth <= 0) {
    max_depth = 1; // don't let the user allocate nothing
  }
  if (len <= 0) {
    len = 64; // allocating 0 bytes is wasteful.
  }
  if (len > SIMDJSON_MAXSIZE_BYTES) {
    return false;
  }
  if ((len <= byte_capacity) && (max_depth <= depth_capacity)) {
    return true;
  }
  deallocate();
  valid = false;
  byte_capacity = 0; // will only set it to len after allocations are a success
  n_structural_indexes = 0;
  uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]);

  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of at least len + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6" 
  //where len + 1 tape elements are
  // generated, see issue https://github.com/lemire/simdjson/issues/345
  size_t local_tape_capacity = ROUNDUP_N(len + 2, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t local_string_capacity = ROUNDUP_N(5 * len / 3 + 32, 64);
  string_buf.reset( new (std::nothrow) uint8_t[local_string_capacity]);
  tape.reset(new (std::nothrow) uint64_t[local_tape_capacity]);
  containing_scope_offset.reset(new (std::nothrow) uint32_t[max_depth]);
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  //ret_address = new (std::nothrow) void *[max_depth];
  ret_address.reset(new (std::nothrow) void *[max_depth]);
#else
  ret_address.reset(new (std::nothrow) char[max_depth]);
#endif
  if (!string_buf || !tape ||
      !containing_scope_offset || !ret_address ||
      !structural_indexes) {
    std::cerr << "Could not allocate memory" << std::endl;
    return false;
  }
  /*
  // We do not need to initialize this content for parsing, though we could
  // need to initialize it for safety.
  memset(string_buf, 0 , local_string_capacity);
  memset(structural_indexes, 0, max_structures * sizeof(uint32_t));
  memset(tape, 0, local_tape_capacity * sizeof(uint64_t));
  */
  byte_capacity = len;
  depth_capacity = max_depth;
  tape_capacity = local_tape_capacity;
  string_capacity = local_string_capacity;
  return true;
}

bool ParsedJson::is_valid() const { return valid; }

int ParsedJson::get_error_code() const { return error_code; }

std::string ParsedJson::get_error_message() const {
  return error_message(error_code);
}

void ParsedJson::deallocate() {
  byte_capacity = 0;
  depth_capacity = 0;
  tape_capacity = 0;
  string_capacity = 0;
  ret_address.reset();
  containing_scope_offset.reset();
  tape.reset();
  string_buf.reset();
  structural_indexes.reset();
  valid = false;
}

void ParsedJson::init() {
  current_string_buf_loc = string_buf.get();
  current_loc = 0;
  valid = false;
}

WARN_UNUSED
bool ParsedJson::print_json(std::ostream &os) const {
  if (!valid) {
    return false;
  }
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = (tape_val >> 56);
  size_t how_many = 0;
  if (type == 'r') {
    how_many = tape_val & JSON_VALUE_MASK;
  } else {
    fprintf(stderr, "Error: no starting root node?");
    return false;
  }
  if (how_many > tape_capacity) {
    fprintf(
        stderr,
        "We may be exceeding the tape capacity. Is this a valid document?\n");
    return false;
  }
  tape_idx++;
  std::unique_ptr<bool[]> in_object(new bool[depth_capacity]);
  std::unique_ptr<size_t[]> in_object_idx(new size_t[depth_capacity]);
  int depth = 1; // only root at level 0
  in_object_idx[depth] = 0;
  in_object[depth] = false;
  for (; tape_idx < how_many; tape_idx++) {
    tape_val = tape[tape_idx];
    uint64_t payload = tape_val & JSON_VALUE_MASK;
    type = (tape_val >> 56);
    if (!in_object[depth]) {
      if ((in_object_idx[depth] > 0) && (type != ']')) {
        os << ",";
      }
      in_object_idx[depth]++;
    } else { // if (in_object) {
      if ((in_object_idx[depth] > 0) && ((in_object_idx[depth] & 1) == 0) &&
          (type != '}')) {
        os << ",";
      }
      if (((in_object_idx[depth] & 1) == 1)) {
        os << ":";
      }
      in_object_idx[depth]++;
    }
    switch (type) {
    case '"': // we have a string
      os << '"';
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf.get() + payload + sizeof(uint32_t)),
          os, string_length);
      os << '"';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << static_cast<int64_t>(tape[++tape_idx]);
      break;
    case 'u':
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << tape[++tape_idx];
      break;
    case 'd': // we have a double
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer;
      break;
    case 'n': // we have a null
      os << "null";
      break;
    case 't': // we have a true
      os << "true";
      break;
    case 'f': // we have a false
      os << "false";
      break;
    case '{': // we have an object
      os << '{';
      depth++;
      in_object[depth] = true;
      in_object_idx[depth] = 0;
      break;
    case '}': // we end an object
      depth--;
      os << '}';
      break;
    case '[': // we start an array
      os << '[';
      depth++;
      in_object[depth] = false;
      in_object_idx[depth] = 0;
      break;
    case ']': // we end an array
      depth--;
      os << ']';
      break;
    case 'r': // we start and end with the root node
      fprintf(stderr, "should we be hitting the root node?\n");
      return false;
    default:
      fprintf(stderr, "bug %c\n", type);
      return false;
    }
  }
  return true;
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) const {
  if (!valid) {
    return false;
  }
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = (tape_val >> 56);
  os << tape_idx << " : " << type;
  tape_idx++;
  size_t how_many = 0;
  if (type == 'r') {
    how_many = tape_val & JSON_VALUE_MASK;
  } else {
    fprintf(stderr, "Error: no starting root node?");
    return false;
  }
  os << "\t// pointing to " << how_many << " (right after last node)\n";
  uint64_t payload;
  for (; tape_idx < how_many; tape_idx++) {
    os << tape_idx << " : ";
    tape_val = tape[tape_idx];
    payload = tape_val & JSON_VALUE_MASK;
    type = (tape_val >> 56);
    switch (type) {
    case '"': // we have a string
      os << "string \"";
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf.get() + payload + sizeof(uint32_t)),
                  os,
          string_length);
      os << '"';
      os << '\n';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
      break;
    case 'u': // we have a long uint
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "unsigned integer " << tape[++tape_idx] << "\n";
      break;
    case 'd': // we have a double
      os << "float ";
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer << '\n';
      break;
    case 'n': // we have a null
      os << "null\n";
      break;
    case 't': // we have a true
      os << "true\n";
      break;
    case 'f': // we have a false
      os << "false\n";
      break;
    case '{': // we have an object
      os << "{\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case 'r': // we start and end with the root node
      printf("end of root\n");
      return false;
    default:
      return false;
    }
  }
  tape_val = tape[tape_idx];
  payload = tape_val & JSON_VALUE_MASK;
  type = (tape_val >> 56);
  os << tape_idx << " : " << type << "\t// pointing to " << payload
     << " (start root)\n";
  return true;
}
} // namespace simdjson
