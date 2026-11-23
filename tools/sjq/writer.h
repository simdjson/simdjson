#ifndef SJQ_WRITER_H
#define SJQ_WRITER_H

#include "value.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
// SSE2 is part of x86-64, so it needs no compiler flag or runtime check.
#define SJQ_SSE2 1
#include <emmintrin.h>
#endif

namespace sjq {

// Color slots, in the order of JQ_COLORS.
enum color_slot { C_NULL, C_FALSE, C_TRUE, C_NUM, C_STR, C_ARR, C_OBJ, C_KEY, C_COUNT };

struct writer {
  std::string buf{};
  FILE *out{stdout};
  bool pretty{true};  // false: compact (-c)
  int indent{2};
  bool tab{false};
  bool sort_keys{false};
  bool to_memory{false};
  bool color{false};
  const std::string *colors{nullptr};  // C_COUNT escape sequences when color is on

  writer() { buf.reserve(1 << 20); }
  writer(const writer &) = delete;
  writer &operator=(const writer &) = delete;
  void copy_format(const writer &o) {
    pretty = o.pretty;
    indent = o.indent;
    tab = o.tab;
    sort_keys = o.sort_keys;
    color = o.color;
    colors = o.colors;
  }
  void flush() {
    if (!buf.empty()) { fwrite(buf.data(), 1, buf.size(), out); buf.clear(); }
    fflush(out);
  }
  void maybe_flush() { if (!to_memory && buf.size() >= (1 << 20)) { flush(); } }

  void put(char c) { buf.push_back(c); }
  void put(std::string_view s) { buf.append(s.data(), s.size()); }

  void newline_indent(int depth) {
    buf.push_back('\n');
    if (tab) { buf.append(size_t(depth), '\t'); }
    else { buf.append(size_t(depth) * size_t(indent), ' '); }
  }

  // Colored tokens, laid out as jq 1.8 does: each token is wrapped in its
  // color and a reset; indentation and the space after ':' are not.
  void cbegin(color_slot c) { if (color) { put(colors[c]); } }
  void cend() { if (color) { put("\x1b[0m"); } }
  void open(color_slot c, char ch) { cbegin(c); put(ch); cend(); }
  void empty(color_slot c, std::string_view s) { cbegin(c); put(s); cend(); }
  void comma(color_slot c) { cbegin(c); put(','); cend(); }
  void colon() {
    cbegin(C_OBJ);
    put(':');
    cend();
    if (pretty) { put(' '); }
  }
  void key(std::string_view k) { cbegin(C_KEY); write_string(k); cend(); }
  void string_value(std::string_view s) { cbegin(C_STR); write_string(s); cend(); }
  void atom(color_slot c, std::string_view text) { cbegin(c); put(text); cend(); }
  void number_value(double d) { cbegin(C_NUM); write_double(d); cend(); }

  static inline bool needs_escape(uint8_t c) { return c < 0x20 || c == '"' || c == '\\' || c == 0x7f; }

  void write_string(std::string_view s) {
    buf.push_back('"');
    const char *p = s.data();
    const char *end = p + s.size();
    while (p < end) {
      const char *run = p;
#if defined(__ARM_NEON)
      // 16 bytes at a time: any byte < 0x20, '"', '\\' or 0x7f stops the scan.
      while (end - p >= 16) {
        uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
        uint8x16_t m = vorrq_u8(vorrq_u8(vcltq_u8(v, vdupq_n_u8(0x20)), vceqq_u8(v, vdupq_n_u8('"'))),
                                vorrq_u8(vceqq_u8(v, vdupq_n_u8('\\')), vceqq_u8(v, vdupq_n_u8(0x7f))));
        if (vmaxvq_u8(m)) { break; }
        p += 16;
      }
#elif defined(SJQ_SSE2)
      // Bytes below 0x20 are those equal to their minimum with 0x1f.
      while (end - p >= 16) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
        __m128i m = _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(_mm_min_epu8(v, _mm_set1_epi8(0x1f)), v), _mm_cmpeq_epi8(v, _mm_set1_epi8('"'))),
                                 _mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\')), _mm_cmpeq_epi8(v, _mm_set1_epi8(0x7f))));
        if (_mm_movemask_epi8(m)) { break; }
        p += 16;
      }
#endif
      // SWAR scan: 8 bytes at a time for any byte needing escape.
      while (end - p >= 8) {
        uint64_t w;
        memcpy(&w, p, 8);
        const uint64_t ones = 0x0101010101010101ULL, high = 0x8080808080808080ULL;
        uint64_t lt20 = (w - ones * 0x20) & ~w;
        uint64_t q = w ^ (ones * '"');
        uint64_t bs = w ^ (ones * '\\');
        uint64_t del = w ^ (ones * 0x7f);
        uint64_t hit = (lt20 | ((q - ones) & ~q) | ((bs - ones) & ~bs) | ((del - ones) & ~del)) & high;
        if (hit) { break; }
        p += 8;
      }
      while (p < end && !needs_escape(uint8_t(*p))) { p++; }
      buf.append(run, size_t(p - run));
      if (p == end) { break; }
      uint8_t c = uint8_t(*p++);
      switch (c) {
      case '"': put("\\\""); break;
      case '\\': put("\\\\"); break;
      case '\n': put("\\n"); break;
      case '\t': put("\\t"); break;
      case '\r': put("\\r"); break;
      case '\b': put("\\b"); break;
      case '\f': put("\\f"); break;
      default: {
        char tmp[7];
        snprintf(tmp, sizeof(tmp), "\\u%04x", c);
        put(std::string_view(tmp, 6));
      }
      }
    }
    buf.push_back('"');
  }

  // Formats like jq for computed numbers: shortest round-trip digits;
  // exponential notation when decpt <= -4 or decpt > ndigits + 15, where the
  // value is 0.DIGITS * 10^decpt (as in jq's jvp_dtoa_fmt).
  void write_double(double d) {
    if (std::isnan(d)) { put("null"); return; }
    if (std::isinf(d)) { d = d > 0 ? 1.7976931348623157e308 : -1.7976931348623157e308; }
    if (d == std::floor(d) && std::fabs(d) < 1e15) {
      char tmp[32];
      auto r = std::to_chars(tmp, tmp + sizeof(tmp), int64_t(d));
      if (d == 0 && std::signbit(d)) { put('-'); }
      put(std::string_view(tmp, size_t(r.ptr - tmp)));
      return;
    }
    char tmp[simdjson::internal::to_chars_buffer_size];
    char *e = simdjson::internal::to_chars(tmp, tmp + sizeof(tmp), d);
    std::string_view s(tmp, size_t(e - tmp));
    double ad = std::fabs(d);
    if (ad >= 1e-4 && ad < 1e15) {
      // simdjson's shortest formatter uses the same notation as jq here.
      if (s.size() >= 2 && s.substr(s.size() - 2) == ".0") { s.remove_suffix(2); }
      put(s);
      return;
    }
    // Otherwise take the digits and the exponent from simdjson's output
    // (d.ddde+XX) and lay them out as jq does.
    bool neg = s[0] == '-';
    if (neg) { s.remove_prefix(1); }
    size_t epos = s.find('e');
    std::string_view mant = s.substr(0, epos);
    int exp10 = 0;
    if (epos != std::string_view::npos) {
      size_t i = epos + 1;
      bool eneg = false;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) { eneg = s[i] == '-'; i++; }
      for (; i < s.size(); i++) { exp10 = exp10 * 10 + (s[i] - '0'); }
      if (eneg) { exp10 = -exp10; }
    }
    size_t dot = mant.find('.');
    int decpt = int(dot == std::string_view::npos ? mant.size() : dot) + exp10;
    char digits[32];
    int nd = 0;
    for (char c : mant) { if (c != '.' && nd < 32) { digits[nd++] = c; } }
    int lead = 0;
    while (lead < nd - 1 && digits[lead] == '0') { lead++; decpt--; }
    while (nd - 1 > lead && digits[nd - 1] == '0') { nd--; }
    std::string_view dv(digits + lead, size_t(nd - lead));
    nd = int(dv.size());
    if (neg) { put('-'); }
    if (decpt <= -4 || decpt > nd + 15) {
      int x = decpt - 1;
      put(dv[0]);
      if (nd > 1) { put('.'); put(dv.substr(1)); }
      char t[16];
      int n = snprintf(t, sizeof(t), "e%c%02d", x < 0 ? '-' : '+', std::abs(x));
      put(std::string_view(t, size_t(n)));
    } else if (decpt <= 0) {
      put("0.");
      buf.append(size_t(-decpt), '0');
      put(dv);
    } else if (nd <= decpt) {
      put(dv);
      buf.append(size_t(decpt - nd), '0');
    } else {
      put(dv.substr(0, size_t(decpt)));
      put('.');
      put(dv.substr(size_t(decpt)));
    }
  }

  void write_dom_number(dom::element el) {
    char tmp[32];
    cbegin(C_NUM);
    switch (el.type()) {
    case dom::element_type::INT64: {
      int64_t x = 0; must_get(el.get_int64(), x);
      auto r = std::to_chars(tmp, tmp + sizeof(tmp), x);
      put(std::string_view(tmp, size_t(r.ptr - tmp)));
      break;
    }
    case dom::element_type::UINT64: {
      uint64_t x = 0; must_get(el.get_uint64(), x);
      auto r = std::to_chars(tmp, tmp + sizeof(tmp), x);
      put(std::string_view(tmp, size_t(r.ptr - tmp)));
      break;
    }
    default: {
      double d = 0; must_get(el.get_double(), d);
      write_double(d);
    }
    }
    cend();
  }

  void write_dom(dom::element el, int depth) {
    check_stack();
    switch (el.type()) {
    case dom::element_type::ARRAY: {
      dom::array a; must_get(el.get_array(), a);
      auto it = a.begin(), e = a.end();
      if (it == e) { empty(C_ARR, "[]"); return; }
      open(C_ARR, '[');
      bool first = true;
      for (; it != e; ++it) {
        if (!first) { comma(C_ARR); }
        first = false;
        if (pretty) { newline_indent(depth + 1); }
        write_dom(*it, depth + 1);
      }
      if (pretty) { newline_indent(depth); }
      open(C_ARR, ']');
      return;
    }
    case dom::element_type::OBJECT: {
      dom::object o; must_get(el.get_object(), o);
      auto it = o.begin(), e = o.end();
      if (it == e) { empty(C_OBJ, "{}"); return; }
      if (sort_keys) { write_value(value::of(el), depth); return; }
      open(C_OBJ, '{');
      bool first = true;
      for (; it != e; ++it) {
        if (!first) { comma(C_OBJ); }
        first = false;
        if (pretty) { newline_indent(depth + 1); }
        key(it.key());
        colon();
        write_dom(it.value(), depth + 1);
      }
      if (pretty) { newline_indent(depth); }
      open(C_OBJ, '}');
      return;
    }
    case dom::element_type::STRING: {
      std::string_view str; must_get(el.get_string(), str);
      string_value(str);
      return;
    }
    case dom::element_type::NULL_VALUE: atom(C_NULL, "null"); return;
    case dom::element_type::BOOL: {
      bool b = false; must_get(el.get_bool(), b);
      if (b) { atom(C_TRUE, "true"); } else { atom(C_FALSE, "false"); }
      return;
    }
    default: write_dom_number(el); return;
    }
  }

  void write_value(const value &v, int depth) {
    check_stack();
    if (v.dom && !(sort_keys && v.k == K::Obj)) { write_dom(v.e, depth); return; }
    switch (v.k) {
    case K::Null: atom(C_NULL, "null"); return;
    case K::False: atom(C_FALSE, "false"); return;
    case K::True: atom(C_TRUE, "true"); return;
    case K::Num: number_value(v.num); return;
    case K::Str: string_value(v.h->s); return;
    case K::Arr: {
      if (v.h->a.empty()) { empty(C_ARR, "[]"); return; }
      open(C_ARR, '[');
      bool first = true;
      for (const value &x : v.h->a) {
        if (!first) { comma(C_ARR); }
        first = false;
        if (pretty) { newline_indent(depth + 1); }
        write_value(x, depth + 1);
      }
      if (pretty) { newline_indent(depth); }
      open(C_ARR, ']');
      return;
    }
    case K::Obj: {
      auto fields = sort_keys ? sorted_fields(v) : v.h->o;
      if (fields.empty()) { empty(C_OBJ, "{}"); return; }
      open(C_OBJ, '{');
      bool first = true;
      for (const auto &kv : fields) {
        if (!first) { comma(C_OBJ); }
        first = false;
        if (pretty) { newline_indent(depth + 1); }
        key(kv.first);
        colon();
        write_value(kv.second, depth + 1);
      }
      if (pretty) { newline_indent(depth); }
      open(C_OBJ, '}');
      return;
    }
    }
  }
};

inline std::string to_json(const value &v) {
  writer w{};
  w.pretty = false;
  w.write_value(v, 0);
  return std::move(w.buf);
}

} // namespace sjq

#endif // SJQ_WRITER_H
