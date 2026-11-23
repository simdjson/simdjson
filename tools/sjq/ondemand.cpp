#include "ondemand.h"

namespace sjq {

static od_cond_ptr od_compile_cond(const node *n) {
  if (auto b = dynamic_cast<const binop_node *>(n)) {
    if (b->o == op::and_ || b->o == op::or_) {
      auto l = std::make_unique<od_logic>();
      l->o = b->o;
      l->a = od_compile_cond(b->a.get());
      l->b = od_compile_cond(b->b.get());
      if (!l->a || !l->b) { return nullptr; }
      return l;
    }
    if (b->o == op::eq || b->o == op::ne || b->o == op::lt || b->o == op::le || b->o == op::gt || b->o == op::ge) {
      const node *pa = b->a.get(), *pb = b->b.get();
      op o = b->o;
      if (dynamic_cast<const literal_node *>(pa)) {
        std::swap(pa, pb);
        // Mirror the comparison so the path is on the left.
        if (o == op::lt) { o = op::gt; } else if (o == op::gt) { o = op::lt; }
        else if (o == op::le) { o = op::ge; } else if (o == op::ge) { o = op::le; }
      }
      auto lit = dynamic_cast<const literal_node *>(pb);
      if (!lit || lit->v.k == K::Arr || lit->v.k == K::Obj) { return nullptr; }
      od_ptr path = od_compile(pa);
      if (!path || !path->is_nav) { return nullptr; }
      auto c = std::make_unique<od_cmp>();
      c->path = std::move(path);
      c->o = o;
      c->lit = lit->v;
      return c;
    }
    return nullptr;
  }
  if (auto p = dynamic_cast<const pipe_node *>(n)) {
    auto call = dynamic_cast<const call_node *>(p->b.get());
    if (call && call->name == "not" && call->args.empty()) {
      auto x = std::make_unique<od_not>();
      x->a = od_compile_cond(p->a.get());
      if (!x->a) { return nullptr; }
      return x;
    }
  }
  if (auto call = dynamic_cast<const call_node *>(n)) {
    if (call->name == "not" && call->args.empty()) {
      auto x = std::make_unique<od_not>();
      auto t = std::make_unique<od_truthy>();
      t->path = std::make_unique<od_identity>();
      x->a = std::move(t);
      return x;
    }
    if (call->name == "has" && call->args.size() == 1) {
      auto lit = dynamic_cast<const literal_node *>(call->args[0].get());
      if (!lit || lit->v.k != K::Str) { return nullptr; }
      auto h = std::make_unique<od_has>();
      h->key = std::string(get_str(lit->v));
      return h;
    }
    if ((call->name == "true" || call->name == "false") && call->args.empty()) {
      auto c = std::make_unique<od_const>();
      c->v = call->name == "true";
      return c;
    }
  }
  od_ptr path = od_compile(n);
  if (path && path->is_nav) {
    auto t = std::make_unique<od_truthy>();
    t->path = std::move(path);
    return t;
  }
  return nullptr;
}

od_ptr od_compile(const node *n) {
  if (dynamic_cast<const identity_node *>(n)) { return std::make_unique<od_identity>(); }
  if (auto f = dynamic_cast<const field_node *>(n)) {
    auto s = std::make_unique<od_step>();
    s->kind = od_step::FIELD;
    s->name = f->name;
    s->opt = f->opt;
    if (f->target) {
      s->target = od_compile(f->target.get());
      if (!s->target || !s->target->is_nav) { return nullptr; }
    }
    return s;
  }
  if (auto x = dynamic_cast<const index_node *>(n)) {
    auto lit = dynamic_cast<const literal_node *>(x->index.get());
    if (!lit) { return nullptr; }
    auto s = std::make_unique<od_step>();
    s->opt = x->opt;
    if (lit->v.k == K::Str) {
      s->kind = od_step::FIELD;
      s->name = std::string(get_str(lit->v));
    } else if (lit->v.k == K::Num) {
      double d = get_num(lit->v);
      if (d < 0 || d != std::floor(d) || d > 1e15) { return nullptr; }
      s->kind = od_step::INDEX;
      s->idx = size_t(d);
    } else {
      return nullptr;
    }
    if (x->target) {
      s->target = od_compile(x->target.get());
      if (!s->target || !s->target->is_nav) { return nullptr; }
    }
    return s;
  }
  if (auto it = dynamic_cast<const iterate_node *>(n)) {
    auto s = std::make_unique<od_step>();
    s->kind = od_step::ITER;
    s->opt = it->opt;
    if (it->target) {
      s->target = od_compile(it->target.get());
      if (!s->target || !s->target->is_nav) { return nullptr; }
    }
    return s;
  }
  if (auto t = dynamic_cast<const try_node *>(n)) {
    if (t->handler) { return nullptr; }
    od_ptr body = od_compile(t->body.get());
    // Only steps are made optional: their type errors become empty output.
    if (!body || !body->is_nav || !(dynamic_cast<od_step *>(body.get()))) { return nullptr; }
    body->set_opt();
    return body;
  }
  if (auto p = dynamic_cast<const pipe_node *>(n)) {
    auto r = std::make_unique<od_pipe>();
    r->a = od_compile(p->a.get());
    r->b = od_compile(p->b.get());
    if (!r->a || !r->b || !r->a->is_nav) { return nullptr; }
    r->is_nav = r->b->is_nav;
    return r;
  }
  if (auto c = dynamic_cast<const comma_node *>(n)) {
    auto r = std::make_unique<od_comma>();
    r->a = od_compile(c->a.get());
    r->b = od_compile(c->b.get());
    if (!r->a || !r->b) { return nullptr; }
    r->is_nav = r->a->is_nav && r->b->is_nav;
    return r;
  }
  if (auto call = dynamic_cast<const call_node *>(n)) {
    if (call->name == "select" && call->args.size() == 1) {
      auto s = std::make_unique<od_select>();
      s->cond = od_compile_cond(call->args[0].get());
      if (!s->cond) { return nullptr; }
      return s;
    }
    if (call->name == "length" && call->args.empty()) { return std::make_unique<od_length>(); }
    if (call->name == "empty" && call->args.empty()) { return std::make_unique<od_empty>(); }
    return nullptr;
  }
  if (auto lit = dynamic_cast<const literal_node *>(n)) {
    auto l = std::make_unique<od_literal>();
    l->v = lit->v;
    l->text = lit->text;
    return l;
  }
  if (auto o = dynamic_cast<const object_node *>(n)) {
    auto r = std::make_unique<od_object>();
    for (auto &e : o->entries) {
      auto k = dynamic_cast<const literal_node *>(e.first.get());
      if (!k || k->v.k != K::Str) { return nullptr; }
      std::string key(get_str(k->v));
      for (auto &prev : r->entries) { if (prev.first == key) { return nullptr; } }
      od_ptr v = od_compile(e.second.get());
      if (!v) { return nullptr; }
      r->entries.emplace_back(std::move(key), std::move(v));
    }
    return r;
  }
  if (auto a = dynamic_cast<const array_node *>(n)) {
    auto r = std::make_unique<od_array_cons>();
    if (a->body) {
      r->body = od_compile(a->body.get());
      if (!r->body) { return nullptr; }
    }
    return r;
  }
  return nullptr;
}

} // namespace sjq
