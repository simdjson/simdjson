#include "ast.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace sjq {

enum class T { End, Dot, DotDot, Field, Ident, Var, Num, Str, Punct, Keyword, Format };

struct token {
  T t;
  std::string s{};
  double num{0};
};

struct lexer {
  std::string src{};
  size_t i{0};
  std::vector<token> toks{};

  [[noreturn]] void fail(const std::string &m) { throw std::runtime_error("syntax error: " + m); }

  static bool ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
  static bool ident_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

  static int hex_digit(char c) {
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
  }
  // The four hex digits of a \u escape at src[at].
  unsigned hex4(size_t at) {
    if (at + 4 > src.size()) { fail("invalid \\uXXXX escape"); }
    unsigned cp = 0;
    for (size_t k = at; k < at + 4; k++) {
      int d = hex_digit(src[k]);
      if (d < 0) { fail("invalid \\uXXXX escape"); }
      cp = cp * 16 + unsigned(d);
    }
    return cp;
  }

  std::string read_string() {
    // i points after the opening quote.
    std::string r;
    while (i < src.size() && src[i] != '"') {
      char c = src[i++];
      if (c == '\\') {
        if (i >= src.size()) { fail("unterminated string"); }
        char d = src[i++];
        switch (d) {
        case 'n': r.push_back('\n'); break;
        case 't': r.push_back('\t'); break;
        case 'r': r.push_back('\r'); break;
        case 'b': r.push_back('\b'); break;
        case 'f': r.push_back('\f'); break;
        case '/': r.push_back('/'); break;
        case '\\': r.push_back('\\'); break;
        case '"': r.push_back('"'); break;
        case 'u': {
          unsigned cp = hex4(i);
          i += 4;
          if (cp >= 0xDC00 && cp < 0xE000) { fail("invalid \\uXXXX\\uXXXX surrogate pair escape"); }
          if (cp >= 0xD800 && cp < 0xDC00) {
            if (i + 2 > src.size() || src[i] != '\\' || src[i + 1] != 'u') { fail("invalid \\uXXXX\\uXXXX surrogate pair escape"); }
            unsigned lo = hex4(i + 2);
            if (lo < 0xDC00 || lo >= 0xE000) { fail("invalid \\uXXXX\\uXXXX surrogate pair escape"); }
            i += 6;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          }
          if (cp < 0x80) { r.push_back(char(cp)); }
          else if (cp < 0x800) { r.push_back(char(0xC0 | (cp >> 6))); r.push_back(char(0x80 | (cp & 0x3F))); }
          else if (cp < 0x10000) { r.push_back(char(0xE0 | (cp >> 12))); r.push_back(char(0x80 | ((cp >> 6) & 0x3F))); r.push_back(char(0x80 | (cp & 0x3F))); }
          else { r.push_back(char(0xF0 | (cp >> 18))); r.push_back(char(0x80 | ((cp >> 12) & 0x3F))); r.push_back(char(0x80 | ((cp >> 6) & 0x3F))); r.push_back(char(0x80 | (cp & 0x3F))); }
          break;
        }
        case '(': fail("string interpolation is not supported");
        default: fail(std::string("bad escape \\") + d);
        }
      } else {
        r.push_back(c);
      }
    }
    if (i >= src.size()) { fail("unterminated string"); }
    i++;
    return r;
  }

  void run() {
    static const char *keywords[] = {"if", "then", "elif", "else", "end", "as", "reduce", "foreach", "and", "or", "try", "catch", "label", "def"};
    while (true) {
      while (i < src.size() && isspace((unsigned char)src[i])) { i++; }
      if (i < src.size() && src[i] == '#') { while (i < src.size() && src[i] != '\n') { i++; } continue; }
      if (i >= src.size()) { toks.push_back({T::End, ""}); return; }
      char c = src[i];
      if (c == '.') {
        if (i + 1 < src.size() && src[i + 1] == '.') { i += 2; toks.push_back({T::DotDot, ".."}); continue; }
        if (i + 1 < src.size() && ident_start(src[i + 1])) {
          size_t j = i + 1;
          while (j < src.size() && ident_char(src[j])) { j++; }
          toks.push_back({T::Field, src.substr(i + 1, j - i - 1)});
          i = j;
          continue;
        }
        if (i + 1 < src.size() && src[i + 1] == '"') {
          i += 2;
          toks.push_back({T::Field, read_string()});
          continue;
        }
        i++;
        toks.push_back({T::Dot, "."});
        continue;
      }
      if (c == '$') {
        size_t j = i + 1;
        while (j < src.size() && ident_char(src[j])) { j++; }
        toks.push_back({T::Var, src.substr(i + 1, j - i - 1)});
        i = j;
        continue;
      }
      if (c == '@') {
        size_t j = i + 1;
        while (j < src.size() && ident_char(src[j])) { j++; }
        toks.push_back({T::Format, src.substr(i + 1, j - i - 1)});
        i = j;
        continue;
      }
      if (isdigit((unsigned char)c)) {
        char *endp = nullptr;
        double d = strtod(src.c_str() + i, &endp);
        size_t j = size_t(endp - src.c_str());
        toks.push_back({T::Num, src.substr(i, j - i), d});
        i = j;
        continue;
      }
      if (c == '"') { i++; toks.push_back({T::Str, read_string()}); continue; }
      if (ident_start(c)) {
        size_t j = i;
        while (j < src.size() && (ident_char(src[j]) || (src[j] == ':' && j + 1 < src.size() && src[j + 1] == ':'))) {
          if (src[j] == ':') { j++; }
          j++;
        }
        std::string w = src.substr(i, j - i);
        i = j;
        bool kw = false;
        for (auto k : keywords) { if (w == k) { kw = true; } }
        toks.push_back({kw ? T::Keyword : T::Ident, w});
        continue;
      }
      // jq lexes ?// (destructuring alternatives) as one token.
      if (src.compare(i, 3, "?//") == 0) { fail("?// is not supported"); }
      static const char *two[] = {"==", "!=", "<=", ">=", "//"};
      bool matched = false;
      for (auto p : two) {
        if (src.compare(i, 2, p) == 0) { toks.push_back({T::Punct, p}); i += 2; matched = true; break; }
      }
      if (matched) { continue; }
      if (strchr("|,()[]{}:;<>+-*/%?", c)) { toks.push_back({T::Punct, std::string(1, c)}); i++; continue; }
      fail(std::string("unexpected character '") + c + "'");
    }
  }
};

struct parser_t {
  std::vector<token> toks{};
  size_t p{0};
  int next_label{0};
  std::vector<std::pair<std::string, int>> labels{};

  [[noreturn]] void fail(const std::string &m) {
    throw std::runtime_error("syntax error: " + m + " near '" + toks[p].s + "'");
  }
  const token &peek() const { return toks[p]; }
  bool is_punct(const char *s) const { return toks[p].t == T::Punct && toks[p].s == s; }
  bool is_kw(const char *s) const { return toks[p].t == T::Keyword && toks[p].s == s; }
  void expect_punct(const char *s) { if (!is_punct(s)) { fail(std::string("expected '") + s + "'"); } p++; }
  void expect_kw(const char *s) { if (!is_kw(s)) { fail(std::string("expected '") + s + "'"); } p++; }

  // pipe: lowest precedence; handles "term as $x | body" and "def".
  node_ptr parse_pipe() {
    if (is_kw("label")) {
      p++;
      if (peek().t != T::Var) { fail("expected $label"); }
      std::string name = toks[p++].s;
      expect_punct("|");
      auto n = std::make_unique<label_node>();
      n->id = next_label++;
      labels.emplace_back(name, n->id);
      n->body = parse_pipe();
      labels.pop_back();
      return n;
    }
    node_ptr lhs = parse_comma();
    if (is_kw("as")) {
      p++;
      if (peek().t != T::Var) { fail("expected variable after 'as'"); }
      auto n = std::make_unique<bind_node>();
      n->name = toks[p++].s;
      expect_punct("|");
      n->source = std::move(lhs);
      n->body = parse_pipe();
      return n;
    }
    if (is_punct("|")) {
      p++;
      auto n = std::make_unique<pipe_node>();
      n->a = std::move(lhs);
      n->b = parse_pipe();
      return n;
    }
    return lhs;
  }

  node_ptr parse_comma() {
    node_ptr lhs = parse_alt();
    while (is_punct(",")) {
      p++;
      auto n = std::make_unique<comma_node>();
      n->a = std::move(lhs);
      n->b = parse_alt();
      lhs = std::move(n);
    }
    return lhs;
  }

  node_ptr make_bin(op o, node_ptr a, node_ptr b) {
    auto n = std::make_unique<binop_node>();
    n->o = o;
    n->a = std::move(a);
    n->b = std::move(b);
    return n;
  }

  node_ptr parse_alt() {
    node_ptr lhs = parse_or();
    if (is_punct("//")) {
      p++;
      return make_bin(op::alt, std::move(lhs), parse_alt());  // right-associative
    }
    return lhs;
  }
  node_ptr parse_or() {
    node_ptr lhs = parse_and();
    while (is_kw("or")) { p++; lhs = make_bin(op::or_, std::move(lhs), parse_and()); }
    return lhs;
  }
  node_ptr parse_and() {
    node_ptr lhs = parse_cmp();
    while (is_kw("and")) { p++; lhs = make_bin(op::and_, std::move(lhs), parse_cmp()); }
    return lhs;
  }
  node_ptr parse_cmp() {
    node_ptr lhs = parse_add();
    static const std::pair<const char *, op> ops[] = {{"==", op::eq}, {"!=", op::ne}, {"<", op::lt}, {"<=", op::le}, {">", op::gt}, {">=", op::ge}};
    for (auto &o : ops) {
      if (is_punct(o.first)) { p++; return make_bin(o.second, std::move(lhs), parse_add()); }
    }
    return lhs;
  }
  node_ptr parse_add() {
    node_ptr lhs = parse_mul();
    while (is_punct("+") || is_punct("-")) {
      op o = is_punct("+") ? op::add : op::sub;
      p++;
      lhs = make_bin(o, std::move(lhs), parse_mul());
    }
    return lhs;
  }
  node_ptr parse_mul() {
    node_ptr lhs = parse_unary();
    while (is_punct("*") || is_punct("/") || is_punct("%")) {
      op o = is_punct("*") ? op::mul : (is_punct("/") ? op::div : op::mod);
      p++;
      lhs = make_bin(o, std::move(lhs), parse_unary());
    }
    return lhs;
  }
  node_ptr parse_unary() {
    if (is_punct("-")) {
      p++;
      auto n = std::make_unique<neg_node>();
      n->a = parse_postfix();
      return n;
    }
    return parse_postfix();
  }

  // Parses the inside of "[...]" after a term: "]" (iterate), "e]", "e:e]", ":e]", "e:]".
  node_ptr parse_bracket_suffix(node_ptr target) {
    expect_punct("[");
    if (is_punct("]")) {
      p++;
      auto n = std::make_unique<iterate_node>();
      n->target = std::move(target);
      return n;
    }
    node_ptr from;
    if (!is_punct(":")) { from = parse_pipe(); }
    if (is_punct(":")) {
      p++;
      auto n = std::make_unique<slice_node>();
      n->target = std::move(target);
      n->from = std::move(from);
      if (!is_punct("]")) { n->to = parse_pipe(); }
      expect_punct("]");
      return n;
    }
    expect_punct("]");
    auto n = std::make_unique<index_node>();
    n->target = std::move(target);
    n->index = std::move(from);
    return n;
  }

  static void mark_optional(node *n) {
    if (auto f = dynamic_cast<field_node *>(n)) { f->opt = true; }
    else if (auto x = dynamic_cast<index_node *>(n)) { x->opt = true; }
    else if (auto s = dynamic_cast<slice_node *>(n)) { s->opt = true; }
    else if (auto it = dynamic_cast<iterate_node *>(n)) { it->opt = true; }
  }

  bool paren_term{false};

  node_ptr parse_postfix() {
    paren_term = false;
    node_ptr t = parse_term();
    bool bare_step = !paren_term;
    while (true) {
      if (peek().t == T::Field) {
        auto n = std::make_unique<field_node>();
        n->target = std::move(t);
        n->name = toks[p++].s;
        t = std::move(n);
        bare_step = true;
      } else if (peek().t == T::Dot && toks[p + 1].t == T::Punct && toks[p + 1].s == "[") {
        p++;
        t = parse_bracket_suffix(std::move(t));
        bare_step = true;
      } else if (is_punct("[")) {
        t = parse_bracket_suffix(std::move(t));
        bare_step = true;
      } else if (is_punct("?")) {
        p++;
        // Postfix "?" on a path step suppresses that step's errors only (as
        // jq's INDEX_OPT); elsewhere it is try.
        bool is_step = dynamic_cast<field_node *>(t.get()) || dynamic_cast<index_node *>(t.get()) || dynamic_cast<iterate_node *>(t.get()) || dynamic_cast<slice_node *>(t.get());
        if (is_step && bare_step) {
          mark_optional(t.get());
        } else {
          auto n = std::make_unique<try_node>();
          n->body = std::move(t);
          t = std::move(n);
        }
      } else if (is_kw("as") && false) {
      } else {
        return t;
      }
    }
  }

  // Each nested term recurses; deep nesting would overflow the stack.
  int depth{0};
  static constexpr int MAX_DEPTH = 256;
  struct depth_guard {
    parser_t &p;
    explicit depth_guard(parser_t &parser) : p(parser) {
      if (++p.depth > MAX_DEPTH) { p.fail("filter nested too deeply"); }
    }
    ~depth_guard() { p.depth--; }
    depth_guard(const depth_guard &) = delete;
    depth_guard &operator=(const depth_guard &) = delete;
  };

  node_ptr parse_term() {
    depth_guard guard(*this);
    const token &tk = peek();
    switch (tk.t) {
    case T::Dot: {
      p++;
      if (is_punct("[")) { return parse_bracket_suffix(nullptr); }
      if (peek().t == T::Str) {  // ."foo" when lexed separately
        auto n = std::make_unique<field_node>();
        n->name = toks[p++].s;
        return n;
      }
      return std::make_unique<identity_node>();
    }
    case T::DotDot: p++; return std::make_unique<recurse_node>();
    case T::Field: {
      auto n = std::make_unique<field_node>();
      n->name = toks[p++].s;
      return n;
    }
    case T::Num: {
      auto n = std::make_unique<literal_node>();
      n->v = value::number(tk.num);
      n->text = tk.s;
      p++;
      return n;
    }
    case T::Str: {
      auto n = std::make_unique<literal_node>();
      n->v = value::string(tk.s);
      p++;
      return n;
    }
    case T::Var: {
      if (tk.s == "__loc__") { fail("$__loc__ unsupported"); }
      auto n = std::make_unique<var_node>();
      n->name = tk.s;
      p++;
      return n;
    }
    case T::Format: {
      if (tk.s == "json") { p++; auto n = std::make_unique<call_node>(); n->name = "tojson"; return n; }
      if (tk.s == "text") { p++; auto n = std::make_unique<call_node>(); n->name = "tostring"; return n; }
      fail("unsupported format @" + tk.s);
    }
    case T::Keyword: {
      if (tk.s == "if") {
        p++;
        auto n = std::make_unique<if_node>();
        n->cond = parse_pipe();
        expect_kw("then");
        n->then_ = parse_pipe();
        if_node *cur = n.get();
        while (is_kw("elif")) {
          p++;
          auto m = std::make_unique<if_node>();
          m->cond = parse_pipe();
          expect_kw("then");
          m->then_ = parse_pipe();
          if_node *mp = m.get();
          cur->else_ = std::move(m);
          cur = mp;
        }
        if (is_kw("else")) { p++; cur->else_ = parse_pipe(); }
        expect_kw("end");
        return n;
      }
      if (tk.s == "reduce" || tk.s == "foreach") {
        bool is_reduce = tk.s == "reduce";
        p++;
        node_ptr source = parse_postfix();
        expect_kw("as");
        if (peek().t != T::Var) { fail("expected variable"); }
        std::string name = toks[p++].s;
        expect_punct("(");
        node_ptr init = parse_pipe();
        expect_punct(";");
        node_ptr update = parse_pipe();
        node_ptr extract;
        if (!is_reduce && is_punct(";")) { p++; extract = parse_pipe(); }
        expect_punct(")");
        if (is_reduce) {
          auto n = std::make_unique<reduce_node>();
          n->source = std::move(source); n->init = std::move(init); n->update = std::move(update); n->name = name;
          return n;
        }
        auto n = std::make_unique<foreach_node>();
        n->source = std::move(source); n->init = std::move(init); n->update = std::move(update); n->extract = std::move(extract); n->name = name;
        return n;
      }
      if (tk.s == "try") {
        p++;
        auto n = std::make_unique<try_node>();
        n->body = parse_postfix();
        if (is_kw("catch")) { p++; n->handler = parse_postfix(); }
        return n;
      }
      fail("unexpected keyword " + tk.s);
    }
    case T::Ident: {
      std::string name = tk.s;
      p++;
      if (name == "break") {
        if (peek().t != T::Var) { fail("expected $label after break"); }
        std::string l = toks[p++].s;
        for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
          if (it->first == l) {
            struct break_node : node {
              int id{0};
              void eval(const value &, const env_ptr &, emit_ref) const override { throw break_signal{id}; }
            };
            auto n = std::make_unique<break_node>();
            n->id = it->second;
            return n;
          }
        }
        fail("$*label-" + l + " is not defined");
      }
      if ((name == "true" || name == "false" || name == "null") && !is_punct("(")) {
        auto lit = std::make_unique<literal_node>();
        lit->v = name == "null" ? value::null() : value::boolean(name == "true");
        return lit;
      }
      auto n = std::make_unique<call_node>();
      n->name = name;
      if (is_punct("(")) {
        p++;
        n->args.push_back(parse_pipe());
        while (is_punct(";")) { p++; n->args.push_back(parse_pipe()); }
        expect_punct(")");
      }
      return n;
    }
    case T::Punct: {
      if (tk.s == "(") {
        p++;
        node_ptr e = parse_pipe();
        expect_punct(")");
        paren_term = true;
        return e;
      }
      if (tk.s == "[") {
        p++;
        auto n = std::make_unique<array_node>();
        if (!is_punct("]")) { n->body = parse_pipe(); }
        expect_punct("]");
        return n;
      }
      if (tk.s == "{") {
        p++;
        auto n = std::make_unique<object_node>();
        while (!is_punct("}")) {
          node_ptr key, val;
          const token &k = peek();
          std::string kname;
          bool simple = false;
          if (k.t == T::Ident || k.t == T::Keyword) { kname = k.s; simple = true; p++; }
          else if (k.t == T::Str) { kname = k.s; simple = true; p++; }
          else if (k.t == T::Var) {
            p++;
            auto lk = std::make_unique<literal_node>();
            lk->v = value::string(k.s);
            auto vv = std::make_unique<var_node>();
            vv->name = k.s;
            key = std::move(lk);
            val = std::move(vv);
          } else if (is_punct("(")) {
            p++;
            key = parse_pipe();
            expect_punct(")");
          } else {
            fail("bad object key");
          }
          if (simple) {
            auto lk = std::make_unique<literal_node>();
            lk->v = value::string(kname);
            key = std::move(lk);
          }
          if (is_punct(":")) {
            p++;
            val = parse_obj_value();
          } else if (!val) {
            if (!simple) { fail("object key needs a value"); }
            auto f = std::make_unique<field_node>();
            f->name = kname;
            val = std::move(f);
          }
          n->entries.emplace_back(std::move(key), std::move(val));
          if (is_punct(",")) { p++; continue; }
          if (!is_punct("}")) { fail("expected ',' or '}' in object"); }
        }
        p++;
        return n;
      }
      fail("unexpected '" + tk.s + "'");
    }
    default: fail("unexpected end of filter");
    }
  }

  // An object value is an expression without top-level ',' (jq: ExpD).
  node_ptr parse_obj_value() {
    node_ptr lhs = parse_alt();
    while (is_punct("|")) {
      p++;
      auto n = std::make_unique<pipe_node>();
      n->a = std::move(lhs);
      n->b = parse_alt();
      lhs = std::move(n);
    }
    return lhs;
  }
};

node_ptr compile(const std::string &src) {
  lexer lx{};
  lx.src = src;
  lx.run();
  parser_t ps{};
  ps.toks = std::move(lx.toks);
  node_ptr n = ps.parse_pipe();
  if (ps.peek().t != T::End) { ps.fail("unexpected token"); }
  return n;
}

} // namespace sjq
