from pathlib import Path
from typing import Literal
from io import BufferedWriter, BufferedReader
import re
import sys

ContainerState = Literal['orig', 'array', 'flat']
ScalarState = Literal['orig', '1digit', 'str']
StringState = Literal['orig', 'unescaped', 'ascii', 'empty']
CONTAINER_STATES: list[ContainerState] = ['orig', 'array', 'flat']
SCALAR_STATES: list[ScalarState] = ['orig', '1digit', 'str']
STRING_STATES: list[StringState] = ['orig', 'unescaped', 'ascii', 'empty']

def right_pad(padded_length: int, b: bytes):
    assert len(b) <= padded_length
    return b + b' '*(padded_length-len(b))

def right_pad2(r: bytes, b: bytes):
    print(f"right_pad({r}, {b})")
    return right_pad(len(r), b)

class JsonFile:
    def __init__(self,
                 original_json_file: Path,
                 container_state: ContainerState = 'orig',
                 scalar_state: ScalarState = 'orig',
                 string_state: StringState = 'orig'):
        self.original_json_file = original_json_file
        self.container_state: ContainerState = container_state
        self.scalar_state: ScalarState = scalar_state
        self.string_state: StringState = string_state

    @property
    def path(self):
        if self.container_state == 'orig' and self.scalar_state == 'orig' and self.string_state == 'orig':
            return self.original_json_file
        else:
            return self.original_json_file.with_stem(f"{self.original_json_file.stem}-{self.container_state}-{self.scalar_state}-{self.string_state}")

    def write(self, force: bool = False):
        if force or not self.path.exists():
            with open(self.path, 'wb') as out:
                self.write_to(out)

    def with_container_state(self, container_state: ContainerState):
        return JsonFile(self.original_json_file, container_state, self.scalar_state, self.string_state)
    def with_scalar_state(self, scalar_state: ScalarState):
        return JsonFile(self.original_json_file, self.container_state, scalar_state, self.string_state)
    def with_string_state(self, string_state: StringState):
        return JsonFile(self.original_json_file, self.container_state, self.scalar_state, string_state)    

    def open(self):
        return open(self.path, 'rb')

    def write_to(self, out: BufferedWriter):
        if self.string_state == 'unescaped':
            return self.remove_escapes(self.with_string_state('orig').open(), out)
        elif self.string_state == 'ascii':
            return self.remove_utf8(self.with_string_state('unescaped').open(), out)
        elif self.string_state == 'empty':
            return self.replace_strings(self.with_string_state('ascii').open(), out, b'""')
        else:
            assert self.string_state == 'orig'

        if self.scalar_state == '1digit':
            return self.replace_numbers(self.with_scalar_state('orig').open(), out, b'0')
        elif self.scalar_state == 'str':
            return self.replace_non_strings(self.with_scalar_state('1digit').open(), out, b'""')
        else:
            assert self.scalar_state == 'orig'

        if self.container_state == 'array':
            return self.replace_objects_with_arrays(self.with_container_state('orig').open(), out)
        elif self.container_state == 'flat':
            return self.remove_nesting(self.with_container_state('array').open(), out)
        else:
            assert self.container_state == 'orig'

        assert self.path.exists()

    def remove_escapes(self, input: BufferedReader, out: BufferedWriter):
        for line in input:
            out.write(re.sub(rb'\\(.)', rb'__', line))

    def remove_utf8(self, input: BufferedReader, out: BufferedWriter):
        for line in input:
            out.write(bytes([(b if b < 128 else ord('_')) for b in line]))

    def replace_strings(self, input: BufferedReader, out: BufferedWriter, replacement: bytes):
        for line in input:
            assert line.find(b'\\') == -1
            out.write(re.sub(rb'"([^"]*)"', lambda s: right_pad(len(s.group(0)), replacement), line))

    def replace_numbers(self, input: BufferedReader, out: BufferedWriter, replacement: bytes):
        for line in input:
            for (non_string, string) in self.split_by_strings(line):
                out.write(re.sub(rb'\s*[-0-9][-+0-9.eE]*\s*', lambda s: right_pad(len(s.group(0)), replacement), non_string))
                out.write(string)

    def replace_non_strings(self, input: BufferedReader, out: BufferedWriter, replacement: bytes):
        for line in input:
            for (non_string, string) in self.split_by_strings(line):
                out.write(re.sub(rb'\s*[^,:{}[\] \r\t\n]+\s*', lambda s: right_pad(len(s.group(0)), replacement), non_string))
                out.write(string)

    def replace_objects_with_arrays(self, input: BufferedReader, out: BufferedWriter):
        for line in input:
            for (non_string, string) in self.split_by_strings(line):
                out.write(non_string.replace(b'{', b'[').replace(b'}', b']').replace(b':', b','))
                out.write(string)

    def remove_nesting(self, input: BufferedReader, out: BufferedWriter):
        prev_line = None
        is_first_line = True
        lines = iter(input)
        line = next(lines, None)
        next_line = None
        while line is not None:
            out_line = b''
            # Remove any { } or [ ], and replace : with ,
            for (non_string, string) in self.split_by_strings(line):
                # Replace empty objects or arrays with ""
                non_string = re.sub(rb'(\{(\s|\n)*\}|\[(\s|\n*)\])', lambda s: right_pad(len(s.group(0)), b'""'), non_string)
                # Remove other braces entirely
                non_string = re.sub(rb'([{}[\]])', lambda s: right_pad(len(s.group(0)), b' '), non_string)
                # Replace : with ,
                non_string = non_string.replace(b':', b',')
                out_line += non_string
                out_line += string

            # Replace the first character with [
            if next_line is None:
                assert line[0] in [ord(x) for x in [ b'[', b'{', b' ', b'\t', b'\r', b'\n' ]]
                out_line = b'[' + out_line[1:]

            # Replace the last character with ]
            next_line = next(lines, None)
            if next_line is None:
                assert out_line[-1] in [ord(x) for x in [ b']', b'}', b' ', b'\t', b'\r', b'\n' ]]
                out_line = bytes(out_line[:-1] + b']')
            line = next_line
            out.write(out_line)

    def split_by_strings(self, line: bytes):
        result: list[tuple[bytes, bytes]] = []
        while len(line) > 0:
            quote = line.find(b'"')
            if quote == -1:
                result.append((line, b''))
                break
            end_quote = quote+1
            while line[end_quote] != ord(b'"'):
                assert end_quote < len(line)
                if line[end_quote] == ord(b'\\'):
                    end_quote += 1
                end_quote += 1
            result.append((line[:quote],line[quote:end_quote+1]))
            line = line[end_quote+1:]
        return result

original_json_file = Path(sys.argv[1])
for container_state in CONTAINER_STATES:
    for scalar_state in SCALAR_STATES:
        for string_state in STRING_STATES:
            output_file = JsonFile(original_json_file, container_state, scalar_state, string_state)
            if output_file.path.exists():
                print(f"Skipping {output_file.path}")
                continue
            print(f"Writing {output_file.path}")
            output_file.write()
