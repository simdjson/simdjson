#include "simdjson/parsedjson.h"

namespace simdjson {
ParsedJson::ParsedJson()
    : structural_indexes(nullptr), tape(nullptr),
      containing_scope_offset(nullptr), ret_address(nullptr),
      string_buf(nullptr), current_string_buf_loc(nullptr) {}

ParsedJson::~ParsedJson() { deallocate(); }

ParsedJson::ParsedJson(ParsedJson &&p)
    : byte_capacity(p.byte_capacity), depth_capacity(p.depth_capacity),
      tape_capacity(p.tape_capacity), string_capacity(p.string_capacity),
      current_loc(p.current_loc), n_structural_indexes(p.n_structural_indexes),
      structural_indexes(p.structural_indexes), tape(p.tape),
      containing_scope_offset(p.containing_scope_offset),
      ret_address(p.ret_address), string_buf(p.string_buf),
      current_string_buf_loc(p.current_string_buf_loc), valid(p.valid) {
  p.structural_indexes = nullptr;
  p.tape = nullptr;
  p.containing_scope_offset = nullptr;
  p.ret_address = nullptr;
  p.string_buf = nullptr;
  p.current_string_buf_loc = nullptr;
}

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
  if ((len <= byte_capacity) && (depth_capacity < max_depth)) {
    return true;
  }
  deallocate();
  valid = false;
  byte_capacity = 0; // will only set it to len after allocations are a success
  n_structural_indexes = 0;
  uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
  structural_indexes = new (std::nothrow) uint32_t[max_structures];
  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of len + 1
  size_t local_tape_capacity = ROUNDUP_N(len + 1, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t local_string_capacity = ROUNDUP_N(5 * len / 3 + 32, 64);
  string_buf = new (std::nothrow) uint8_t[local_string_capacity];
  tape = new (std::nothrow) uint64_t[local_tape_capacity];
  containing_scope_offset = new (std::nothrow) uint32_t[max_depth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  ret_address = new (std::nothrow) void *[max_depth];
#else
  ret_address = new (std::nothrow) char[max_depth];
#endif
  if ((string_buf == nullptr) || (tape == nullptr) ||
      (containing_scope_offset == nullptr) || (ret_address == nullptr) ||
      (structural_indexes == nullptr)) {
    std::cerr << "Could not allocate memory" << std::endl;
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;

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
  delete[] ret_address;
  delete[] containing_scope_offset;
  delete[] tape;
  delete[] string_buf;
  delete[] structural_indexes;
  valid = false;
}

void ParsedJson::init() {
  current_string_buf_loc = string_buf;
  current_loc = 0;
  valid = false;
}

WARN_UNUSED
bool ParsedJson::print_json(std::ostream &os) {
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
  bool *in_object = new bool[depth_capacity];
  auto *in_object_idx = new size_t[depth_capacity];
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
      memcpy(&string_length, string_buf + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf + payload + sizeof(uint32_t)),
          string_length);
      os << '"';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        delete[] in_object;
        delete[] in_object_idx;
        return false;
      }
      os << static_cast<int64_t>(tape[++tape_idx]);
      break;
    case 'd': // we have a double
      if (tape_idx + 1 >= how_many) {
        delete[] in_object;
        delete[] in_object_idx;
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
      delete[] in_object;
      delete[] in_object_idx;
      return false;
    default:
      fprintf(stderr, "bug %c\n", type);
      delete[] in_object;
      delete[] in_object_idx;
      return false;
    }
  }
  delete[] in_object;
  delete[] in_object_idx;
  return true;
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) {
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
      memcpy(&string_length, string_buf + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf + payload + sizeof(uint32_t)),
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
