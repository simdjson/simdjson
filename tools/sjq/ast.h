#ifndef SJQ_AST_H
#define SJQ_AST_H

#include "value.h"
#include "writer.h"

#include <cmath>
#include <functional>

namespace sjq {

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
// Fuzzing builds bound the work of generators (range, recurse) so that a
// filter such as range(1e18) does not run for ever.
inline thread_local long long g_fuzz_budget = 0;
inline void fuzz_tick() {
  if (--g_fuzz_budget < 0) { throw jq_error("fuzzing budget exhausted"); }
}
#else
inline void fuzz_tick() {}
#endif

// Variable environment: a persistent linked list.
struct env {
  std::string name{};
  value v{};
  std::shared_ptr<const env> next{};
};
using env_ptr = std::shared_ptr<const env>;

inline const value *lookup(const env_ptr &e, const std::string &name) {
  for (const env *p = e.get(); p; p = p->next.get()) {
    if (p->name == name) { return &p->v; }
  }
  return nullptr;
}

using emit_fn = std::function<void(const value &)>;

// Lightweight non-owning callable reference to avoid std::function allocations
// on the hot paths.
class emit_ref {
  void *obj{};
  void (*call)(void *, const value &);
public:
  template <class F>
  emit_ref(F &&f) : obj((void *)&f), call([](void *o, const value &v) { (*(std::remove_reference_t<F> *)o)(v); }) {}
  void operator()(const value &v) const { call(obj, v); }
};

struct node {
  virtual ~node() = default;
  virtual void eval(const value &in, const env_ptr &e, emit_ref out) const = 0;
  // Path-only fast information: is this node a pure ".", ".k", ".[n]" chain
  // that yields exactly one output?
};
using node_ptr = std::unique_ptr<node>;

struct identity_node : node {
  void eval(const value &in, const env_ptr &, emit_ref out) const override { out(in); }
};

inline void index_value(const value &t, const value &idx, emit_ref out, bool opt) {
  if (t.k == K::Null) {
    if (idx.k == K::Str || idx.k == K::Num || idx.k == K::Null) { out(value::null()); return; }
  }
  if (t.k == K::Obj && idx.k == K::Str) {
    value r;
    if (object_get(t, get_str(idx), r)) { out(r); } else { out(value::null()); }
    return;
  }
  if (t.k == K::Arr && idx.k == K::Num) {
    double d = get_num(idx);
    value r;
    if (!std::isnan(d) && array_get(t, to_int64(std::floor(d)), r)) { out(r); } else { out(value::null()); }
    return;
  }
  if (opt) { return; }
  if (idx.k == K::Str) {
    throw jq_error(std::string("Cannot index ") + kind_name(t.k) + " with string \"" + std::string(get_str(idx)) + "\"");
  }
  throw jq_error(std::string("Cannot index ") + kind_name(t.k) + " with " + kind_name(idx.k));
}

struct field_node : node {
  node_ptr target{};  // may be null: applies to input
  std::string name{};
  bool opt{false};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    auto apply = [&](const value &t) {
      if (t.k == K::Obj) {
        value r;
        if (object_get(t, name, r)) { out(r); } else { out(value::null()); }
      } else if (t.k == K::Null) {
        out(value::null());
      } else if (!opt) {
        throw jq_error(std::string("Cannot index ") + kind_name(t.k) + " with string \"" + name + "\"");
      }
    };
    if (target) { target->eval(in, e, apply); } else { apply(in); }
  }
};

struct index_node : node {
  node_ptr target{};
  node_ptr index{};
  bool opt{false};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    auto apply = [&](const value &t) {
      index->eval(in, e, [&](const value &idx) { index_value(t, idx, out, opt); });
    };
    if (target) { target->eval(in, e, apply); } else { apply(in); }
  }
};

struct slice_node : node {
  node_ptr target{}, from{}, to{};  // from/to may be null
  bool opt{false};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    auto apply = [&](const value &t) {
      auto do_slice = [&](const value *f, const value *l) {
        if (t.k == K::Null) { out(value::null()); return; }
        if (t.k != K::Arr && t.k != K::Str) {
          if (opt) { return; }
          throw jq_error(std::string("Cannot index ") + kind_name(t.k) + " with object");
        }
        // jq counts strings in code points.
        int64_t n = t.k == K::Arr ? int64_t(container_size(t)) : int64_t(utf8_count(get_str(t)));
        auto clamp = [&](const value *x, int64_t dflt) {
          if (!x || x->k == K::Null) { return dflt; }
          int64_t i = to_int64(std::floor(get_num(*x)));
          if (i < 0) { i += n; }
          return std::max<int64_t>(0, std::min<int64_t>(n, i));
        };
        int64_t a = clamp(f, 0), b = clamp(l, n);
        if (b < a) { b = a; }
        if (t.k == K::Str) {
          std::string_view str = get_str(t);
          size_t ba = utf8_offset(str, size_t(a));
          size_t bb = ba + utf8_offset(str.substr(ba), size_t(b - a));
          out(value::string(std::string(str.substr(ba, bb - ba))));
        } else {
          auto v = to_vec(t);
          out(value::array(std::vector<value>(v.begin() + a, v.begin() + b)));
        }
      };
      auto with_to = [&](const value *f) {
        if (to) { to->eval(in, e, [&](const value &l) { do_slice(f, &l); }); } else { do_slice(f, nullptr); }
      };
      if (from) { from->eval(in, e, [&](const value &f) { with_to(&f); }); } else { with_to(nullptr); }
    };
    if (target) { target->eval(in, e, apply); } else { apply(in); }
  }
};

struct iterate_node : node {
  node_ptr target{};
  bool opt{false};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    auto apply = [&](const value &t) {
      if (t.k == K::Arr) {
        for_each_elem(t, [&](const value &x) { out(x); });
      } else if (t.k == K::Obj) {
        for_each_field(t, [&](std::string_view, const value &x) { out(x); });
      } else if (!opt) {
        cannot_iterate(t);
      }
    };
    if (target) { target->eval(in, e, apply); } else { apply(in); }
  }
};

struct try_node : node {
  node_ptr body{}, handler{};  // handler may be null
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    try {
      body->eval(in, e, out);
    } catch (jq_error &err) {
      if (handler) {
        handler->eval(err.payload.k == K::Null ? value::string(err.what()) : err.payload, e, out);
      }
    }
  }
};

struct recurse_node : node {
  static void rec(const value &v, emit_ref out) {
    check_stack();
    out(v);
    if (v.k == K::Arr) { for_each_elem(v, [&](const value &x) { rec(x, out); }); }
    else if (v.k == K::Obj) { for_each_field(v, [&](std::string_view, const value &x) { rec(x, out); }); }
  }
  void eval(const value &in, const env_ptr &, emit_ref out) const override { rec(in, out); }
};

struct literal_node : node {
  value v{};
  std::string text{};  // source text of a number literal
  void eval(const value &, const env_ptr &, emit_ref out) const override { out(v); }
};

struct var_node : node {
  std::string name{};
  void eval(const value &, const env_ptr &e, emit_ref out) const override {
    const value *v = lookup(e, name);
    if (!v) { throw jq_error("$" + name + " is not defined"); }
    out(*v);
  }
};

struct pipe_node : node {
  node_ptr a{}, b{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    check_stack();
    a->eval(in, e, [&](const value &x) { b->eval(x, e, out); });
  }
};

struct comma_node : node {
  node_ptr a{}, b{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    a->eval(in, e, out);
    b->eval(in, e, out);
  }
};

struct bind_node : node {  // source as $name | body
  node_ptr source{}, body{};
  std::string name{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    source->eval(in, e, [&](const value &x) {
      auto ne = std::make_shared<env>(env{name, x, e});
      body->eval(in, ne, out);
    });
  }
};

struct reduce_node : node {
  node_ptr source{}, init{}, update{};
  std::string name{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    init->eval(in, e, [&](const value &start) {
      value acc = start;
      bool have = true;
      source->eval(in, e, [&](const value &x) {
        auto ne = std::make_shared<env>(env{name, x, e});
        value last;
        bool got = false;
        update->eval(have ? acc : value::null(), ne, [&](const value &r) { last = r; got = true; });
        acc = got ? last : value::null();
        have = got;
      });
      if (have) { out(acc); } else { out(value::null()); }
    });
  }
};

struct foreach_node : node {
  node_ptr source{}, init{}, update{}, extract{};
  std::string name{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    init->eval(in, e, [&](const value &start) {
      value acc = start;
      source->eval(in, e, [&](const value &x) {
        auto ne = std::make_shared<env>(env{name, x, e});
        update->eval(acc, ne, [&](const value &r) {
          acc = r;
          if (extract) { extract->eval(r, ne, out); } else { out(r); }
        });
      });
    });
  }
};

struct if_node : node {
  node_ptr cond{}, then_{}, else_{};  // else_ may be null (identity)
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    cond->eval(in, e, [&](const value &c) {
      if (truthy(c)) { then_->eval(in, e, out); }
      else if (else_) { else_->eval(in, e, out); }
      else { out(in); }
    });
  }
};

struct label_node : node {
  node_ptr body{};
  int id{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    try { body->eval(in, e, out); } catch (break_signal &b) { if (b.label != id) { throw; } }
  }
};

struct array_node : node {
  node_ptr body{};  // may be null
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    std::vector<value> r;
    if (body) { body->eval(in, e, [&](const value &x) { r.push_back(x); }); }
    out(value::array(std::move(r)));
  }
};

struct object_node : node {
  std::vector<std::pair<node_ptr, node_ptr>> entries{};
  void build(size_t i, std::vector<std::pair<std::string, value>> &acc, const value &in, const env_ptr &e, emit_ref out) const {
    if (i == entries.size()) {
      out(value::object(acc));
      return;
    }
    entries[i].first->eval(in, e, [&](const value &k) {
      if (k.k != K::Str) { throw jq_error(std::string("Object keys must be strings")); }
      entries[i].second->eval(in, e, [&](const value &v) {
        std::string key(get_str(k));
        size_t saved = acc.size();
        auto it = std::find_if(acc.begin(), acc.end(), [&](const auto &kv) { return kv.first == key; });
        value prev;
        bool replaced = false;
        if (it != acc.end()) { prev = it->second; it->second = v; replaced = true; }
        else { acc.emplace_back(std::move(key), v); }
        build(i + 1, acc, in, e, out);
        if (replaced) { it->second = prev; } else { acc.resize(saved); }
      });
    });
  }
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    std::vector<std::pair<std::string, value>> acc;
    build(0, acc, in, e, out);
  }
};

enum class op { add, sub, mul, div, mod, eq, ne, lt, le, gt, ge, and_, or_, alt };

value merge_objects(const value &a, const value &b, bool deep);
value arith(op o, const value &a, const value &b);

struct binop_node : node {
  op o{};
  node_ptr a{}, b{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    check_stack();
    if (o == op::and_ || o == op::or_) {
      a->eval(in, e, [&](const value &x) {
        bool tx = truthy(x);
        if (o == op::and_ && !tx) { out(value::boolean(false)); return; }
        if (o == op::or_ && tx) { out(value::boolean(true)); return; }
        b->eval(in, e, [&](const value &y) { out(value::boolean(truthy(y))); });
      });
      return;
    }
    if (o == op::alt) {
      // As in jq 1.8, an error on the left propagates.
      bool any = false;
      a->eval(in, e, [&](const value &x) { if (truthy(x)) { any = true; out(x); } });
      if (!any) { b->eval(in, e, out); }
      return;
    }
    // jq evaluates the right operand in the outer loop.
    b->eval(in, e, [&](const value &y) {
      a->eval(in, e, [&](const value &x) { out(arith(o, x, y)); });
    });
  }
};

struct neg_node : node {
  node_ptr a{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override {
    a->eval(in, e, [&](const value &x) {
      if (x.k != K::Num) { throw jq_error(std::string(kind_name(x.k)) + " cannot be negated"); }
      out(value::number(0 - get_num(x)));  // as jq 1.8: -(0) is 0
    });
  }
};

// Built-in function calls.
struct call_node : node {
  std::string name{};
  std::vector<node_ptr> args{};
  void eval(const value &in, const env_ptr &e, emit_ref out) const override;
};

// Parses a filter; throws std::runtime_error on a syntax error.
node_ptr compile(const std::string &src);

} // namespace sjq

#endif // SJQ_AST_H
