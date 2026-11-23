#!/usr/bin/env python3
"""Tests for sjq.

Groups (all run by default):
  golden    cases/*.test: exact output and exit status, recorded from jq 1.8.2
  targeted  behavior that needs files or processes (mapping, streaming, ...)
  jqsuite   jq's own jq.test and man.test; tests listed in
            jq/expected_pass.txt must pass, the others are reported
  diff      differential runs against jq (only with --jq)

Usage: run_tests.py --sjq PATH [--data DIR] [--jq PATH] [--group NAME]...
       run_tests.py --sjq PATH --update-expected   (rewrite expected_pass.txt)
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TIMEOUT = 60

failures = []


def fail(name, detail):
    failures.append(name)
    print(f"FAIL {name}\n{detail}", flush=True)


def norm(b):
    # Windows writes CRLF line ends unless -b is given.
    return b.replace(b"\r\n", b"\n") if sys.platform == "win32" else b


def run(argv, stdin=b"", env=None, timeout=TIMEOUT):
    full_env = dict(os.environ)
    full_env.pop("NO_COLOR", None)
    full_env.pop("JQ_COLORS", None)
    if env:
        full_env.update(env)
    try:
        p = subprocess.run(argv, input=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           env=full_env, timeout=timeout)
        return p.returncode, norm(p.stdout), norm(p.stderr)
    except subprocess.TimeoutExpired:
        return None, b"", b"timeout"


# ---------------------------------------------------------------------------
# Golden cases. Format, blocks separated by blank lines:
#   # comment
#   %%FLAGS -c -r        (optional)
#   %%FILE twitter.json  (optional: input file from --data instead of a line)
#   %%EXIT 5             (optional, default 0)
#   program
#   input                (one line, absent with %%FILE)
#   expected output lines (exact text)
# ---------------------------------------------------------------------------

def parse_blocks(path):
    blocks, cur, start = [], [], 0
    with open(path, "rb") as f:
        lines = f.read().decode("utf-8").split("\n")
    for i, line in enumerate(lines + [""]):
        if line.startswith("#") and not cur:
            continue
        if line.strip() == "":
            if cur:
                blocks.append((start, cur))
                cur = []
            continue
        if not cur:
            start = i + 1
        cur.append(line)
    return blocks


def golden(sjq, data):
    count = 0
    for name in sorted(os.listdir(os.path.join(HERE, "cases"))):
        if not name.endswith(".test"):
            continue
        path = os.path.join(HERE, "cases", name)
        for line_no, block in parse_blocks(path):
            flags, file_arg, exit_code = [], None, 0
            while block and block[0].startswith("%%"):
                key, _, rest = block[0].partition(" ")
                if key == "%%FLAGS":
                    flags = rest.split()
                elif key == "%%FILE":
                    file_arg = os.path.join(data, rest.strip())
                elif key == "%%EXIT":
                    exit_code = int(rest)
                block = block[1:]
            program = block[0]
            if file_arg:
                stdin, expected = b"", block[1:]
                argv = [sjq] + flags + [program, file_arg]
            else:
                stdin, expected = (block[1] + "\n").encode("utf-8"), block[2:]
                argv = [sjq] + flags + [program]
            code, out, err = run(argv, stdin)
            want = "".join(e + "\n" for e in expected).encode("utf-8")
            count += 1
            if code != exit_code or out != want:
                fail(f"golden {name}:{line_no} {program}",
                     f"  exit {code} (want {exit_code})\n  got:  {out[:400]!r}\n  want: {want[:400]!r}\n  stderr: {err[:200]!r}")
    print(f"golden: {count} cases", flush=True)


# ---------------------------------------------------------------------------
# jq's test suite
# ---------------------------------------------------------------------------

def parse_jq_tests(path):
    """Yields (id, kind, program, input, outputs); kind is 'ok' or 'fail'."""
    with open(path, "rb") as f:
        lines = f.read().decode("utf-8").split("\n")
    i, n = 0, len(lines)
    base = os.path.basename(path)
    while i < n:
        line = lines[i]
        if line.strip() == "" or line.startswith("#"):
            i += 1
            continue
        if line.startswith("%%FAIL"):
            program = lines[i + 1]
            tid = f"{base}:{i + 2}"
            i += 2
            while i < n and lines[i].strip() != "":
                i += 1
            yield tid, "fail", program, None, None
            continue
        tid = f"{base}:{i + 1}"
        program = line
        inp = lines[i + 1] if i + 1 < n else ""
        i += 2
        outs = []
        while i < n and lines[i].strip() != "":
            outs.append(lines[i])
            i += 1
        yield tid, "ok", program, inp, outs


def json_values(text):
    dec = json.JSONDecoder()
    vals, i, s = [], 0, text.strip()
    while i < len(s):
        v, j = dec.raw_decode(s, i)
        vals.append(v)
        i = j
        while i < len(s) and s[i].isspace():
            i += 1
    return vals


def jq_test_passes(sjq, kind, program, inp, outs):
    if kind == "fail":
        # %%FAIL programs must be rejected at compile time (exit status 3),
        # not crash or time out.
        code, _, _ = run([sjq, "-n", program], timeout=10)
        return code == 3
    code, out, _ = run([sjq, "-c", program], (inp + "\n").encode("utf-8"), timeout=10)
    if code != 0:
        return False
    try:
        got = json_values(out.decode("utf-8"))
        want = [json.loads(o) for o in outs]
    except (ValueError, UnicodeDecodeError):
        return False
    return got == want


def jqsuite(sjq, update):
    expected_path = os.path.join(HERE, "jq", "expected_pass.txt")
    expected = set()
    if os.path.exists(expected_path) and not update:
        with open(expected_path) as f:
            expected = {l.strip() for l in f if l.strip() and not l.startswith("#")}
    passed, total, tests = [], 0, {}
    for name in ("jq.test", "man.test"):
        for tid, kind, program, inp, outs in parse_jq_tests(os.path.join(HERE, "jq", name)):
            total += 1
            tests[tid] = (kind, program, inp)
            if jq_test_passes(sjq, kind, program, inp, outs):
                passed.append(tid)
    passed_set = set(passed)
    print(f"jqsuite: {len(passed)} of {total} jq tests pass", flush=True)
    if update:
        with open(expected_path, "w") as f:
            f.write("# jq tests (file:line of the program) that sjq passes; see run_tests.py\n")
            for tid in passed:
                f.write(tid + "\n")
        print(f"jqsuite: wrote {expected_path}")
        return
    lost = sorted(expected - passed_set)
    gained = sorted(passed_set - expected)
    if lost:
        details = []
        for tid in lost:
            if tid not in tests:
                details.append(f"  {tid}: no such test")
                continue
            kind, program, inp = tests[tid]
            argv = [sjq, "-n", program] if kind == "fail" else [sjq, "-c", program]
            start = time.time()
            code, out, err = run(argv, b"" if kind == "fail" else (inp + "\n").encode("utf-8"), timeout=10)
            details.append(f"  {tid}: {program[:60]!r} exit {code} in {time.time() - start:.1f} s, "
                           f"stdout {out[:80]!r}, stderr {err[:120]!r}")
        fail("jqsuite", "  no longer passing: " + " ".join(lost) + "\n" + "\n".join(details))
    if gained:
        print("jqsuite: newly passing (update expected_pass.txt): " + " ".join(gained), flush=True)


# ---------------------------------------------------------------------------
# Targeted tests
# ---------------------------------------------------------------------------

def check(name, cond, detail=""):
    if not cond:
        fail(name, "  " + detail)


def targeted(sjq, data, tmp):
    # Exit status.
    check("exit: no filter", run([sjq])[0] == 2)
    code, _, err = run([sjq, ".["], b"1")
    check("exit: syntax error", code == 3, f"exit {code} {err[:200]!r}")
    check("exit: --indent 8", run([sjq, "--indent", "8", "."], b"1")[0] == 2)
    check("exit: missing file", run([sjq, ".", os.path.join(tmp, "nope.json")])[0] == 2)
    check("exit: runtime error of the last input", run([sjq, ".a"], b"[]")[0] == 5)
    check("exit: error then success", run([sjq, ".a"], b'[] {"a":1}')[0] == 0)
    check("exit: -b accepted", run([sjq, "-b", "-c", "."], b"[1]")[1] == b"[1]\n")
    check("-j", run([sjq, "-j", ".[]"], b'["x", 1, "y"]')[1] == b"x1y")
    check("empty input", run([sjq, "."], b"") == (0, b"", b""))
    check("blank input", run([sjq, "."], b"  \n ") == (0, b"", b""))

    # A stack too small for the On-Demand writer: the DOM path runs the
    # document again, and either succeeds or reports the error, never aborts.
    if os.name == "posix":
        import resource
        deep = b"[" * 1000 + b"]" * 1000
        def small_stack():
            resource.setrlimit(resource.RLIMIT_STACK, (256 << 10, resource.getrlimit(resource.RLIMIT_STACK)[1]))
        p = subprocess.run([sjq, "-c", ".[0]"], input=deep, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           preexec_fn=small_stack, timeout=TIMEOUT)
        ok = (p.returncode == 0 and p.stdout == deep[1:-1] + b"\n") or (p.returncode == 5 and b"nested too deeply" in p.stderr)
        check("stack guard on the On-Demand path", ok, f"exit {p.returncode} {p.stderr[:200]!r}")

    # Input sizes around page boundaries: the mapped padding must hold.
    for page in (4096, 16384, 65536):
        for d in (0, 1, 63, 64, 65):
            size = page - d
            # An array, then a bare number in the very last byte.
            text = b"[" + b"1," * ((size - 3) // 2 - 4) + b"0]"
            text = text + b" " * (size - len(text) - 2) + b" 7"
            path = os.path.join(tmp, f"pad_{size}.json")
            with open(path, "wb") as f:
                f.write(text[:size])
            code, out, err = run([sjq, "-c", "length", path])
            check(f"padding {size}", code == 0 and out.endswith(b"7\n"), f"exit {code} {out[-40:]!r} {err[:200]!r}")

    # A large NDJSON file, filtered in parallel, sequentially, redirected and piped.
    docs = [{"i": i, "s": "x" * (i % 50), "a": [i, i * 2], "o": {"k": str(i)}} for i in range(120000)]
    nd = "".join(json.dumps(d, separators=(",", ":")) + "\n" for d in docs).encode()
    nd_path = os.path.join(tmp, "big.ndjson")
    with open(nd_path, "wb") as f:
        f.write(nd)
    want = "".join(json.dumps(d["o"], separators=(",", ":")) + "\n" for d in docs).encode()
    for label, argv, stdin in (
        ("parallel", [sjq, "-c", ".o", nd_path], b""),
        ("sequential", [sjq, "--threads", "1", "-c", ".o", nd_path], b""),
        ("no-mmap", [sjq, "--no-mmap", "-c", ".o", nd_path], b""),
        ("pipe", [sjq, "-c", ".o"], nd),
    ):
        code, out, _ = run(argv, stdin)
        check(f"ndjson {label}", code == 0 and out == want, f"exit {code}, {len(out)} bytes")
    with open(nd_path, "rb") as f:
        p = subprocess.run([sjq, "-c", ".o"], stdin=f, stdout=subprocess.PIPE, timeout=TIMEOUT)
    check("ndjson redirected stdin", p.returncode == 0 and norm(p.stdout) == want)

    # Pretty-printed documents: the parallel cuts land inside documents.
    pretty = "".join(json.dumps(d, indent=2) + "\n" for d in docs[:40000]).encode()
    pretty_path = os.path.join(tmp, "pretty.json")
    with open(pretty_path, "wb") as f:
        f.write(pretty)
    want_c = "".join(json.dumps(d, separators=(",", ":")) + "\n" for d in docs[:40000]).encode()
    code, out, _ = run([sjq, "-c", ".", pretty_path])
    check("pretty stream parallel fallback", code == 0 and out == want_c, f"exit {code}")

    # An error in the middle stops the output there.
    bad = nd.split(b"\n")
    bad_doc = b"\n".join(bad[:60000]) + b'\n{"i": [1 2]}\n' + b"\n".join(bad[60000:])
    bad_path = os.path.join(tmp, "bad.ndjson")
    with open(bad_path, "wb") as f:
        f.write(bad_doc)
    code, out, _ = run([sjq, "-c", ".i", bad_path])
    check("error mid-stream", code == 5 and out.count(b"\n") == 60000, f"exit {code}, {out.count(chr(10).encode())} lines")
    code, out, err = run([sjq, "-c", "."], b'{"a":1}\n{"a": [1,')
    check("truncated input", code == 5 and out == b'{"a":1}\n' and b"Unfinished" in err, f"exit {code} {out!r} {err!r}")

    # A file that shrinks while it is read (POSIX test hook).
    if sys.platform != "win32":
        victim = os.path.join(tmp, "victim.ndjson")
        with open(victim, "wb") as f:
            f.write(nd)
        code, _, err = run([sjq, "-c", ".i", victim], env={"SJQ_TEST_TRUNCATE": "1000"})
        check("file truncated while read", code == 2 and b"changed while being read" in err, f"exit {code} {err!r}")

    # Streaming: each document is written before the next one arrives.
    p = subprocess.Popen([sjq, "--unbuffered", "-c", ".a"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    got = []

    def reader():
        got.append(p.stdout.readline())

    p.stdin.write(b'{"a": 1}\n')
    p.stdin.flush()
    t = threading.Thread(target=reader)
    t.start()
    t.join(20)
    streaming_ok = bool(got) and norm(got[0]) == b"1\n"
    p.stdin.write(b'{"a": 2}\n')
    p.stdin.close()
    rest = norm(p.stdout.read())
    p.wait(timeout=TIMEOUT)
    check("streaming stdin", streaming_ok and rest == b"2\n", f"first {got!r} rest {rest!r}")

    # Colors (jq 1.8 layout and defaults), JQ_COLORS.
    code, out, _ = run([sjq, "-C", "-c", "."], b'{"a":[1,"x",null,true]}')
    want_color = (b'\x1b[1;39m{\x1b[0m\x1b[1;34m"a"\x1b[0m\x1b[1;39m:\x1b[0m\x1b[1;39m[\x1b[0m\x1b[0;39m1\x1b[0m'
                  b'\x1b[1;39m,\x1b[0m\x1b[0;32m"x"\x1b[0m\x1b[1;39m,\x1b[0m\x1b[0;90mnull\x1b[0m\x1b[1;39m,\x1b[0m'
                  b'\x1b[0;39mtrue\x1b[0m\x1b[1;39m]\x1b[0m\x1b[1;39m}\x1b[0m\n')
    check("colors default", out == want_color, repr(out))
    code, out, _ = run([sjq, "-C", "-c", "."], b'{"a":1}', env={"JQ_COLORS": "0;31:0;32:0;33:4;34:0;35:0;36:0;37:4;31"})
    check("colors JQ_COLORS", out == b'\x1b[0;37m{\x1b[0m\x1b[4;31m"a"\x1b[0m\x1b[0;37m:\x1b[0m\x1b[4;34m1\x1b[0m\x1b[0;37m}\x1b[0m\n', repr(out))
    code, out, err = run([sjq, "-C", "-c", "."], b'1', env={"JQ_COLORS": "bad"})
    check("colors invalid JQ_COLORS", out == b"\x1b[0;39m1\x1b[0m\n" and b"Failed to set $JQ_COLORS" in err, repr(out))
    code, out, _ = run([sjq, "-c", "."], b'1')
    check("no colors on a pipe", out == b"1\n")

    # --seq: RS before each output, RS-separated input.
    code, out, _ = run([sjq, "--seq", "-c", ".a"], b'\x1e{"a":1}\n\x1e{"a":2}\n')
    check("--seq", out == b"\x1e1\n\x1e2\n", repr(out))

    # Non-ASCII file names and filters (UTF-8 code page on Windows).
    uname = os.path.join(tmp, "donn\u00e9es-\u00e9t\u00e9.json")
    with open(uname, "wb") as f:
        f.write('{"cl\u00e9": "valeur \u00e9t\u00e9"}'.encode("utf-8"))
    code, out, err = run([sjq, "-r", '.["cl\u00e9"]', uname])
    check("utf-8 file name and filter", code == 0 and out == "valeur \u00e9t\u00e9\n".encode("utf-8"), f"exit {code} {out!r} {err!r}")

    # Deep values: capped with an error, never a crash.
    code, _, err = run([sjq, "-n", "reduce range(20000) as $i (null; [.]) | length"])
    check("deep value cap", code == 5 and b"nested too deeply" in err, f"exit {code} {err!r}")
    code, _, err = run([sjq, "-n", "1 | recurse(.)"])
    check("recurse cap", code == 5 and b"nested too deeply" in err, f"exit {code} {err!r}")
    code, _, _ = run([sjq, "(" * 2000 + "." + ")" * 2000], b"1")
    check("deeply nested filter", code == 3, f"exit {code}")


# ---------------------------------------------------------------------------
# Differential runs against jq (stdout and exit status; error wording and the
# documented differences are left out).
# ---------------------------------------------------------------------------

DIFF_FILTERS = [
    ".", ".statuses[0]", ".statuses[].user.screen_name", ".statuses | length",
    "[.statuses[] | select(.retweet_count > 0) | .id]",
    ".statuses[] | {id, name: .user.name, n: .user.followers_count}",
    ".statuses | group_by(.user.lang) | map({lang: .[0].user.lang, n: length})",
    ".statuses[] | .entities.urls[]?.expanded_url", ".statuses[0].user | to_entries | .[0:3]",
    "reduce .statuses[] as $s (0; . + $s.favorite_count)", "[paths] | length",
    ".statuses[] | select(.user.lang == \"ja\" and .retweet_count > 0) | .id",
    ".search_metadata", "[..|numbers] | length", ".statuses | map(.text | length) | max",
    ".statuses[0].user | to_entries | .[0:3]", "to_entries", "map(.)", "join(\",\")", "flatten", "any", "add",
    ".a", ".n", ".[]?", "select(.b == true) | .n", "{x: .a, y: .b}", ".a | length",
]


# Known differences, reported but not failures; step by step they go away.
DIFF_KNOWN = {
    # Number literals are lost on the DOM path (map, add over values, ...).
    ("tricky.ndjson", "map(.)"), ("tricky.ndjson", "add"), ("tricky.ndjson", "to_entries"),
    ("tricky.ndjson", "flatten"), ("tricky.ndjson", "join(\",\")"),
    # jq iterates objects in flatten, any and join; sjq requires arrays.
    ("twitter.json", "flatten"), ("twitter.json", "any"), ("tricky.ndjson", "any"),
    ("twitter.json", "join(\",\")"),
}


def differential(sjq, jq, data):
    count = 0
    known = 0
    files = [os.path.join(data, "twitter.json"), os.path.join(HERE, "data", "tricky.ndjson")]
    for path in files:
        for program in DIFF_FILTERS:
            for flags in ([], ["-c"], ["-r"], ["--tab"], ["--indent", "1"]):
                a = run([jq] + flags + [program, path])
                b = run([sjq] + flags + [program, path])
                count += 1
                if a[0] != b[0] or a[1] != b[1]:
                    if (os.path.basename(path), program) in DIFF_KNOWN:
                        known += 1
                    else:
                        fail(f"diff {os.path.basename(path)} {flags} {program}", f"  jq exit {a[0]}, sjq exit {b[0]}")
    print(f"diff: {count} runs against {jq}, {known} known differences", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sjq", required=True)
    ap.add_argument("--data", default=os.path.join(HERE, "..", "..", "..", "jsonexamples"))
    ap.add_argument("--jq")
    ap.add_argument("--group", action="append")
    ap.add_argument("--update-expected", action="store_true")
    args = ap.parse_args()
    sjq = os.path.abspath(args.sjq)
    groups = args.group or ["golden", "targeted", "jqsuite", "diff"]
    start = time.time()
    if args.update_expected:
        jqsuite(sjq, True)
        return 0
    with tempfile.TemporaryDirectory() as tmp:
        if "golden" in groups:
            golden(sjq, args.data)
        if "targeted" in groups:
            targeted(sjq, args.data, tmp)
        if "jqsuite" in groups:
            jqsuite(sjq, False)
        if "diff" in groups and args.jq:
            differential(sjq, args.jq, args.data)
    print(f"{len(failures)} failure(s) in {time.time() - start:.1f} s")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
