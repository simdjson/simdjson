#!/usr/bin/env python3
# Structured seed corpus and dictionary generator for fuzz_builder.
#
# Seeds are operation sequences for fuzz_builder.cpp (see its header comment
# for the op byte layout and payload order). The encoder below mirrors the
# harness consumption EXACTLY, including the C++ argument-evaluation order:
# for value ops (7-10, 12) the value payload bytes are consumed at the call
# site BEFORE the key bytes are consumed inside the emitter, so in object
# context the layout is [op][value payload][key]. A python state machine
# mirrors the harness structural rules so every seed drives builder A with
# well-formed operations.
#
# Usage: gen_builder_corpus.py <output-basename>
# Writes <output-basename>.zip (seed corpus) and <output-basename>.dict.

import sys
import zipfile

OP_START_OBJ = 0
OP_END_OBJ = 1
OP_START_ARR = 2
OP_END_ARR = 3
OP_NULL = 6
OP_INT8 = 7
OP_UINT64 = 8
OP_DOUBLE = 9
OP_BOOL = 10
OP_STRING = 12
OP_R_COMMA = 4
OP_R_COLON = 5
OP_R_CHAR = 11
OP_R_ESCAPE = 13
OP_R_RAW = 14
OP_R_CYCLE = 15


class Enc:
    """Byte-stream encoder mirroring the harness state machine."""

    def __init__(self):
        self.out = bytearray()
        self.stack = []  # '{' or '['
        self.root_used = False

    def _in_obj(self):
        return bool(self.stack) and self.stack[-1] == '{'

    def _in_arr(self):
        return bool(self.stack) and self.stack[-1] == '['

    @staticmethod
    def _str_bytes(s):
        b = s.encode('utf-8')
        return bytes([len(b)]) + b

    # raw builder ops (builder B)
    def r_comma(self):
        self.out.append(OP_R_COMMA)

    def r_colon(self):
        self.out.append(OP_R_COLON)

    def r_char(self, c):
        self.out.append(OP_R_CHAR)
        self.out.append(c & 0xFF)

    def r_escape(self, s):
        self.out.append(OP_R_ESCAPE)
        self.out += self._str_bytes(s)

    def r_raw(self, s):
        self.out.append(OP_R_RAW)
        self.out += self._str_bytes(s)

    def r_cycle(self):
        self.out.append(OP_R_CYCLE)

    # structured ops (builder A); every structured op takes effect by design
    # (seeds never rely on skip paths)
    def start_obj(self, key=None):
        if self._in_obj():
            assert key is not None, 'container in object needs a key'
            self.out.append(OP_START_OBJ)
            self.out += self._str_bytes(key)
        else:
            self.out.append(OP_START_OBJ)
        self.stack.append('{')
        self.root_used = True

    def start_arr(self, key=None):
        if self._in_obj():
            assert key is not None, 'container in object needs a key'
            self.out.append(OP_START_ARR)
            self.out += self._str_bytes(key)
        else:
            self.out.append(OP_START_ARR)
        self.stack.append('[')
        self.root_used = True

    def end(self):
        assert self.stack, 'seed closed a container that was not open'
        self.out.append(OP_END_OBJ if self.stack[-1] == '{' else OP_END_ARR)
        self.stack.pop()

    def null(self, key=None):
        self.out.append(OP_NULL)
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)

    def int8(self, v, key=None):
        self.out.append(OP_INT8)
        self.out.append(v & 0xFF)
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)

    def uint64(self, v, key=None):
        self.out.append(OP_UINT64)
        self.out += v.to_bytes(8, 'little')
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)

    def double_bits(self, bits, key=None):
        self.out.append(OP_DOUBLE)
        self.out += bits.to_bytes(8, 'little')
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)

    def boolean(self, v, key=None):
        self.out.append(OP_BOOL)
        self.out.append(1 if v else 0)
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)

    def string(self, s, key=None):
        self.out.append(OP_STRING)
        self.out += self._str_bytes(s)
        if self._in_obj():
            assert key is not None
            self.out += self._str_bytes(key)


def build_seed(setup):
    enc = Enc()
    setup(enc)
    return bytes(enc.out)


def s_int_array(e):
    e.start_arr()
    e.int8(200)
    e.int8(100)
    e.int8(0)
    e.end()


def s_object(e):
    e.start_obj()
    e.string('x', key='a')
    e.string('', key='b')
    e.boolean(True, key='c')
    e.uint64(0xFFFFFFFFFFFFFFFF, key='d')
    e.end()


def s_nested(e):
    e.start_obj()
    e.start_obj(key='n')
    e.int8(-128, key='deep')
    e.end()
    e.start_arr(key='l')
    e.string('v')
    e.end()
    e.end()


def s_doubles(e):
    e.start_arr()
    e.double_bits(0x7FF8000000000000)  # quiet NaN -> "null"
    e.double_bits(0x7FF0000000000000)  # +inf -> "null"
    e.double_bits(0xFFF0000000000000)  # -inf -> "null"
    e.double_bits(0x3FF0000000000000)  # 1.0
    e.double_bits(0x0000000000000001)  # denormal
    e.end()


def s_strings(e):
    e.start_arr()
    e.string('"\\ \n\t\x01')
    e.string('emoji\U0001f603\u043ach\u6b4c')
    e.string('A' * 80)
    e.string('A' * 80)
    e.end()


def s_nulls(e):
    e.start_arr()
    e.null()
    e.null()
    e.end()
    e.start_obj()
    e.null(key='n1')
    e.null(key='n2')
    e.end()


def s_uint_extremes(e):
    e.start_arr()
    e.uint64(0xFFFFFFFFFFFFFFFF)
    e.uint64(0x8000000000000000)
    e.uint64(0)
    e.int8(0)    # -128
    e.int8(255)  # 127
    e.end()


def s_empty_containers(e):
    e.start_arr()
    e.end()
    e.start_obj()
    e.end()
    e.start_arr()
    e.end()


def s_deep(e):
    for _ in range(12):
        e.start_arr()
    for _ in range(12):
        e.end()


def s_mixed(e):
    e.start_obj()
    e.string('str', key='s')
    e.boolean(False, key='f')
    e.start_arr(key='arr')
    e.int8(50)
    e.start_obj()
    e.string('in', key='k')
    e.end()
    e.double_bits(0x4008000000000000, key='d')  # 3.0
    e.end()
    e.null(key='nothing')
    e.end()


def s_raw_heavy(e):
    e.r_raw('{"x":')
    e.r_raw('123}')
    e.r_cycle()
    e.r_char(0x22)
    e.r_escape('quote\\" ctrl\x07')
    e.r_comma()
    e.r_colon()
    e.r_cycle()


SEEDS = [
    ('int-array', s_int_array),
    ('object', s_object),
    ('nested', s_nested),
    ('doubles-special', s_doubles),
    ('strings-escapes', s_strings),
    ('nulls', s_nulls),
    ('uint-extremes', s_uint_extremes),
    ('empty-containers', s_empty_containers),
    ('deep-12', s_deep),
    ('mixed', s_mixed),
    ('raw-heavy', s_raw_heavy),
]


# --- dictionary (byte-safe escaping, no backslash literals) ----------------

BS = 0x5C
DQ = 0x22


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
            out.append(chr(BS) + 'x%02x' % b)
    return '%s="%s"' % (name, ''.join(out))


def _s(text):
    return text.encode('utf-8')


DICT_VALUES = [
    _s('{'), _s('}'), _s('['), _s(']'), _s(','), _s(':'), _s('": '),
    _s('null'), _s('true'), _s('false'), _s('""'), _s('"a"'), _s('"k"'),
    _s('0'), _s('1'), _s('-128'), _s('127'), _s('1.0'), _s('2.5'),
    _s('18446744073709551615'), _s('9223372036854775807'),
    _s('-9223372036854775808'),
    bytes([DQ, BS, 0x6E, DQ]),
    bytes([DQ, BS, 0x74, DQ]),
    bytes([DQ, BS, BS, DQ]),
    bytes([DQ, BS, 0x75, 0x30, 0x30, 0x30, 0x31, DQ]),
    bytes([DQ, 0xF0, 0x9F, 0x98, 0x80, DQ]),
    bytes([DQ] + [0x41] * 64 + [DQ]),
]


def main():
    if len(sys.argv) != 2:
        sys.exit('usage: gen_builder_corpus.py <output-basename>')
    base = sys.argv[1]
    with zipfile.ZipFile(base + '.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for name, setup in SEEDS:
            z.writestr(name + '.txt', build_seed(setup))
    with open(base + '.dict', 'w', encoding='utf-8') as f:
        for i, value in enumerate(DICT_VALUES):
            f.write(dict_line('kw%d' % i, value) + '\n')
    print('wrote %s.zip (%d seeds) and %s.dict (%d entries)'
          % (base, len(SEEDS), base, len(DICT_VALUES)))


if __name__ == '__main__':
    main()
