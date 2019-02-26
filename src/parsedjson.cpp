#include "simdjson/parsedjson.h"

ParsedJson::ParsedJson() : 
        structural_indexes(nullptr), tape(nullptr), containing_scope_offset(nullptr),
        ret_address(nullptr), string_buf(nullptr), current_string_buf_loc(nullptr) {}

ParsedJson::~ParsedJson() {
    deallocate();
}

ParsedJson::ParsedJson(ParsedJson && p)
    : bytecapacity(p.bytecapacity),
    depthcapacity(p.depthcapacity),
    tapecapacity(p.tapecapacity),
    stringcapacity(p.stringcapacity),
    current_loc(p.current_loc),
    n_structural_indexes(p.n_structural_indexes),
    structural_indexes(p.structural_indexes),
    tape(p.tape),
    containing_scope_offset(p.containing_scope_offset),
    ret_address(p.ret_address),
    string_buf(p.string_buf),
    current_string_buf_loc(p.current_string_buf_loc),
    isvalid(p.isvalid) {
        p.structural_indexes=nullptr;
        p.tape=nullptr;
        p.containing_scope_offset=nullptr;
        p.ret_address=nullptr;
        p.string_buf=nullptr;
        p.current_string_buf_loc=nullptr;
    }



WARN_UNUSED
bool ParsedJson::allocateCapacity(size_t len, size_t maxdepth) {
    if ((maxdepth == 0) || (len == 0)) {
      std::cerr << "capacities must be non-zero " << std::endl;
      return false;
    }
    if ((len <= bytecapacity) && (depthcapacity < maxdepth))
      return true;
    deallocate();
    isvalid = false;
    bytecapacity = 0; // will only set it to len after allocations are a success
    n_structural_indexes = 0;
    uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new (std::nothrow) uint32_t[max_structures];
    size_t localtapecapacity = ROUNDUP_N(len, 64);
    size_t localstringcapacity = ROUNDUP_N(len + 32, 64);
    string_buf = new (std::nothrow) uint8_t[localstringcapacity];
    tape = new (std::nothrow) uint64_t[localtapecapacity];
    containing_scope_offset = new (std::nothrow) uint32_t[maxdepth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
    ret_address = new (std::nothrow) void *[maxdepth];
#else
    ret_address = new (std::nothrow) char[maxdepth];
#endif
    if ((string_buf == nullptr) || (tape == nullptr) ||
        (containing_scope_offset == nullptr) || (ret_address == nullptr) || (structural_indexes == nullptr)) {
      std::cerr << "Could not allocate memory" << std::endl;
      delete[] ret_address;
      delete[] containing_scope_offset;
      delete[] tape;
      delete[] string_buf;
      delete[] structural_indexes;

      return false;
    }

    bytecapacity = len;
    depthcapacity = maxdepth;
    tapecapacity = localtapecapacity;
    stringcapacity = localstringcapacity;
    return true;
}

bool ParsedJson::isValid() const {
    return isvalid;
}

void ParsedJson::deallocate() {
    bytecapacity = 0;
    depthcapacity = 0;
    tapecapacity = 0;
    stringcapacity = 0;
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;
    isvalid = false;
}

void ParsedJson::init() {
    current_string_buf_loc = string_buf;
    current_loc = 0;
    isvalid = false;
}

WARN_UNUSED
bool ParsedJson::printjson(std::ostream &os) {
    if(!isvalid) { 
      return false;
    }
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    if (howmany > tapecapacity) {
      fprintf(stderr,
          "We may be exceeding the tape capacity. Is this a valid document?\n");
      return false;
    }
    tapeidx++;
    bool *inobject = new bool[depthcapacity];
    auto *inobjectidx = new size_t[depthcapacity];
    int depth = 1; // only root at level 0
    inobjectidx[depth] = 0;
    inobject[depth] = false;
    for (; tapeidx < howmany; tapeidx++) {
      tape_val = tape[tapeidx];
      uint64_t payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      if (!inobject[depth]) {
        if ((inobjectidx[depth] > 0) && (type != ']')) {
          os << ",";
        }
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}')) {
          os << ",";
        }
        if (((inobjectidx[depth] & 1) == 1)) {
          os << ":";
        }
        inobjectidx[depth]++;
      }
      switch (type) {
      case '"': // we have a string
        os << '"';
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany) {
          delete[] inobject;
          delete[] inobjectidx;
          return false;
        }
        os <<  static_cast<int64_t>(tape[++tapeidx]);
        break;
      case 'd': // we have a double
        if (tapeidx + 1 >= howmany){
          delete[] inobject;
          delete[] inobjectidx;
          return false;
        }
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
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
        inobject[depth] = true;
        inobjectidx[depth] = 0;
        break;
      case '}': // we end an object
        depth--;
        os << '}';
        break;
      case '[': // we start an array
        os << '[';
        depth++;
        inobject[depth] = false;
        inobjectidx[depth] = 0;
        break;
      case ']': // we end an array
        depth--;
        os << ']';
        break;
      case 'r': // we start and end with the root node
        fprintf(stderr, "should we be hitting the root node?\n");
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      default:
        fprintf(stderr, "bug %c\n", type);
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      }
    }
    delete[] inobject;
    delete[] inobjectidx;
    return true;
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) {
    if(!isvalid) { return false;
}
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    os << tapeidx << " : " << type;
    tapeidx++;
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    os << "\t// pointing to " << howmany <<" (right after last node)\n";
    uint64_t payload;
    for (; tapeidx < howmany; tapeidx++) {
      os << tapeidx << " : ";
      tape_val = tape[tapeidx];
      payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      switch (type) {
      case '"': // we have a string
        os << "string \"";
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
        os << '\n';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany) {
          return false;
        }
        os << "integer " << static_cast<int64_t>(tape[++tapeidx]) << "\n";
        break;
      case 'd': // we have a double
        os << "float ";
        if (tapeidx + 1 >= howmany) {
          return false;
        }
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
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
        os << "{\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case '}': // we end an object
        os << "}\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case '[': // we start an array
        os << "[\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case ']': // we end an array
        os << "]\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case 'r': // we start and end with the root node
        printf("end of root\n");
        return false;
      default:
        return false;
      }
    }
    tape_val = tape[tapeidx];
    payload = tape_val & JSONVALUEMASK;
    type = (tape_val >> 56);
    os << tapeidx << " : "<< type <<"\t// pointing to " << payload <<" (start root)\n";
    return true;
}
