#!/usr/bin/env python3
# Structured seed corpus and dictionary generator for fuzz_fractured_json.
#
# The FracturedJson formatter picks a layout strategy (inline, compact
# multiline, table, expanded) based on document shape and options; random
# mutation of plain JSON rarely reaches the table/padding code paths with
# the right option combinations, so each seed below pairs a structurally
# valid document with option bytes crafted to steer the formatter into one
# strategy (see fuzz_fractured_json.cpp make_options for the byte layout:
# 15 option bytes, then the document).
#
# Usage: gen_fractured_json_corpus.py <output-basename>
# Writes <output-basename>.zip (seed corpus) and <output-basename>.dict.

import sys
import zipfile

# --- option byte encoding (mirror of make_options in the harness) ----------

def opts_bytes(max_total_line_length=120,
               max_inline_complexity=2,
               max_compact_array_complexity=2,
               indent_spaces=4,
               always_expand_depth=-1,
               enable_table_format=True,
               max_table_row_complexity=2,
               enable_compact_multiline=True,
               min_compact_array_row_items=3,
               simple_bracket_padding=False,
               nested_bracket_padding=True,
               colon_padding=True,
               comma_padding=True,
               comma_placement=2,  # before_padding_except_numbers
               number_alignment=0):  # left
    return bytes([
        max(0, min(255, (max_total_line_length - 1) // 16)),
        max_inline_complexity % 9,
        max_compact_array_complexity % 9,
        indent_spaces % 33,
        (always_expand_depth + 1) % 10,
        1 if enable_table_format else 0,
        max_table_row_complexity % 9,
        1 if enable_compact_multiline else 0,
        max(0, min(63, min_compact_array_row_items - 1)),
        1 if simple_bracket_padding else 0,
        1 if nested_bracket_padding else 0,
        1 if colon_padding else 0,
        1 if comma_padding else 0,
        comma_placement % 3,
        1 if number_alignment else 0,
    ])

# --- seeds: (name, option overrides, document text) ------------------------

USERS = ('[{"id":1,"name":"Alice","active":true,"score":10.5},'
         '{"id":2,"name":"Bob","active":false,"score":9.25},'
         '{"id":3,"name":"Carol","active":true,"score":12.0}]')

SEEDS = [
    ("users-table", dict(enable_table_format=True, max_table_row_complexity=2,
                         max_inline_complexity=1), USERS),
    ("users-table-comma-after", dict(enable_table_format=True,
                                     comma_placement=1), USERS),
    ("users-table-no-padding", dict(enable_table_format=True,
                                    simple_bracket_padding=True,
                                    nested_bracket_padding=False,
                                    colon_padding=False,
                                    comma_padding=False), USERS),
    ("numbers-right-align", dict(number_alignment=1, enable_table_format=True),
     '[123, 4.5, -6789, 1e10, 0.125, 9223372036854775807]'),
    ("compact-multiline", dict(min_compact_array_row_items=1,
                               max_total_line_length=81,
                               enable_compact_multiline=True,
                               max_compact_array_complexity=0),
     '[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]'),
    ("inline-object", dict(max_inline_complexity=4,
                           max_compact_array_complexity=4),
     '{"name":"x","vals":[1,2,3],"meta":{"ok":true}}'),
    ("expanded-deep", dict(always_expand_depth=1),
     '{"a":{"b":{"c":{"d":{"e":[1,2,{"f":null}]}}}}}'),
    ("missing-keys-table", dict(enable_table_format=True,
                                max_table_row_complexity=1),
     '[{"a":1,"b":2},{"a":3,"c":4},{"b":5,"d":6}]'),
    ("long-strings", dict(max_total_line_length=60),
     '{"s":"' + "A" * 200 + '","t":"line1\\nline2\\tquote\\" end","u":""}'),
    ("unicode", dict(indent_spaces=2),
     '{"ключ":"значение","emoji":"😀🎉","cjk":"日本語のテキスト"}'),
    ("empty-containers", dict(max_inline_complexity=0),
     '[[], {}, "", {"":null}, [null,false,true]]'),
    ("mixed-scalars", dict(enable_compact_multiline=True,
                           min_compact_array_row_items=2),
     '[null,true,false,0,-1,2.5,"s","",{}]'),
    ("nested-arrays-table", dict(enable_table_format=True,
                                 max_table_row_complexity=3),
     '[[[1,2],[3,4]],[[5,6],[7,8]],[[9,10],[11,12]]]'),
    ("root-scalar", dict(), '42'),
    ("number-extremes", dict(max_total_line_length=160),
     '[9223372036854775807,-9223372036854775808,18446744073709551615,'
     '1e308,-1e-308,0.1,-0.0,1e15,1E+2,5e-1]'),
    ("wide-object", dict(enable_table_format=True, max_inline_complexity=1),
     '{' + ','.join('"k%d":%s' % (i, v) for i, v in enumerate(
         ["1", "2.5", '"x"', "true", "null", "[]"] * 4)) + '}'),
]

# --- dictionary ------------------------------------------------------------

# Byte-level escaping: libFuzzer dict strings need backslash and quote
# escaped, and non-ASCII bytes as \xNN (built from byte values, so this
# file contains no backslash literals).
BS = 0x5C  # backslash
DQ = 0x22  # double quote


def _s(text):
    return text.encode("utf-8")


DICT_VALUES = [
    _s("{"), _s("}"), _s("["), _s("]"), _s(","), _s(":"), _s(": "),
    _s("null"), _s("true"), _s("false"),
    _s('"a"'), _s('"k0"'), _s('""'),
    _s("0"), _s("-0.0"), _s("1"), _s("-1"), _s("1.5"), _s("1e10"),
    _s("1e308"), _s("-1e-308"),
    _s("9223372036854775807"), _s("-9223372036854775808"),
    _s("18446744073709551615"),
    bytes([DQ, BS, 0x6E, DQ]),                       # "\n" as two chars
    bytes([DQ, BS, 0x74, DQ]),                       # "\t"
    bytes([DQ, BS, DQ, DQ]),                         # "\"" escaped quote
    bytes([DQ, BS, 0x75, 0x30, 0x30, 0x30, 0x30, DQ]),  # "\u0000"
    bytes([DQ, 0xF0, 0x9F, 0x98, 0x80, DQ]),         # quoted emoji (UTF-8)
    bytes([DQ] + [0x41] * 64 + [DQ]),                # quoted 64-char run
]


def dict_line(name, value):
    out = []
    for b in value:
        if b == BS:
            out.append(chr(BS) + chr(BS))
        elif b == DQ:
            out.append(chr(BS) + chr(DQ))
        elif 0x20 <= b < 0x7F:
            out.append(chr(b))
        else:
            out.append(chr(BS) + "x%02x" % b)
    return '%s="%s"' % (name, "".join(out))


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: gen_fractured_json_corpus.py <output-basename>")
    base = sys.argv[1]
    with zipfile.ZipFile(base + ".zip", "w", zipfile.ZIP_DEFLATED) as z:
        for name, opt_overrides, doc in SEEDS:
            z.writestr(name + ".txt",
                       opts_bytes(**opt_overrides) + doc.encode("utf-8"))
    with open(base + ".dict", "w", encoding="utf-8") as f:
        for i, value in enumerate(DICT_VALUES):
            f.write(dict_line("kw%d" % i, value) + "\n")
    print("wrote %s.zip (%d seeds) and %s.dict (%d entries)"
          % (base, len(SEEDS), base, len(DICT_VALUES)))


if __name__ == "__main__":
    main()
