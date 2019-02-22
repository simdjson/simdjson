#include "simdjson/parsedjson.h"

ParsedJson::ParsedJson() : bytecapacity(0), depthcapacity(0), tapecapacity(0), stringcapacity(0),
        current_loc(0), n_structural_indexes(0),
        structural_indexes(NULL), tape(NULL), containing_scope_offset(NULL),
        ret_address(NULL), string_buf(NULL), current_string_buf_loc(NULL), isvalid(false) {}

ParsedJson::~ParsedJson() {
    deallocate();
}

ParsedJson::ParsedJson(ParsedJson && p)
    : bytecapacity(std::move(p.bytecapacity)),
    depthcapacity(std::move(p.depthcapacity)),
    tapecapacity(std::move(p.tapecapacity)),
    stringcapacity(std::move(p.stringcapacity)),
    current_loc(std::move(p.current_loc)),
    n_structural_indexes(std::move(p.n_structural_indexes)),
    structural_indexes(std::move(p.structural_indexes)),
    tape(std::move(p.tape)),
    containing_scope_offset(std::move(p.containing_scope_offset)),
    ret_address(std::move(p.ret_address)),
    string_buf(std::move(p.string_buf)),
    current_string_buf_loc(std::move(p.current_string_buf_loc)),
    isvalid(std::move(p.isvalid)) {
        p.structural_indexes=NULL;
        p.tape=NULL;
        p.containing_scope_offset=NULL;
        p.ret_address=NULL;
        p.string_buf=NULL;
        p.current_string_buf_loc=NULL;
    }



WARN_UNUSED
bool ParsedJson::allocateCapacity(size_t len, size_t maxdepth) {
    if ((maxdepth == 0) || (len == 0)) {
      std::cerr << "capacities must be non-zero " << std::endl;
      return false;
    }
    if (len > 0) {
      if ((len <= bytecapacity) && (depthcapacity < maxdepth))
        return true;
      deallocate();
    }
    isvalid = false;
    bytecapacity = 0; // will only set it to len after allocations are a success
    n_structural_indexes = 0;
    uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new uint32_t[max_structures];
    size_t localtapecapacity = ROUNDUP_N(len, 64);
    size_t localstringcapacity = ROUNDUP_N(len + 32, 64);
    string_buf = new uint8_t[localstringcapacity];
    tape = new uint64_t[localtapecapacity];
    containing_scope_offset = new uint32_t[maxdepth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
    ret_address = new void *[maxdepth];
#else
    ret_address = new char[maxdepth];
#endif
    if ((string_buf == NULL) || (tape == NULL) ||
        (containing_scope_offset == NULL) || (ret_address == NULL) || (structural_indexes == NULL)) {
      std::cerr << "Could not allocate memory" << std::endl;
      if(ret_address != NULL) delete[] ret_address;
      if(containing_scope_offset != NULL) delete[] containing_scope_offset;
      if(tape != NULL) delete[] tape;
      if(string_buf != NULL) delete[] string_buf;
      if(structural_indexes != NULL) delete[] structural_indexes;
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
    if(ret_address != NULL) delete[] ret_address;
    if(containing_scope_offset != NULL) delete[] containing_scope_offset;
    if(tape != NULL) delete[] tape;
    if(string_buf != NULL) delete[] string_buf;
    if(structural_indexes != NULL) delete[] structural_indexes;
    isvalid = false;
}

void ParsedJson::init() {
    current_string_buf_loc = string_buf;
    current_loc = 0;
    isvalid = false;
}

WARN_UNUSED
bool ParsedJson::printjson(std::ostream &os) {
    if(!isvalid) return false;
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
    size_t *inobjectidx = new size_t[depthcapacity];
    int depth = 1; // only root at level 0
    inobjectidx[depth] = 0;
    inobject[depth] = false;
    for (; tapeidx < howmany; tapeidx++) {
      tape_val = tape[tapeidx];
      uint64_t payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      if (!inobject[depth]) {
        if ((inobjectidx[depth] > 0) && (type != ']'))
          os << ",";
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}'))
          os << ",";
        if (((inobjectidx[depth] & 1) == 1))
          os << ":";
        inobjectidx[depth]++;
      }
      switch (type) {
      case '"': // we have a string
        os << '"';
        print_with_escapes((const unsigned char *)(string_buf + payload));
        os << '"';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany)
          return false;
        os <<  (int64_t)tape[++tapeidx];
        break;
      case 'd': // we have a double
        if (tapeidx + 1 >= howmany)
          return false;
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
    if(!isvalid) return false;
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
        if (tapeidx + 1 >= howmany)
          return false;
        os << "integer " << (int64_t)tape[++tapeidx] << "\n";
        break;
      case 'd': // we have a double
        os << "float ";
        if (tapeidx + 1 >= howmany)
          return false;
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
