#include "ast.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>

namespace sjq {

std::string error_text(const value &v) {
  std::string s = to_json(v);
  if (s.size() > 28) {
    s.resize(25);
    s += "...";
  }
  return s;
}

value arith(op o, const value &a, const value &b) {
  switch (o) {
  case op::add:
    if (a.k == K::Null) { return b; }
    if (b.k == K::Null) { return a; }
    if (a.k == K::Num && b.k == K::Num) { return value::number(get_num(a) + get_num(b)); }
    if (a.k == K::Str && b.k == K::Str) {
      std::string s(get_str(a));
      s.append(get_str(b));
      return value::string(std::move(s));
    }
    if (a.k == K::Arr && b.k == K::Arr) {
      auto v = to_vec(a);
      for_each_elem(b, [&](const value &x) { v.push_back(x); });
      return value::array(std::move(v));
    }
    if (a.k == K::Obj && b.k == K::Obj) { return merge_objects(a, b, false); }
    break;
  case op::sub:
    if (a.k == K::Num && b.k == K::Num) { return value::number(get_num(a) - get_num(b)); }
    if (a.k == K::Arr && b.k == K::Arr) {
      auto bv = to_vec(b);
      std::vector<value> r;
      for_each_elem(a, [&](const value &x) {
        for (auto &y : bv) { if (compare(x, y) == 0) { return; } }
        r.push_back(x);
      });
      return value::array(std::move(r));
    }
    break;
  case op::mul:
    if (a.k == K::Num && b.k == K::Num) { return value::number(get_num(a) * get_num(b)); }
    if (a.k == K::Obj && b.k == K::Obj) { return merge_objects(a, b, true); }
    if ((a.k == K::Str && b.k == K::Num) || (a.k == K::Num && b.k == K::Str)) {
      const value &s = a.k == K::Str ? a : b;
      double n = get_num(a.k == K::Num ? a : b);
      if (std::isnan(n) || n <= 0) { return value::null(); }
      std::string_view piece = get_str(s);
      if (piece.empty()) { return value::string(std::string()); }
      int64_t times = to_int64(std::ceil(n));
      // jq keeps string lengths within an int.
      if (!piece.empty() && times > int64_t(INT32_MAX) / int64_t(piece.size())) { throw jq_error("Repeat string result too long"); }
      std::string r;
      r.reserve(piece.size() * size_t(times));
      for (int64_t i = 0; i < times; i++) {
        fuzz_tick();
        r.append(get_str(s));
      }
      return value::string(std::move(r));
    }
    break;
  case op::div:
    if (a.k == K::Num && b.k == K::Num) {
      if (get_num(b) == 0) { throw jq_error(std::string(to_json(a)) + " and " + to_json(b) + " cannot be divided because the divisor is zero"); }
      return value::number(get_num(a) / get_num(b));
    }
    if (a.k == K::Str && b.k == K::Str) {
      std::vector<value> parts;
      std::string_view s = get_str(a), sep = get_str(b);
      if (s.empty()) { return value::array({}); }
      if (sep.empty()) {
        for (size_t i = 0; i < s.size();) {
          size_t len = utf8_offset(s.substr(i), 1);
          parts.push_back(value::string(std::string(s.substr(i, len))));
          i += len;
        }
      } else {
        size_t pos = 0;
        while (true) {
          size_t f = s.find(sep, pos);
          if (f == std::string_view::npos) { parts.push_back(value::string(std::string(s.substr(pos)))); break; }
          parts.push_back(value::string(std::string(s.substr(pos, f - pos))));
          pos = f + sep.size();
        }
      }
      return value::array(std::move(parts));
    }
    break;
  case op::mod:
    if (a.k == K::Num && b.k == K::Num) {
      int64_t y = to_int64(get_num(b));
      if (y == 0) {
        throw jq_error(std::string(kind_name(a.k)) + " (" + error_text(a) + ") and " + kind_name(b.k) + " (" + error_text(b) + ") cannot be divided (remainder) because the divisor is zero");
      }
      int64_t x = to_int64(get_num(a));
      if (y < 0) { y = -y; }  // to_int64 stays above INT64_MIN
      return value::number(double(x % y));
    }
    break;
  case op::eq: return value::boolean(compare(a, b) == 0);
  case op::ne: return value::boolean(compare(a, b) != 0);
  case op::lt: return value::boolean(compare(a, b) < 0);
  case op::le: return value::boolean(compare(a, b) <= 0);
  case op::gt: return value::boolean(compare(a, b) > 0);
  case op::ge: return value::boolean(compare(a, b) >= 0);
  default: break;
  }
  static const char *names[] = {"added", "subtracted", "multiplied", "divided", "divided"};
  std::string msg = std::string(kind_name(a.k)) + " (" + error_text(a) + ") and " + kind_name(b.k) + " (" + error_text(b) + ") cannot be " + names[int(o)];
  throw jq_error(msg);
}

value merge_objects(const value &a, const value &b, bool deep) {
  auto r = to_fields(a);
  for_each_field(b, [&](std::string_view k, const value &v) {
    auto it = std::find_if(r.begin(), r.end(), [&](const auto &kv) { return kv.first == k; });
    if (it == r.end()) { r.emplace_back(std::string(k), v); return; }
    if (deep && it->second.k == K::Obj && v.k == K::Obj) { it->second = merge_objects(it->second, v, true); }
    else { it->second = v; }
  });
  return value::object(std::move(r));
}

static value length_of(const value &v) {
  switch (v.k) {
  case K::Null: return value::number(0);
  case K::True: case K::False: throw jq_error("boolean (" + to_json(v) + ") has no length");
  case K::Num: return value::number(std::fabs(get_num(v)));
  case K::Str: {
    return value::number(double(utf8_count(get_str(v))));
  }
  default: return value::number(double(container_size(v)));
  }
}

static value keys_of(const value &v, bool sorted) {
  std::vector<value> r{};
  if (v.k == K::Obj) {
    std::vector<std::string> ks;
    for_each_field(v, [&](std::string_view k, const value &) { ks.emplace_back(k); });
    if (sorted) { std::sort(ks.begin(), ks.end()); }
    for (auto &k : ks) { r.push_back(value::string(std::move(k))); }
  } else if (v.k == K::Arr) {
    size_t n = container_size(v);
    for (size_t i = 0; i < n; i++) { r.push_back(value::number(double(i))); }
  } else {
    throw jq_error(std::string(kind_name(v.k)) + " (" + to_json(v) + ") has no keys");
  }
  return value::array(std::move(r));
}

static void require_kind(const value &v, K k, const char *what) {
  if (v.k != k) { throw jq_error(std::string(kind_name(v.k)) + " (" + to_json(v).substr(0, 11) + ") " + what); }
}

static bool contains(const value &a, const value &b) {
  if (a.k == K::Obj && b.k == K::Obj) {
    bool ok = true;
    for_each_field(b, [&](std::string_view k, const value &bv) {
      value av;
      if (!ok) { return; }
      if (!object_get(a, k, av) || !contains(av, bv)) { ok = false; }
    });
    return ok;
  }
  if (a.k == K::Arr && b.k == K::Arr) {
    auto av = to_vec(a);
    bool ok = true;
    for_each_elem(b, [&](const value &bv) {
      if (!ok) { return; }
      bool found = false;
      for (auto &x : av) { if (contains(x, bv)) { found = true; break; } }
      ok = found;
    });
    return ok;
  }
  if (a.k == K::Str && b.k == K::Str) { return get_str(a).find(get_str(b)) != std::string_view::npos; }
  if (kind_rank(a.k) == kind_rank(b.k)) { return compare(a, b) == 0; }
  throw jq_error(std::string(kind_name(a.k)) + " and " + kind_name(b.k) + " cannot have their containment checked");
}

static value tostring_of(const value &v) {
  if (v.k == K::Str) { return v; }
  return value::string(to_json(v));
}

static value sort_by_keys(std::vector<std::pair<value, value>> &kv) {
  std::stable_sort(kv.begin(), kv.end(), [](const auto &x, const auto &y) { return compare(x.first, y.first) < 0; });
  std::vector<value> r{};
  for (auto &p : kv) { r.push_back(p.second); }
  return value::array(std::move(r));
}

// Evaluates f on x and returns its outputs collected in an array value
// (used for sort_by/group_by keys, as jq does).
static value collect(const node &f, const value &x, const env_ptr &e) {
  std::vector<value> r{};
  f.eval(x, e, [&](const value &y) { r.push_back(y); });
  return value::array(std::move(r));
}

void call_node::eval(const value &in, const env_ptr &e, emit_ref out) const {
  check_stack();
  const size_t n = args.size();
  auto arg1 = [&](auto &&f) { args[0]->eval(in, e, f); };
  if (n == 0) {
    if (name == "empty") { return; }
    if (name == "not") { out(value::boolean(!truthy(in))); return; }
    if (name == "length") { out(length_of(in)); return; }
    if (name == "utf8bytelength") { require_kind(in, K::Str, "only strings have UTF-8 byte length"); out(value::number(double(get_str(in).size()))); return; }
    if (name == "keys") { out(keys_of(in, true)); return; }
    if (name == "keys_unsorted") { out(keys_of(in, false)); return; }
    if (name == "type") { out(value::string(kind_name(in.k))); return; }
    if (name == "true") { out(value::boolean(true)); return; }
    if (name == "false") { out(value::boolean(false)); return; }
    if (name == "null") { out(value::null()); return; }
    if (name == "values") { if (in.k != K::Null) { out(in); } return; }
    if (name == "nulls") { if (in.k == K::Null) { out(in); } return; }
    if (name == "booleans") { if (in.k == K::True || in.k == K::False) { out(in); } return; }
    if (name == "numbers") { if (in.k == K::Num) { out(in); } return; }
    if (name == "strings") { if (in.k == K::Str) { out(in); } return; }
    if (name == "arrays") { if (in.k == K::Arr) { out(in); } return; }
    if (name == "objects") { if (in.k == K::Obj) { out(in); } return; }
    if (name == "iterables") { if (in.k == K::Arr || in.k == K::Obj) { out(in); } return; }
    if (name == "scalars") { if (in.k != K::Arr && in.k != K::Obj) { out(in); } return; }
    if (name == "add") {
      if (in.k != K::Arr && in.k != K::Obj) { cannot_iterate(in); }
      value acc;
      bool first = true;
      auto f = [&](const value &x) { acc = first ? x : arith(op::add, acc, x); first = false; };
      if (in.k == K::Arr) { for_each_elem(in, f); } else { for_each_field(in, [&](std::string_view, const value &x) { f(x); }); }
      out(acc);
      return;
    }
    if (name == "first") { value r; if (array_get(in, 0, r)) { out(r); } else { out(value::null()); } return; }
    if (name == "last") { value r; if (array_get(in, -1, r)) { out(r); } else { out(value::null()); } return; }
    if (name == "reverse") {
      if (in.k == K::Str) {
        // jq 1.8 defines reverse by indexing, which fails on strings.
        size_t cps = utf8_count(get_str(in));
        if (cps == 0) { out(value::array({})); return; }
        throw jq_error("Cannot index string with number (" + std::to_string(cps - 1) + ")");
      }
      if (in.k == K::Null) { out(value::array({})); return; }
      auto v = to_vec(in); std::reverse(v.begin(), v.end()); out(value::array(std::move(v))); return;
    }
    if (name == "sort") {
      require_kind(in, K::Arr, "cannot be sorted, as it is not an array");
      auto v = to_vec(in);
      std::stable_sort(v.begin(), v.end(), [](const value &a, const value &b) { return compare(a, b) < 0; });
      out(value::array(std::move(v)));
      return;
    }
    if (name == "unique") {
      require_kind(in, K::Arr, "cannot be sorted, as it is not an array");
      auto v = to_vec(in);
      std::stable_sort(v.begin(), v.end(), [](const value &a, const value &b) { return compare(a, b) < 0; });
      v.erase(std::unique(v.begin(), v.end(), [](const value &a, const value &b) { return compare(a, b) == 0; }), v.end());
      out(value::array(std::move(v)));
      return;
    }
    if (name == "min" || name == "max") {
      require_kind(in, K::Arr, "cannot be iterated over");
      value best;
      bool first = true;
      bool want_max = name == "max";
      for_each_elem(in, [&](const value &x) {
        if (first) { best = x; first = false; return; }
        int c = compare(x, best);
        if (want_max ? c >= 0 : c < 0) { best = x; }
      });
      out(best);
      return;
    }
    if (name == "tostring") { out(tostring_of(in)); return; }
    if (name == "tonumber") {
      if (in.k == K::Num) { out(in); return; }
      require_kind(in, K::Str, "cannot be parsed as a number");
      std::string s(get_str(in));
      char *endp = nullptr;
      double d = strtod(s.c_str(), &endp);
      if (s.empty() || *endp) { throw jq_error("Cannot parse '" + s + "' as JSON"); }
      out(value::number(d));
      return;
    }
    if (name == "ascii_downcase" || name == "ascii_upcase") {
      require_kind(in, K::Str, "cannot be case-converted");
      std::string s(get_str(in));
      for (char &c : s) {
        if (name == "ascii_downcase" && c >= 'A' && c <= 'Z') { c = char(c + 32); }
        if (name == "ascii_upcase" && c >= 'a' && c <= 'z') { c = char(c - 32); }
      }
      out(value::string(std::move(s)));
      return;
    }
    if (name == "floor") { out(value::number(std::floor(get_num(in)))); return; }
    if (name == "ceil") { out(value::number(std::ceil(get_num(in)))); return; }
    if (name == "round") { out(value::number(std::round(get_num(in)))); return; }
    if (name == "sqrt") { out(value::number(std::sqrt(get_num(in)))); return; }
    if (name == "fabs" || name == "abs") { out(value::number(std::fabs(get_num(in)))); return; }
    if (name == "to_entries") {
      if (in.k != K::Obj) { (void)keys_of(in, false); }  // raises jq's error
      std::vector<value> r;
      for_each_field(in, [&](std::string_view k, const value &v) {
        r.push_back(value::object({{"key", value::string(std::string(k))}, {"value", v}}));
      });
      out(value::array(std::move(r)));
      return;
    }
    if (name == "from_entries") {
      std::vector<std::pair<std::string, value>> r;
      for_each_elem(in, [&](const value &x) {
        value k, v;
        if (!object_get(x, "key", k) && !object_get(x, "k", k) && !object_get(x, "name", k) && !object_get(x, "Name", k) && !object_get(x, "Key", k)) { k = value::null(); }
        if (!object_get(x, "value", v) && !object_get(x, "v", v) && !object_get(x, "Value", v)) { v = value::null(); }
        std::string key = k.k == K::Str ? std::string(get_str(k)) : to_json(k);
        auto it = std::find_if(r.begin(), r.end(), [&](const auto &kv) { return kv.first == key; });
        if (it != r.end()) { it->second = v; } else { r.emplace_back(std::move(key), v); }
      });
      out(value::object(std::move(r)));
      return;
    }
    if (name == "tojson") { out(value::string(to_json(in))); return; }
    if (name == "input_line_number") { out(value::number(0)); return; }
    if (name == "any" || name == "all") {
      bool want_any = name == "any";
      bool r = !want_any;
      for_each_elem(in, [&](const value &x) {
        if (want_any && truthy(x)) { r = true; }
        if (!want_any && !truthy(x)) { r = false; }
      });
      out(value::boolean(r));
      return;
    }
    if (name == "flatten") {
      std::vector<value> r;
      std::function<void(const value &)> rec = [&](const value &v) {
        for_each_elem(v, [&](const value &x) { if (x.k == K::Arr) { rec(x); } else { r.push_back(x); } });
      };
      rec(in);
      out(value::array(std::move(r)));
      return;
    }
    if (name == "error") { jq_error err(to_json(in)); err.payload = in; throw err; }
    if (name == "recurse") { recurse_node::rec(in, out); return; }
    if (name == "paths" || name == "leaf_paths") {
      std::vector<value> path;
      bool leaves = name == "leaf_paths";
      std::function<void(const value &)> rec = [&](const value &v) {
        auto visit = [&](value key, const value &x) {
          path.push_back(key);
          if (!leaves || (x.k != K::Arr && x.k != K::Obj)) { out(value::array(path)); }
          rec(x);
          path.pop_back();
        };
        if (v.k == K::Arr) { double i = 0; for_each_elem(v, [&](const value &x) { visit(value::number(i++), x); }); }
        else if (v.k == K::Obj) { for_each_field(v, [&](std::string_view k, const value &x) { visit(value::string(std::string(k)), x); }); }
      };
      rec(in);
      return;
    }
  } else if (n == 1) {
    if (name == "select") { arg1([&](const value &c) { if (truthy(c)) { out(in); } }); return; }
    if (name == "map") {
      if (in.k != K::Arr && in.k != K::Obj) { cannot_iterate(in); }
      std::vector<value> r;
      auto f = [&](const value &x) { args[0]->eval(x, e, [&](const value &y) { r.push_back(y); }); };
      if (in.k == K::Arr) { for_each_elem(in, f); } else { for_each_field(in, [&](std::string_view, const value &x) { f(x); }); }
      out(value::array(std::move(r)));
      return;
    }
    if (name == "map_values") {
      if (in.k == K::Arr) {
        std::vector<value> r;
        for_each_elem(in, [&](const value &x) {
          bool done = false;
          args[0]->eval(x, e, [&](const value &y) { if (!done) { r.push_back(y); done = true; } });
        });
        out(value::array(std::move(r)));
        return;
      }
      std::vector<std::pair<std::string, value>> r;
      for_each_field(in, [&](std::string_view k, const value &x) {
        bool done = false;
        args[0]->eval(x, e, [&](const value &y) { if (!done) { r.emplace_back(std::string(k), y); done = true; } });
      });
      out(value::object(std::move(r)));
      return;
    }
    if (name == "has") {
      arg1([&](const value &k) {
        if (in.k == K::Null) { out(value::boolean(false)); return; }
        if (in.k == K::Obj && k.k == K::Str) { value tmp; out(value::boolean(object_get(in, get_str(k), tmp))); return; }
        if (in.k == K::Arr && k.k == K::Num) { double d = get_num(k); out(value::boolean(d >= 0 && d < double(container_size(in)))); return; }
        throw jq_error(std::string("Cannot check whether ") + kind_name(in.k) + " has a " + kind_name(k.k) + " key");
      });
      return;
    }
    if (name == "contains") { arg1([&](const value &b) { out(value::boolean(contains(in, b))); }); return; }
    if (name == "inside") { arg1([&](const value &b) { out(value::boolean(contains(b, in))); }); return; }
    if (name == "startswith" || name == "endswith") {
      arg1([&](const value &b) {
        if (in.k != K::Str || b.k != K::Str) { throw jq_error(name + "() requires string inputs"); }
        std::string_view s = get_str(in), p = get_str(b);
        bool r = p.size() <= s.size() && (name == "startswith" ? s.substr(0, p.size()) == p : s.substr(s.size() - p.size()) == p);
        out(value::boolean(r));
      });
      return;
    }
    if (name == "ltrimstr" || name == "rtrimstr") {
      arg1([&](const value &b) {
        if (in.k != K::Str || b.k != K::Str) { out(in); return; }
        std::string_view s = get_str(in), p = get_str(b);
        if (name == "ltrimstr" && s.substr(0, p.size()) == p) { out(value::string(std::string(s.substr(p.size())))); return; }
        if (name == "rtrimstr" && p.size() <= s.size() && s.substr(s.size() - p.size()) == p) { out(value::string(std::string(s.substr(0, s.size() - p.size())))); return; }
        out(in);
      });
      return;
    }
    if (name == "split") { arg1([&](const value &b) { out(arith(op::div, in, b)); }); return; }
    if (name == "join") {
      arg1([&](const value &sep) {
        std::string r;
        bool first = true;
        for_each_elem(in, [&](const value &x) {
          if (!first) {
            // jq adds the separator with +: other kinds raise its error.
            if (sep.k == K::Str) { r.append(get_str(sep)); }
            else { (void)arith(op::add, value::string(r), sep); }
          }
          first = false;
          if (x.k == K::Null) { return; }
          if (x.k == K::Str) { r.append(get_str(x)); }
          else if (x.k == K::Num || x.k == K::True || x.k == K::False) { r.append(to_json(x)); }
          else { throw jq_error(std::string("Cannot join with ") + kind_name(x.k)); }
        });
        out(value::string(std::move(r)));
      });
      return;
    }
    if (name == "sort_by" || name == "group_by" || name == "unique_by" || name == "min_by" || name == "max_by") {
      require_kind(in, K::Arr, "cannot be sorted, as it is not an array");
      std::vector<std::pair<value, value>> kv;
      for_each_elem(in, [&](const value &x) { kv.emplace_back(collect(*args[0], x, e), x); });
      if (name == "sort_by") { out(sort_by_keys(kv)); return; }
      if (name == "min_by" || name == "max_by") {
        if (kv.empty()) { out(value::null()); return; }
        size_t best = 0;
        for (size_t i = 1; i < kv.size(); i++) {
          int c = compare(kv[i].first, kv[best].first);
          if (name == "max_by" ? c >= 0 : c < 0) { best = i; }
        }
        out(kv[best].second);
        return;
      }
      std::stable_sort(kv.begin(), kv.end(), [](const auto &x, const auto &y) { return compare(x.first, y.first) < 0; });
      std::vector<value> groups;
      std::vector<value> cur;
      for (size_t i = 0; i < kv.size(); i++) {
        if (i > 0 && compare(kv[i].first, kv[i - 1].first) != 0) {
          groups.push_back(name == "group_by" ? value::array(std::move(cur)) : cur[0]);
          cur.clear();
        }
        cur.push_back(kv[i].second);
      }
      if (!cur.empty()) { groups.push_back(name == "group_by" ? value::array(std::move(cur)) : cur[0]); }
      out(value::array(std::move(groups)));
      return;
    }
    if (name == "any" || name == "all") {
      bool want_any = name == "any";
      bool r = !want_any;
      for_each_elem(in, [&](const value &x) {
        args[0]->eval(x, e, [&](const value &y) {
          if (want_any && truthy(y)) { r = true; }
          if (!want_any && !truthy(y)) { r = false; }
        });
      });
      out(value::boolean(r));
      return;
    }
    if (name == "range") {
      arg1([&](const value &upto) {
        double u = get_num(upto);
        for (double i = 0; i < u; i++) {
          fuzz_tick();
          out(value::number(i));
        }
      });
      return;
    }
    if (name == "recurse") {
      // Each level recurses: bound it like nested values.
      uint32_t level = 0;
      std::function<void(const value &)> rec = [&](const value &v) {
        fuzz_tick();
        check_depth(++level);
        out(v);
        args[0]->eval(v, e, [&](const value &x) { rec(x); });
        level--;
      };
      rec(in);
      return;
    }
    if (name == "with_entries") {
      std::vector<std::pair<std::string, value>> r;
      for_each_field(in, [&](std::string_view k, const value &v) {
        value entry = value::object({{"key", value::string(std::string(k))}, {"value", v}});
        args[0]->eval(entry, e, [&](const value &y) {
          value nk, nv;
          if (!object_get(y, "key", nk)) { nk = value::null(); }
          if (!object_get(y, "value", nv)) { nv = value::null(); }
          std::string key = nk.k == K::Str ? std::string(get_str(nk)) : to_json(nk);
          auto it = std::find_if(r.begin(), r.end(), [&](const auto &kv) { return kv.first == key; });
          if (it != r.end()) { it->second = nv; } else { r.emplace_back(std::move(key), nv); }
        });
      });
      out(value::object(std::move(r)));
      return;
    }
    if (name == "first") {
      struct stop {};
      try { args[0]->eval(in, e, [&](const value &x) { out(x); throw stop{}; }); } catch (stop &) {}
      return;
    }
    if (name == "last") {
      value r; bool got = false;
      args[0]->eval(in, e, [&](const value &x) { r = x; got = true; });
      if (got) { out(r); }
      return;
    }
    if (name == "error") { arg1([&](const value &m) { jq_error err(m.k == K::Str ? std::string(get_str(m)) : to_json(m)); err.payload = m; throw err; }); return; }
    if (name == "test") {
      // Literal substring match only (no regular expressions in this prototype).
      arg1([&](const value &p) { out(value::boolean(get_str(in).find(get_str(p)) != std::string_view::npos)); });
      return;
    }
    if (name == "indices" || name == "index" || name == "rindex") {
      arg1([&](const value &p) {
        std::vector<value> r;
        if (in.k == K::Str) {
          std::string_view s = get_str(in), q = get_str(p);
          if (!q.empty()) {
            // Offsets are in code points.
            size_t cp = 0, counted = 0;
            for (size_t pos = s.find(q); pos != std::string_view::npos; pos = s.find(q, pos + 1)) {
              cp += utf8_count(s.substr(counted, pos - counted));
              counted = pos;
              r.push_back(value::number(double(cp)));
            }
          }
        } else if (in.k == K::Arr) {
          std::vector<value> v = to_vec(in);
          if (p.k == K::Arr) {
            // A subarray: where its elements occur in sequence.
            std::vector<value> q = to_vec(p);
            if (!q.empty()) {
              for (size_t i = 0; i + q.size() <= v.size(); i++) {
                size_t j = 0;
                while (j < q.size() && compare(v[i + j], q[j]) == 0) { j++; }
                if (j == q.size()) { r.push_back(value::number(double(i))); }
              }
            }
          } else {
            for (size_t i = 0; i < v.size(); i++) {
              if (compare(v[i], p) == 0) { r.push_back(value::number(double(i))); }
            }
          }
        }
        if (name == "indices") { out(value::array(std::move(r))); }
        else if (r.empty()) { out(value::null()); }
        else { out(name == "index" ? r.front() : r.back()); }
      });
      return;
    }
    if (name == "del") {
      // del(.k) and del(.[n]) for simple paths only.
      throw jq_error("del/1 is not supported in this prototype");
    }
  } else if (n == 2) {
    if (name == "limit") {
      args[0]->eval(in, e, [&](const value &lim) {
        int64_t l = to_int64(get_num(lim));
        if (l <= 0) { return; }
        int64_t count = 0;
        struct stop {};
        try {
          args[1]->eval(in, e, [&](const value &x) { out(x); if (++count >= l) { throw stop{}; } });
        } catch (stop &) {}
      });
      return;
    }
    if (name == "range") {
      args[0]->eval(in, e, [&](const value &from) {
        args[1]->eval(in, e, [&](const value &upto) {
          for (double i = get_num(from); i < get_num(upto); i++) {
            fuzz_tick();
            out(value::number(i));
          }
        });
      });
      return;
    }
  }
  throw jq_error(name + "/" + std::to_string(n) + " is not defined");
}

} // namespace sjq
