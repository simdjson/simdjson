#ifndef SJQ_VALUE_H
#define SJQ_VALUE_H

#include "simdjson.h"

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sjq {

using namespace simdjson;

// Unpacks a simdjson result whose type was checked before, so that it cannot
// fail.
template <class R, class T> inline void must_get(R &&r, T &out) {
  if (std::forward<R>(r).get(out)) { throw std::logic_error("sjq: internal error: unexpected JSON type"); }
}

enum class K : uint8_t { Null, False, True, Num, Str, Arr, Obj };

struct value;
struct heap;
using heap_ptr = std::shared_ptr<const heap>;

struct value {
  K k{K::Null};
  bool dom{false};      // true: e holds the value (any kind)
  dom::element e{};     // DOM-backed value
  double num{0};        // owned number
  heap_ptr h{};         // owned string/array/object

  static value null() { return value(); }
  static value boolean(bool b) { value v; v.k = b ? K::True : K::False; return v; }
  static value number(double d) { value v; v.k = K::Num; v.num = d; return v; }
  static value of(dom::element el);
  static value string(std::string s);
  static value array(std::vector<value> a);
  static value object(std::vector<std::pair<std::string, value>> o);
};

// Values built by filters can nest without bound, while the algorithms on
// them (output, comparison, ..) recurse once per level. Their nesting depth
// is recorded when they are built and capped: g_max_depth is lower on worker
// threads, whose stacks are small (a worker that exceeds it fails its chunk,
// which is then redone on the main thread).
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
// Sanitizers make stack frames several times larger.
inline thread_local uint32_t g_max_depth = 1000;
#else
inline thread_local uint32_t g_max_depth = 10000;
#endif

struct heap {
  std::string s{};
  std::vector<value> a{};
  std::vector<std::pair<std::string, value>> o{};
  uint32_t depth{0};  // levels of arrays and objects built by filters

  heap() = default;
  heap(const heap &) = delete;
  heap &operator=(const heap &) = delete;
  // Frees nested children with a worklist: the default destructor would
  // recurse once per level.
  ~heap();
};

struct jq_error : std::runtime_error {
  value payload{};
  explicit jq_error(const std::string &m) : std::runtime_error(m) {}
};
struct break_signal { int label; };

// Evaluation recurses through nested filters (recurse(f), generators) and
// nested values. Each thread records where its stack starts and how much of
// it sjq may use; the recursive entry points call check_stack(), which raises
// a jq error before the stack overflows, whatever the frame sizes of the
// build (sanitizers make them much larger).
inline thread_local uintptr_t g_stack_base = 0;
inline thread_local size_t g_stack_budget = 0;

#if defined(_MSC_VER) && !defined(__clang__)
#define SJQ_FRAME_ADDRESS() reinterpret_cast<uintptr_t>(_AddressOfReturnAddress())
#else
#define SJQ_FRAME_ADDRESS() reinterpret_cast<uintptr_t>(__builtin_frame_address(0))
#endif

inline void stack_guard_init(size_t budget) {
  g_stack_base = SJQ_FRAME_ADDRESS();
  g_stack_budget = budget;
}
inline void check_stack() {
  uintptr_t f = SJQ_FRAME_ADDRESS();
  if (g_stack_budget && f < g_stack_base && g_stack_base - f > g_stack_budget) {
    throw jq_error("stack exhausted: the filter or the value is nested too deeply");
  }
}

inline uint32_t child_depth(const value &v) {
  return v.h && (v.k == K::Arr || v.k == K::Obj) ? v.h->depth : 0;
}
inline void check_depth(uint32_t d) {
  if (d > g_max_depth) { throw jq_error("value nested too deeply (more than " + std::to_string(g_max_depth) + " levels)"); }
}

inline heap::~heap() {
  std::vector<heap_ptr> pending;
  auto take = [&pending](heap &h) {
    for (value &v : h.a) {
      if (v.h && v.h.use_count() == 1) { pending.push_back(std::move(v.h)); }
    }
    for (auto &kv : h.o) {
      if (kv.second.h && kv.second.h.use_count() == 1) { pending.push_back(std::move(kv.second.h)); }
    }
  };
  take(*this);
  while (!pending.empty()) {
    heap_ptr p = std::move(pending.back());
    pending.pop_back();
    // Sole owner: its children can be detached before it is freed.
    take(const_cast<heap &>(*p));
  }
}

inline K kind_of(dom::element el) {
  switch (el.type()) {
  case dom::element_type::ARRAY: return K::Arr;
  case dom::element_type::OBJECT: return K::Obj;
  case dom::element_type::STRING: return K::Str;
  case dom::element_type::NULL_VALUE: return K::Null;
  case dom::element_type::BOOL: {
    bool b = false;
    must_get(el.get_bool(), b);
    return b ? K::True : K::False;
  }
  default: return K::Num;
  }
}

inline value value::of(dom::element el) {
  value v{};
  v.k = kind_of(el);
  v.dom = true;
  v.e = el;
  return v;
}
inline value value::string(std::string s) {
  auto p = std::make_shared<heap>();
  p->s = std::move(s);
  value v; v.k = K::Str; v.h = std::move(p); return v;
}
inline value value::array(std::vector<value> a) {
  uint32_t d = 0;
  for (const value &x : a) { d = std::max(d, child_depth(x)); }
  check_depth(d + 1);
  auto p = std::make_shared<heap>();
  p->depth = d + 1;
  p->a = std::move(a);
  value v; v.k = K::Arr; v.h = std::move(p); return v;
}
inline value value::object(std::vector<std::pair<std::string, value>> o) {
  uint32_t d = 0;
  for (const auto &kv : o) { d = std::max(d, child_depth(kv.second)); }
  check_depth(d + 1);
  auto p = std::make_shared<heap>();
  p->depth = d + 1;
  p->o = std::move(o);
  value v; v.k = K::Obj; v.h = std::move(p); return v;
}

// Converts a double to int64_t as jq does: saturating, NaN as 0 (a plain cast
// of an out-of-range double is undefined behavior).
inline int64_t to_int64(double d) {
  if (std::isnan(d)) { return 0; }
  if (d >= 9223372036854775807.0) { return INT64_MAX; }
  if (d <= -9223372036854775807.0) { return -INT64_MAX; }
  return int64_t(d);
}

inline const char *kind_name(K k) {
  switch (k) {
  case K::Null: return "null";
  case K::False: case K::True: return "boolean";
  case K::Num: return "number";
  case K::Str: return "string";
  case K::Arr: return "array";
  case K::Obj: return "object";
  }
  return "?";
}

inline bool truthy(const value &v) { return !(v.k == K::Null || v.k == K::False); }

// The number or string in a value. Callers check the kind first; a wrong
// kind raises an error instead of reading the wrong member.
inline double get_num(const value &v) {
  if (v.k != K::Num) { throw jq_error(std::string(kind_name(v.k)) + " is not a number"); }
  if (!v.dom) { return v.num; }
  double d = 0;
  must_get(v.e.get_double(), d);
  return d;
}
inline std::string_view get_str(const value &v) {
  if (v.k != K::Str) { throw jq_error(std::string(kind_name(v.k)) + " is not a string"); }
  if (!v.dom) { return v.h->s; }
  std::string_view s;
  must_get(v.e.get_string(), s);
  return s;
}

// Strings are valid UTF-8: a code point starts at every byte that is not a
// continuation byte.
inline size_t utf8_count(std::string_view s) {
  size_t n = 0;
  for (unsigned char c : s) { n += (c & 0xC0) != 0x80; }
  return n;
}
// The byte offset of code point cp, or s.size() past the end.
inline size_t utf8_offset(std::string_view s, size_t cp) {
  size_t i = 0;
  for (; i < s.size(); i++) {
    if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
      if (cp == 0) { return i; }
      cp--;
    }
  }
  return i;
}

// Uniform iteration over arrays and objects. Other values raise jq's error.
// The value as jq 1.8 shows it in error messages: its compact JSON, cut to
// 25 bytes followed by "..." when longer than 28.
std::string error_text(const value &v);

[[noreturn]] inline void cannot_iterate(const value &v) {
  throw jq_error(std::string("Cannot iterate over ") + kind_name(v.k) + " (" + error_text(v) + ")");
}
template <class F> inline void for_each_elem(const value &v, F &&f) {
  if (v.k != K::Arr) { cannot_iterate(v); }
  if (v.dom) {
    dom::array a;
    must_get(v.e.get_array(), a);
    for (dom::element x : a) { f(value::of(x)); }
  } else {
    for (const value &x : v.h->a) { f(x); }
  }
}
template <class F> inline void for_each_field(const value &v, F &&f) {
  if (v.k != K::Obj) { cannot_iterate(v); }
  if (v.dom) {
    dom::object o;
    must_get(v.e.get_object(), o);
    for (dom::key_value_pair kv : o) { f(kv.key, value::of(kv.value)); }
  } else {
    for (const auto &kv : v.h->o) { f(std::string_view(kv.first), kv.second); }
  }
}
inline size_t container_size(const value &v) {
  if (v.k != K::Arr && v.k != K::Obj) { cannot_iterate(v); }
  if (v.dom) {
    if (v.k == K::Arr) { dom::array a; must_get(v.e.get_array(), a); return a.size(); }
    dom::object o; must_get(v.e.get_object(), o); return o.size();
  }
  return v.k == K::Arr ? v.h->a.size() : v.h->o.size();
}
inline bool object_get(const value &v, std::string_view key, value &out) {
  if (v.dom) {
    dom::object o;
    must_get(v.e.get_object(), o);
    dom::element x;
    if (o.at_key(key).get(x)) { return false; }
    out = value::of(x);
    return true;
  }
  // last occurrence wins, consistent with how owned objects are built (keys unique)
  for (const auto &kv : v.h->o) {
    if (kv.first == key) { out = kv.second; return true; }
  }
  return false;
}
inline bool array_get(const value &v, int64_t idx, value &out) {
  int64_t n = int64_t(container_size(v));
  if (idx < 0) { idx += n; }
  if (idx < 0 || idx >= n) { return false; }
  if (v.dom) {
    dom::array a;
    must_get(v.e.get_array(), a);
    dom::element x;
    if (a.at(size_t(idx)).get(x)) { return false; }
    out = value::of(x);
    return true;
  }
  out = v.h->a[size_t(idx)];
  return true;
}
inline std::vector<value> to_vec(const value &v) {
  std::vector<value> r{};
  for_each_elem(v, [&](const value &x) { r.push_back(x); });
  return r;
}
inline std::vector<std::pair<std::string, value>> to_fields(const value &v) {
  std::vector<std::pair<std::string, value>> r{};
  for_each_field(v, [&](std::string_view k, const value &x) { r.emplace_back(std::string(k), x); });
  return r;
}

inline int kind_rank(K k) {
  switch (k) {
  case K::Null: return 0;
  case K::False: return 1;
  case K::True: return 2;
  case K::Num: return 3;
  case K::Str: return 4;
  case K::Arr: return 5;
  case K::Obj: return 6;
  }
  return 7;
}

inline int compare(const value &a, const value &b);

inline std::vector<std::pair<std::string, value>> sorted_fields(const value &v) {
  auto f = to_fields(v);
  std::stable_sort(f.begin(), f.end(), [](const auto &x, const auto &y) { return x.first < y.first; });
  return f;
}

inline int compare(const value &a, const value &b) {
  check_stack();
  int ra = kind_rank(a.k), rb = kind_rank(b.k);
  if (ra != rb) { return ra < rb ? -1 : 1; }
  switch (a.k) {
  case K::Num: {
    double x = get_num(a), y = get_num(b);
    return x < y ? -1 : (x > y ? 1 : 0);
  }
  case K::Str: {
    int c = get_str(a).compare(get_str(b));
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
  }
  case K::Arr: {
    auto x = to_vec(a), y = to_vec(b);
    for (size_t i = 0; i < x.size() && i < y.size(); i++) {
      int c = compare(x[i], y[i]);
      if (c) { return c; }
    }
    return x.size() < y.size() ? -1 : (x.size() > y.size() ? 1 : 0);
  }
  case K::Obj: {
    auto x = sorted_fields(a), y = sorted_fields(b);
    // jq compares the sorted key sets first, then the values.
    for (size_t i = 0; i < x.size() && i < y.size(); i++) {
      int c = x[i].first.compare(y[i].first);
      if (c) { return c < 0 ? -1 : 1; }
    }
    if (x.size() != y.size()) { return x.size() < y.size() ? -1 : 1; }
    for (size_t i = 0; i < x.size(); i++) {
      int c = compare(x[i].second, y[i].second);
      if (c) { return c; }
    }
    return 0;
  }
  default: return 0;
  }
}

} // namespace sjq

#endif // SJQ_VALUE_H
