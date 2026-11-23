#!/usr/bin/env python3
"""Writes cases/*.test from the specs below, with outputs recorded from jq.

Usage: make_golden.py --jq PATH_TO_JQ_1.8 [--data DIR]

Each spec is (flags, program, input); an input of the form "@name" reads the
file name from --data instead of a line of standard input. Only stdout and the
exit status are recorded: sjq follows jq 1.7 for error wording.
"""

import argparse
import os
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))

LIT = '{"a": 1.50, "b": 1E2, "c": -0, "d": 0.0000001, "e": 12.5e1, "f": 100000000000000000000000000001, "g": [1.000, 2.0]}'
STR = '{"s": "tab\\there \\"q\\" \\\\ \\u0001 \\u007f \\u00e9 \\ud83d\\ude00 / \\/", "k\\u00e9y": "v"}'

SPECS = {
    "options": [
        ([], ".", '{"a":[1,{"b":2}],"c":{},"d":[]}'),
        (["-c"], ".", '{"a":[1,{"b":2}],"c":{},"d":[]}'),
        (["--tab"], ".", '{"a":[1,{"b":2}]}'),
        (["--indent", "0"], ".", '{"a":[1,{"b":2}]}'),
        (["--indent", "3"], ".", '{"a":[1,{"b":2}]}'),
        (["--indent", "7"], ".", '{"a":[1]}'),
        (["--indent", "-1"], ".", '{"a":[1]}'),
        (["-r"], ".[]", '["x", 1, "y\\nz", null]'),
        (["-S", "-c"], ".", '{"b":1,"a":{"d":2,"c":3}}'),
        (["-n"], "1, 2", "null"),
        (["-nc"], "[1, {\"a\": null}]", "null"),
        (["-rc"], ".a", '{"a": "s"} {"a": [1]}'),
        (["--seq", "-c"], ".a", '\x1e{"a":1}'),
        (["-c"], ".", '1 "two" [3] {"four":4} null true'),
    ],
    "literals": [
        (["-c"], ".", LIT),
        ([], ".", LIT),
        (["-c"], ".a, .b, .c, .d, .e, .f, .g", LIT),
        (["-c"], "{x: .a, y: [.b, .f]}", LIT),
        (["-c"], "select(.a > 1) | .g", LIT),
        (["-c"], ".", "1.50"),
        (["-c"], ".", "100000000000000000000000000001"),
        (["-c"], ".", STR),
        (["-r"], ".s", STR),
        (["-c"], "[1.50, 1e3, 0.1]", "null"),
        (["-c"], "0.1 + 0.2, 1 / 3, 1e300 * 10, 1e17 * 10, 2.5 * 2, 1e-7 * 1, -0 * 1", "null"),
        (["-c"], "[.[] * 1]", "[3.5, 1234.5678, 0.0001, 0.00012, 123456789012]"),
    ],
    "ondemand": [
        (["-c"], ".a.b.c", '{"a":{"b":{"c":[1,2]}}}'),
        (["-c"], ".a[1]", '{"a":[10,20,30]}'),
        (["-c"], ".a[]", '{"a":[10,{"x":1},"s"]}'),
        (["-c"], ".[]", '{"a":1,"b":[2]}'),
        (["-c"], ".a?", '[1]'),
        (["-c"], ".a[1]?", '{"a":"str"}'),
        (["-c"], '.["a"]', '{"a":1}'),
        (["-c"], ".missing", '{"a":1}'),
        (["-c"], ".a.b", '{"a":null}'),
        (["-c"], "select(.n > 1) | .n", '{"n":1} {"n":2} {"n":3}'),
        (["-c"], 'select(.s == "x") | .id', '{"s":"x","id":1} {"s":"y","id":2}'),
        (["-c"], "select(.n >= 2 and .n < 3)", '{"n":1} {"n":2} {"n":3}'),
        (["-c"], "select(.n == 1 or .m)", '{"n":1} {"n":2,"m":true} {"n":3}'),
        (["-c"], "select(.n | not)", '{"n":false} {"n":1} {"m":2}'),
        (["-c"], 'select(has("m"))', '{"n":1} {"m":2}'),
        (["-c"], "select(2 < .n)", '{"n":1} {"n":3}'),
        (["-c"], "{a, b: .x.y, c: [.a, .x]}", '{"a":1,"x":{"y":2}}'),
        (["-c"], "[.[] | .k]", '[{"k":1},{"k":2},{}]'),
        (["-c"], ".a, .b", '{"b":2,"a":1}'),
        (["-c"], ".a | length", '{"a":[1,2,3]} {"a":"\\u00e9t\\u00e9"} {"a":{"x":1}} {"a":null} {"a":-5}'),
        (["-c"], '"lit", 1.50, null', '{}'),
        (["-c"], "empty", '{"a":1}'),
        (["-c"], ".", '"root string" 42 true null'),
    ],
    "builtins": [
        (["-c"], "keys, keys_unsorted, length", '{"b":1,"a":2}'),
        (["-c"], "map(. * 2), add, min, max, sort, unique, reverse", "[3,1,2,3]"),
        (["-c"], "sort_by(.n), group_by(.g), unique_by(.g), min_by(.n), max_by(.n)", '[{"n":2,"g":"x"},{"n":1,"g":"y"},{"n":3,"g":"x"}]'),
        (["-c"], "to_entries, with_entries({key, value: (.value + 1)}), (to_entries | from_entries)", '{"a":1,"b":2}'),
        (["-c"], '[.[] | type]', '[null, true, 1, "s", [], {}]'),
        (["-c"], 'has("a"), has("z"), (.a | has(1))', '{"a":[1,2]}'),
        (["-c"], 'contains({a:[1]}), inside({a:[1,2],b:3}), contains("bar")', '{"a":[1,2]}'),
        (["-c"], '.[] | startswith("ab"), endswith("yz"), ltrimstr("a"), rtrimstr("z")', '["abxyz"]'),
        (["-c"], 'split(","), (split(",") | join("-")), ascii_downcase, ascii_upcase', '"A,b,C"'),
        (["-c"], "[limit(3; range(10))], first(range(5;10)), [range(2;8)]", "null"),
        (["-c"], "reduce .[] as $x (0; . + $x), [foreach .[] as $x (0; . + $x)]", "[1,2,3]"),
        (["-c"], ". as $o | $o.a + 1", '{"a":1}'),
        (["-c"], 'if . > 1 then "big" elif . == 1 then "one" else "small" end', "0 1 2"),
        (["-c"], 'try error("x") catch ., (.a // "default"), [.[]?]', '{"b":1}'),
        (["-c"], "[paths], [..]", '{"a":[1,{"b":2}]}'),
        (["-c"], "flatten, any, all, (map(select(. != null)) | length)", "[[1,[2]],null,3]"),
        (["-c"], "tostring, tojson, (tostring | tonumber)", "[1,2]"),
        (["-c"], ".[2:4], .[:2], .[-2:]", '[0,1,2,3,4,5]'),
        (["-c"], ".[2:4]", '"abcdef"'),
        (["-c"], 'indices(1), index(1), rindex(1), (.[0:2] | tojson)', "[0,1,2,1]"),
        (["-c"], 'label $out | .[] | if . > 2 then ., break $out else . end', "[1,2,3,4]"),
        (["-c"], "floor, ceil, round, sqrt, fabs", "6.25"),
        (["-c"], ". + {c:3}, . * {a:{y:2}}, ([1,2,3] - [2])", '{"a":{"x":1}}'),
        (["-c"], '"x" * 3, ("a,b" / ","), (10 % 3)', "null"),
        (["-c"], "[.[] | numbers], [.[] | strings], [.[] | scalars], [.[] | iterables]", '[1,"s",null,[],{}]'),
        (["-c"], ".[1:3], .[-2:], indices(\"\u20ac\"), index(\"x\"), rindex(\"\u20ac\"), split(\"\")", '"a\u00e9\u20acx\u20ac"'),
        (["-c"], "indices([1,2]), index([1,2]), rindex([1,2]), indices(1), indices([])", "[0,1,2,1,2,3]"),
        (["-c"], "\"\" | reverse", "null"),
        (["-c"], ".a?/2, (.b // .a), (.[] // 0)", '{"a":4,"b":null}'),
        (["-c"], "5.5 % 2, 5 % -2, -5 % 2", "null"),
    ],
    "errors": [
        (["-c"], ".a", '[1]'),
        (["-c"], ".a", '{"a":1} [1] {"a":2}'),
        (["-c"], ".[0]", '{"a":1}'),
        (["-c"], ".[]", "1"),
        (["-c"], "1 / 0", "null"),
        (["-c"], 'error("custom")', "null"),
        (["-c"], ".", '{"a": [1 2]}'),
        (["-c"], '(1, error("x"), 2) // 3', "null"),
        (["-c"], 'error("x") // 1', "null"),
        (["-c"], "reverse", '"ab"'),
        (["-c"], '"\\u41"', "null"),
        (["-c"], '"\\ud800"', "null"),
        (["-c"], ".a?//1", "null"),
    ],
    "files": [
        (["-c"], ".statuses[0].user.screen_name", "@twitter.json"),
        (["-c"], ".statuses | length", "@twitter.json"),
        (["-r"], ".statuses[].user.lang", "@twitter.json"),
        (["-c"], ".statuses[] | select(.retweet_count > 10) | {id, n: .retweet_count}", "@twitter.json"),
        (["-c"], ".search_metadata", "@twitter.json"),
        (["-c"], "[.performances[] | .seatCategories | length] | add", "@citm_catalog.json"),
        (["-c"], ".areaNames | keys | length", "@citm_catalog.json"),
    ],
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jq", required=True)
    ap.add_argument("--data", default=os.path.join(HERE, "..", "..", "..", "jsonexamples"))
    args = ap.parse_args()
    os.makedirs(os.path.join(HERE, "cases"), exist_ok=True)
    for name, specs in SPECS.items():
        out = [f"# Recorded from jq 1.8.2 by make_golden.py; do not edit by hand.\n"]
        for flags, program, inp in specs:
            if inp.startswith("@"):
                argv = [args.jq] + flags + [program, os.path.join(args.data, inp[1:])]
                stdin = b""
            else:
                argv = [args.jq] + flags + [program]
                stdin = (inp + "\n").encode("utf-8")
            p = subprocess.run(argv, input=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            block = []
            if flags:
                block.append("%%FLAGS " + " ".join(flags))
            if inp.startswith("@"):
                block.append("%%FILE " + inp[1:])
            if p.returncode:
                block.append(f"%%EXIT {p.returncode}")
            block.append(program)
            if not inp.startswith("@"):
                block.append(inp)
            text = p.stdout.decode("utf-8")
            if text and not text.endswith("\n"):
                raise SystemExit(f"{name}: output of {program!r} does not end with a newline")
            if "\n\n" in text or (text.startswith("\n")):
                raise SystemExit(f"{name}: output of {program!r} has an empty line")
            block.extend(text.split("\n")[:-1] if text else [])
            out.append("\n".join(block) + "\n\n")
        with open(os.path.join(HERE, "cases", name + ".test"), "w", encoding="utf-8", newline="\n") as f:
            f.write("".join(out))
        print(f"wrote cases/{name}.test ({len(specs)} cases)")


if __name__ == "__main__":
    main()
