#ifndef SJQ_ONDEMAND_H
#define SJQ_ONDEMAND_H

#include "ast.h"
#include "writer.h"



namespace sjq {

//////////////////////////////////////////////////////////////////////////////
// On-Demand fast path
//
// Programs made of paths, .[], select() with comparisons against literals,
// object/array construction, literals, length, ',' and '|' are evaluated
// directly on the On-Demand front end, without building a DOM. Output is
// written from the raw input: strings without escapes are copied as is, and
// number literals are printed the way jq 1.7+ prints them (decNumber's
// to-scientific-string of the literal). Anything unexpected while evaluating a
// document (a type error, an invalid document, a construct that would need
// buffering) throws od_fallback; the caller discards the output of that
// document and runs the DOM evaluator on it, which also produces jq's errors.
//////////////////////////////////////////////////////////////////////////////

struct od_fallback {};

struct odh {
  // ROOT: a scalar document, known only by its token text.
  enum kind_t : uint8_t { NUL, OBJ, ARR, SCALAR, ROOT } k{NUL};
  std::string_view tok{};
  ondemand::json_type st{ondemand::json_type::null};  // for SCALAR
  ondemand::object o{};
  ondemand::array a{};
  ondemand::value v{};
};

[[noreturn]] inline void od_fail() { throw od_fallback{}; }

inline void od_make(ondemand::value v, odh &h) {
  ondemand::json_type t{};
  if (v.type().get(t)) { od_fail(); }
  switch (t) {
  case ondemand::json_type::object:
    h.k = odh::OBJ;
    if (v.get_object().get(h.o)) { od_fail(); }
    return;
  case ondemand::json_type::array:
    h.k = odh::ARR;
    if (v.get_array().get(h.a)) { od_fail(); }
    return;
  case ondemand::json_type::null: {
    bool isn = false;
    if (v.is_null().get(isn) || !isn) { od_fail(); }
    h.k = odh::NUL;
    return;
  }
  case ondemand::json_type::unknown: od_fail();
  default:
    h.k = odh::SCALAR;
    h.st = t;
    h.v = v;
    return;
  }
}

inline std::string_view rtrim_ws(std::string_view s) {
  while (!s.empty() && (s.back() == ' ' || s.back() == '\n' || s.back() == '\r' || s.back() == '\t')) { s.remove_suffix(1); }
  return s;
}

// Writes a JSON number literal as jq 1.7+ does. Returns false if the token is
// not a valid JSON number.
inline bool od_write_number(writer &w, std::string_view t) {
  size_t n = t.size(), i = 0;
  bool neg = false;
  if (i < n && t[i] == '-') { neg = true; i++; }
  size_t int_start = i;
  if (i >= n) { return false; }
  if (t[i] == '0') { i++; }
  else if (t[i] >= '1' && t[i] <= '9') { while (i < n && t[i] >= '0' && t[i] <= '9') { i++; } }
  else { return false; }
  size_t int_end = i, frac_start = i, frac_end = i;
  if (i < n && t[i] == '.') {
    i++;
    frac_start = i;
    while (i < n && t[i] >= '0' && t[i] <= '9') { i++; }
    if (i == frac_start) { return false; }
    frac_end = i;
  }
  bool has_exp = false;
  int64_t exp = 0;
  if (i < n && (t[i] == 'e' || t[i] == 'E')) {
    has_exp = true;
    i++;
    bool eneg = false;
    if (i < n && (t[i] == '+' || t[i] == '-')) { eneg = t[i] == '-'; i++; }
    size_t es = i;
    while (i < n && t[i] >= '0' && t[i] <= '9') {
      if (exp < 1000000000) { exp = exp * 10 + (t[i] - '0'); }
      i++;
    }
    if (i == es) { return false; }
    if (eneg) { exp = -exp; }
  }
  if (i != n) { return false; }
  // Common case: no exponent and a non-zero integer part, or a plain integer.
  if (!has_exp && (t[int_start] != '0' || frac_end == frac_start)) { w.put(t); return true; }
  // General case: coefficient digits and exponent, as decNumber stores them.
  std::string digits(t.substr(int_start, int_end - int_start));
  digits.append(t.substr(frac_start, frac_end - frac_start));
  int64_t e = exp - int64_t(frac_end - frac_start);
  size_t lead = digits.find_first_not_of('0');
  digits = lead == std::string::npos ? std::string("0") : digits.substr(lead);
  int64_t nd = int64_t(digits.size());
  int64_t adjusted = e + nd - 1;
  if (neg) { w.put('-'); }
  if (e <= 0 && adjusted >= -6) {
    if (e == 0) { w.put(digits); return true; }
    int64_t pos = nd + e;
    if (pos > 0) {
      w.put(std::string_view(digits).substr(0, size_t(pos)));
      w.put('.');
      w.put(std::string_view(digits).substr(size_t(pos)));
    } else {
      w.put("0.");
      w.buf.append(size_t(-pos), '0');
      w.put(digits);
    }
    return true;
  }
  w.put(digits[0]);
  if (nd > 1) { w.put('.'); w.put(std::string_view(digits).substr(1)); }
  w.put('E');
  w.put(adjusted >= 0 ? '+' : '-');
  w.put(std::to_string(adjusted >= 0 ? adjusted : -adjusted));
  return true;
}

// True when a raw string body can be copied verbatim (no escapes, no control
// characters, no DEL).
inline bool od_clean_string(std::string_view s) {
  const char *p = s.data(), *end = p + s.size();
#if defined(__ARM_NEON)
  while (end - p >= 16) {
    uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
    uint8x16_t m = vorrq_u8(vorrq_u8(vcltq_u8(v, vdupq_n_u8(0x20)), vceqq_u8(v, vdupq_n_u8('\\'))), vceqq_u8(v, vdupq_n_u8(0x7f)));
    if (vmaxvq_u8(m)) { return false; }
    p += 16;
  }
#elif defined(SJQ_SSE2)
  while (end - p >= 16) {
    __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
    __m128i m = _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(_mm_min_epu8(v, _mm_set1_epi8(0x1f)), v), _mm_cmpeq_epi8(v, _mm_set1_epi8('\\'))),
                             _mm_cmpeq_epi8(v, _mm_set1_epi8(0x7f)));
    if (_mm_movemask_epi8(m)) { return false; }
    p += 16;
  }
#endif
  for (; p < end; p++) {
    uint8_t c = uint8_t(*p);
    if (c < 0x20 || c == '\\' || c == 0x7f) { return false; }
  }
  return true;
}

struct od_writer {
  writer &w;
  bool raw_top{};

  void key(ondemand::field &f) {
    std::string_view k = f.escaped_key();
    if (od_clean_string(k)) {
      w.cbegin(C_KEY); w.put('"'); w.put(k); w.put('"'); w.cend();
    } else {
      std::string_view u;
      if (f.unescaped_key().get(u)) { od_fail(); }
      w.key(u);
    }
  }

  void scalar(ondemand::value &v, ondemand::json_type t, bool raw) {
    switch (t) {
    case ondemand::json_type::string: {
      std::string_view tok = rtrim_ws(v.raw_json_token());
      if (tok.size() < 2 || tok.back() != '"') { od_fail(); }
      std::string_view body = tok.substr(1, tok.size() - 2);
      if (od_clean_string(body)) {
        if (raw) { w.put(body); } else { w.cbegin(C_STR); w.put(tok); w.cend(); }
        return;
      }
      std::string_view u;
      if (v.get_string().get(u)) { od_fail(); }
      if (raw) { w.put(u); } else { w.string_value(u); }
      return;
    }
    case ondemand::json_type::number:
      w.cbegin(C_NUM);
      if (!od_write_number(w, rtrim_ws(v.raw_json_token()))) { od_fail(); }
      w.cend();
      return;
    case ondemand::json_type::boolean: {
      bool b;
      if (v.get_bool().get(b)) { od_fail(); }
      if (b) { w.atom(C_TRUE, "true"); } else { w.atom(C_FALSE, "false"); }
      return;
    }
    case ondemand::json_type::null: {
      bool isn = false;
      if (v.is_null().get(isn) || !isn) { od_fail(); }
      w.atom(C_NULL, "null");
      return;
    }
    default: od_fail();
    }
  }

  void object(ondemand::object &o, int depth) {
    bool first = true;
    for (auto fr : o) {
      ondemand::field f;
      if (std::move(fr).get(f)) { od_fail(); }
      if (first) { w.open(C_OBJ, '{'); } else { w.comma(C_OBJ); }
      first = false;
      if (w.pretty) { w.newline_indent(depth + 1); }
      key(f);
      w.colon();
      value(f.value(), depth + 1);
    }
    if (first) { w.empty(C_OBJ, "{}"); return; }
    if (w.pretty) { w.newline_indent(depth); }
    w.open(C_OBJ, '}');
  }

  void array(ondemand::array &a, int depth) {
    bool first = true;
    for (auto er : a) {
      ondemand::value v;
      if (std::move(er).get(v)) { od_fail(); }
      if (first) { w.open(C_ARR, '['); } else { w.comma(C_ARR); }
      first = false;
      if (w.pretty) { w.newline_indent(depth + 1); }
      value(v, depth + 1);
    }
    if (first) { w.empty(C_ARR, "[]"); return; }
    if (w.pretty) { w.newline_indent(depth); }
    w.open(C_ARR, ']');
  }

  void value(ondemand::value v, int depth) {
    check_stack();
    ondemand::json_type t;
    if (v.type().get(t)) { od_fail(); }
    if (t == ondemand::json_type::object) {
      ondemand::object o;
      if (v.get_object().get(o)) { od_fail(); }
      object(o, depth);
    } else if (t == ondemand::json_type::array) {
      ondemand::array a;
      if (v.get_array().get(a)) { od_fail(); }
      array(a, depth);
    } else {
      scalar(v, t, false);
    }
  }

  void handle(odh &h, int depth, bool raw) {
    switch (h.k) {
    case odh::NUL: w.atom(C_NULL, "null"); return;
    case odh::OBJ: if (h.o.reset().error()) { od_fail(); } object(h.o, depth); return;
    case odh::ARR: if (h.a.reset().error()) { od_fail(); } array(h.a, depth); return;
    case odh::SCALAR: scalar(h.v, h.st, raw); return;
    case odh::ROOT:
      if (h.st == ondemand::json_type::number) {
        w.cbegin(C_NUM);
        if (!od_write_number(w, h.tok)) { od_fail(); }
        w.cend();
      } else if (h.st == ondemand::json_type::string) {
        if (h.tok.size() < 2 || h.tok.back() != '"') { od_fail(); }
        std::string_view body = h.tok.substr(1, h.tok.size() - 2);
        if (!od_clean_string(body)) { od_fail(); }
        if (raw) { w.put(body); } else { w.cbegin(C_STR); w.put(h.tok); w.cend(); }
      } else if (h.tok == "true" || h.tok == "false") {
        w.atom(h.tok == "true" ? C_TRUE : C_FALSE, h.tok);
      } else {
        od_fail();
      }
      return;
    }
  }
};

// Where an emitted output goes: begin() is called before an output is
// written, end() after.
struct od_target {
  virtual ~od_target() = default;
  virtual void begin() = 0;
  virtual void end() = 0;
  int depth{0};
  bool raw{false};
};

template <class A> class cb_ref {
  void *obj{};
  void (*call)(void *, A &);
public:
  template <class F>
  cb_ref(F &&f) : obj((void *)&f), call([](void *o, A &a) { (*(std::remove_reference_t<F> *)o)(a); }) {}
  void operator()(A &a) const { call(obj, a); }
};
using od_cb = cb_ref<odh>;

struct od_expr {
  bool is_nav{false};
  virtual ~od_expr() = default;
  // Navigators yield handles into the document.
  virtual void nav(odh &, od_cb) { od_fail(); }
  // Writes each output; navigators write the handles they yield.
  virtual void emit(odh &in, od_writer &ow, od_target &t) {
    nav(in, [&](odh &x) { t.begin(); ow.handle(x, t.depth, t.raw); t.end(); });
  }
  virtual void set_opt() {}
};
using od_ptr = std::unique_ptr<od_expr>;

struct od_identity : od_expr {
  od_identity() { is_nav = true; }
  void nav(odh &in, od_cb out) override { out(in); }
};

struct od_empty : od_expr {
  od_empty() { is_nav = true; }
  void nav(odh &, od_cb) override {}
};

struct od_step : od_expr {
  enum kind_t { FIELD, INDEX, ITER } kind{FIELD};
  std::string name{};
  size_t idx{0};
  bool opt{false};
  od_ptr target{};
  od_step() { is_nav = true; }
  void set_opt() override { opt = true; if (target) { target->set_opt(); } }
  void type_error() { if (!opt) { od_fail(); } }
  void apply(odh &in, od_cb out) {
    odh child;
    switch (kind) {
    case FIELD:
      if (in.k == odh::OBJ) {
        ondemand::value v;
        auto err = in.o.find_field_unordered(name).get(v);
        if (err == NO_SUCH_FIELD) { out(child); return; }
        if (err) { od_fail(); }
        od_make(v, child);
        out(child);
      } else if (in.k == odh::NUL) {
        out(child);
      } else {
        type_error();
      }
      return;
    case INDEX:
      if (in.k == odh::ARR) {
        if (in.a.reset().error()) { od_fail(); }
        ondemand::value v;
        auto err = in.a.at(idx).get(v);
        if (err == INDEX_OUT_OF_BOUNDS) { out(child); return; }
        if (err) { od_fail(); }
        od_make(v, child);
        out(child);
      } else if (in.k == odh::NUL) {
        out(child);
      } else {
        type_error();
      }
      return;
    case ITER:
      if (in.k == odh::ARR) {
        if (in.a.reset().error()) { od_fail(); }
        for (auto er : in.a) {
          ondemand::value v;
          if (std::move(er).get(v)) { od_fail(); }
          od_make(v, child);
          out(child);
        }
      } else if (in.k == odh::OBJ) {
        if (in.o.reset().error()) { od_fail(); }
        for (auto fr : in.o) {
          ondemand::field f;
          if (std::move(fr).get(f)) { od_fail(); }
          od_make(f.value(), child);
          out(child);
        }
      } else {
        type_error();
      }
      return;
    }
  }
  void nav(odh &in, od_cb out) override {
    if (target) { target->nav(in, [&](odh &x) { apply(x, out); }); }
    else { apply(in, out); }
  }
};

struct od_pipe : od_expr {
  od_ptr a{}, b{};
  void set_opt() override { a->set_opt(); b->set_opt(); }
  void nav(odh &in, od_cb out) override { a->nav(in, [&](odh &x) { b->nav(x, out); }); }
  void emit(odh &in, od_writer &ow, od_target &t) override { a->nav(in, [&](odh &x) { b->emit(x, ow, t); }); }
};

struct od_comma : od_expr {
  od_ptr a{}, b{};
  void set_opt() override { a->set_opt(); b->set_opt(); }
  void nav(odh &in, od_cb out) override { a->nav(in, out); b->nav(in, out); }
  void emit(odh &in, od_writer &ow, od_target &t) override { a->emit(in, ow, t); b->emit(in, ow, t); }
};

// Conditions for select().
struct od_cond {
  virtual ~od_cond() = default;
  virtual bool test(odh &in) = 0;
};
using od_cond_ptr = std::unique_ptr<od_cond>;

inline K od_kind(odh &h) {
  switch (h.k) {
  case odh::NUL: return K::Null;
  case odh::OBJ: return K::Obj;
  case odh::ARR: return K::Arr;
  case odh::ROOT: od_fail();
  case odh::SCALAR:
    if (h.st == ondemand::json_type::string) { return K::Str; }
    if (h.st == ondemand::json_type::number) { return K::Num; }
    {
      bool b;
      if (h.v.get_bool().get(b)) { od_fail(); }
      return b ? K::True : K::False;
    }
  }
  return K::Null;
}

// Compares a document value with a scalar literal, with jq's ordering.
inline int od_compare(odh &h, const value &lit) {
  K k = od_kind(h);
  int ra = kind_rank(k), rb = kind_rank(lit.k);
  if (ra != rb) { return ra < rb ? -1 : 1; }
  if (k == K::Num) {
    double x;
    if (h.v.get_double().get(x)) { od_fail(); }
    double y = get_num(lit);
    return x < y ? -1 : (x > y ? 1 : 0);
  }
  if (k == K::Str) {
    std::string_view x;
    if (h.v.get_string().get(x)) { od_fail(); }
    int c = x.compare(get_str(lit));
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
  }
  return 0;  // null, false, true
}

// Runs a navigator that must yield at most one value; f is called on it.
// Returns false when it yields nothing.
template <class F> inline bool od_single(od_expr &e, odh &in, F &&f) {
  int count = 0;
  e.nav(in, [&](odh &x) {
    if (++count > 1) { od_fail(); }
    f(x);
  });
  return count == 1;
}

struct od_cmp : od_cond {
  od_ptr path{};
  op o{};
  value lit{};
  bool test(odh &in) override {
    bool r = false;
    // A path that yields nothing makes select() yield nothing.
    od_single(*path, in, [&](odh &x) {
      int c = od_compare(x, lit);
      switch (o) {
      case op::eq: r = c == 0; break;
      case op::ne: r = c != 0; break;
      case op::lt: r = c < 0; break;
      case op::le: r = c <= 0; break;
      case op::gt: r = c > 0; break;
      case op::ge: r = c >= 0; break;
      default: od_fail();
      }
    });
    return r;
  }
};

struct od_truthy : od_cond {
  od_ptr path{};
  bool test(odh &in) override {
    bool r = false;
    od_single(*path, in, [&](odh &x) {
      K k = od_kind(x);
      r = !(k == K::Null || k == K::False);
    });
    return r;
  }
};

struct od_has : od_cond {
  std::string key{};
  bool test(odh &in) override {
    if (in.k != odh::OBJ) { od_fail(); }
    ondemand::value v;
    auto err = in.o.find_field_unordered(key).get(v);
    if (err == NO_SUCH_FIELD) { return false; }
    if (err) { od_fail(); }
    return true;
  }
};

struct od_logic : od_cond {
  op o{};  // and_, or_
  od_cond_ptr a{}, b{};
  bool test(odh &in) override {
    bool x = a->test(in);
    if (o == op::and_) { return x && b->test(in); }
    return x || b->test(in);
  }
};

struct od_not : od_cond {
  od_cond_ptr a{};
  bool test(odh &in) override { return !a->test(in); }
};

struct od_const : od_cond {
  bool v{};
  bool test(odh &) override { return v; }
};

struct od_select : od_expr {
  od_cond_ptr cond{};
  od_select() { is_nav = true; }
  void nav(odh &in, od_cb out) override { if (cond->test(in)) { out(in); } }
};

struct od_literal : od_expr {
  value v{};
  std::string text{};
  void emit(odh &, od_writer &ow, od_target &t) override {
    t.begin();
    if (t.raw && v.k == K::Str) { ow.w.put(get_str(v)); }
    else if (!text.empty()) {
      ow.w.cbegin(C_NUM);
      if (!od_write_number(ow.w, text)) { ow.w.write_double(get_num(v)); }
      ow.w.cend();
    }
    else { ow.w.write_value(v, t.depth); }
    t.end();
  }
};

struct od_length : od_expr {
  void emit(odh &in, od_writer &ow, od_target &t) override {
    double n = 0;
    switch (in.k) {
    case odh::NUL: n = 0; break;
    case odh::OBJ: {
      size_t c;
      if (in.o.reset().error() || in.o.count_fields().get(c)) { od_fail(); }
      n = double(c);
      break;
    }
    case odh::ARR: {
      size_t c;
      if (in.a.reset().error() || in.a.count_elements().get(c)) { od_fail(); }
      n = double(c);
      break;
    }
    case odh::SCALAR:
      if (in.st == ondemand::json_type::string) {
        std::string_view s;
        if (in.v.get_string().get(s)) { od_fail(); }
        size_t c = 0;
        for (unsigned char ch : s) { c += (ch & 0xC0) != 0x80; }
        n = double(c);
      } else if (in.st == ondemand::json_type::number) {
        double d;
        if (in.v.get_double().get(d)) { od_fail(); }
        n = std::fabs(d);
      } else {
        od_fail();  // booleans have no length: let the DOM path report it
      }
      break;
    case odh::ROOT: od_fail();
    }
    t.begin();
    ow.w.number_value(n);
    t.end();
  }
};

struct od_object : od_expr {
  std::vector<std::pair<std::string, od_ptr>> entries{};
  void emit(odh &in, od_writer &ow, od_target &t) override {
    t.begin();
    writer &w = ow.w;
    if (entries.empty()) { w.empty(C_OBJ, "{}"); t.end(); return; }
    struct one : od_target {
      int count{0};
      void begin() override { if (++count > 1) { od_fail(); } }
      void end() override {}
    };
    bool first = true;
    for (auto &e : entries) {
      if (first) { w.open(C_OBJ, '{'); } else { w.comma(C_OBJ); }
      first = false;
      if (w.pretty) { w.newline_indent(t.depth + 1); }
      w.key(e.first);
      w.colon();
      one target;
      target.depth = t.depth + 1;
      e.second->emit(in, ow, target);
      if (target.count != 1) { od_fail(); }
    }
    if (w.pretty) { w.newline_indent(t.depth); }
    w.open(C_OBJ, '}');
    t.end();
  }
};

struct od_array_cons : od_expr {
  od_ptr body{};
  void emit(odh &in, od_writer &ow, od_target &t) override {
    t.begin();
    writer &w = ow.w;
    struct elems : od_target {
      writer *w{nullptr};
      int count{0};
      void begin() override {
        if (count++) { w->comma(C_ARR); } else { w->open(C_ARR, '['); }
        if (w->pretty) { w->newline_indent(depth); }
      }
      void end() override {}
    };
    elems target;
    target.w = &w;
    target.depth = t.depth + 1;
    if (body) { body->emit(in, ow, target); }
    if (target.count == 0) { w.empty(C_ARR, "[]"); }
    else {
      if (w.pretty) { w.newline_indent(t.depth); }
      w.open(C_ARR, ']');
    }
    t.end();
  }
};

// Compiles the AST into an On-Demand plan; returns null if unsupported.
od_ptr od_compile(const node *n);

struct od_top : od_target {
  writer *w{};
  bool join{};
  bool seq{};  // --seq: RS before each output
  void begin() override { if (seq) { w->put('\x1e'); } }
  void end() override { if (!join) { w->put('\n'); } }
};

// Evaluates the plan on one document. Returns false, with the document's
// output discarded, when the DOM path must handle it.
template <class Doc>
inline bool od_run_doc(od_expr &plan, Doc &doc, writer &w, bool raw, bool join, bool seq) {
  size_t mark = w.buf.size();
  try {
    odh root;
    ondemand::json_type t;
    if (doc.type().get(t)) { od_fail(); }
    if (t == ondemand::json_type::object) {
      root.k = odh::OBJ;
      if (doc.get_object().get(root.o)) { od_fail(); }
    } else if (t == ondemand::json_type::array) {
      root.k = odh::ARR;
      if (doc.get_array().get(root.a)) { od_fail(); }
    } else if (t == ondemand::json_type::null) {
      bool isn = false;
      if (doc.is_null().get(isn) || !isn) { od_fail(); }
      root.k = odh::NUL;
    } else {
      std::string_view tok;
      if (doc.raw_json_token().get(tok)) { od_fail(); }
      root.k = odh::ROOT;
      root.st = t;
      root.tok = rtrim_ws(tok);
    }
    od_writer ow{w, raw};
    od_top top;
    top.w = &w;
    top.join = join;
    top.seq = seq;
    top.raw = raw;
    plan.emit(root, ow, top);
    return true;
  } catch (od_fallback &) {
    w.buf.resize(mark);
    return false;
  } catch (jq_error &) {
    // The DOM evaluator runs the document again and reports the error.
    w.buf.resize(mark);
    return false;
  }
}

} // namespace sjq

#endif // SJQ_ONDEMAND_H
