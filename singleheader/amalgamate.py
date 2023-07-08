#!/usr/bin/env python3
#
# Creates the amalgamated source files.
#

import sys
import os.path
import subprocess
import os
import re
import shutil
import datetime
from typing import Iterable, Optional, TextIO

if sys.version_info < (3, 0):
    sys.stdout.write("Sorry, requires Python 3.x or better\n")
    sys.exit(1)

SCRIPTPATH = os.path.dirname(os.path.abspath(sys.argv[0]))
PROJECTPATH = os.path.dirname(SCRIPTPATH)

print(f"SCRIPTPATH={SCRIPTPATH} PROJECTPATH={PROJECTPATH}")


print("We are about to amalgamate all simdjson files into one source file.")
print("See https://www.sqlite.org/amalgamation.html and https://en.wikipedia.org/wiki/Single_Compilation_Unit for rationale.")
if "AMALGAMATE_SOURCE_PATH" not in os.environ:
    AMALGAMATE_SOURCE_PATH = os.path.join(PROJECTPATH, "src")
else:
    AMALGAMATE_SOURCE_PATH = os.environ["AMALGAMATE_SOURCE_PATH"]
if "AMALGAMATE_INCLUDE_PATH" not in os.environ:
    AMALGAMATE_INCLUDE_PATH = os.path.join(PROJECTPATH, "include")
else:
    AMALGAMATE_INCLUDE_PATH = os.environ["AMALGAMATE_INCLUDE_PATH"]
if "AMALGAMATE_OUTPUT_PATH" not in os.environ:
    AMALGAMATE_OUTPUT_PATH = os.path.join(SCRIPTPATH)
else:
    AMALGAMATE_OUTPUT_PATH = os.environ["AMALGAMATE_OUTPUT_PATH"]

def any_match(patterns: Iterable[str], string: str):
    return any([re.match(pattern, string) for pattern in patterns])

class Amalgamator:
    @classmethod
    def amalgamate(cls, output_path: str, file: str, roots: list[str], timestamp: str):
        print(f"Creating {output_path}")
        fid = open(output_path, 'w')
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=fid)
        cls(fid, roots).maybe_write_file(file, "")
        fid.close()

    def __init__(self, fid: TextIO, roots: list[str]):
        self.fid = fid
        self.roots = roots
        self.implementation: Optional[str] = None
        self.found_includes: list[str] = []
        self.found_generic_includes: list[tuple[str, str]] = []

    def is_generic(self, filename: str):
        return filename.startswith('simdjson/generic/')

    def should_write_file(self, filename: str):
        # These files get written out every time they are included
        if any_match([r'.*/begin\.h', r'.*/end\.h'], filename):
            return True

        # These files get written out once per implementation
        elif self.is_generic(filename):
            assert self.implementation is not None
            if (filename, self.implementation) not in self.found_generic_includes:
                self.found_generic_includes.append((filename, self.implementation))
                return True

        # Other files get written out once, period
        elif filename not in self.found_includes:
            self.found_includes.append(filename)
            return True

        return False

    def find_file_root(self, file: str):
        for root in self.roots:
            if os.path.exists(os.path.join(root, file)):
                return root
        return None

    def maybe_write_file(self, filename: str, else_line: str):
        root = self.find_file_root(filename)
        if root:
            # If we're including one of our own files, emit the contents
            if self.should_write_file(filename):
                self.write_file(filename, root)
            else:
                suffix = ""
                if self.is_generic(filename):
                    suffix = f" for {self.implementation}"
                self.write(f"// skipped duplicate {else_line}{suffix}")
        else:
            # If it's an external / system include, just emit the #include
            self.write(else_line)

    def write(self, line: str):
        print(line, file=self.fid)

    def write_file(self, filename: str, root: Optional[str] = None):
        if root is None:
            root = self.find_file_root(filename)
            assert root

        # self.write(f"// dofile: invoked with root={root}, filename={filename}")
        file = os.path.join(root, filename)
        assert os.path.exists(file)
        RELFILE = os.path.relpath(file, PROJECTPATH)
        # Windows use \ as a directory separator, but we do not want that:
        OSRELFILE = RELFILE.replace(r'\\','/')
        # Last lines are always ignored. Files should end by an empty lines.
        if self.is_generic(filename):
            suffix = f" for {self.implementation}"
        else:
            suffix = ""
        self.write(f"/* begin file {OSRELFILE}{suffix} */")
        includepattern = re.compile(r'^#include "(.*)"')
        redefines_simdjson_implementation = re.compile(r'^#define\s+SIMDJSON_IMPLEMENTATION\s+(.*)')
        undefines_simdjson_implementation = re.compile(r'^#undef\s+SIMDJSON_IMPLEMENTATION\s*$')
        uses_simdjson_implementation = re.compile(r'\bSIMDJSON_IMPLEMENTATION\b')
        with open(file, 'r') as fid2:
            ignoring = False
            for line in fid2:
                line = line.rstrip('\n')
                if re.match(r"^\s*// BEGIN NOAMALGAMATE\s*$", line):
                    assert not ignoring
                    ignoring = True
                elif re.match(r"^\s*// END NOAMALGAMATE\s*$", line):
                    assert ignoring
                    ignoring = False
                if ignoring:
                    self.write(f"// skipped amalgamate: {line}")
                    continue

                s = includepattern.search(line)
                if s:
                    includedfile = s.group(1)
                    # include all from simdjson.cpp except simdjson.h
                    if includedfile == "simdjson.h" and filename == "simdjson.cpp":
                        self.write(line)
                        continue

                    if includedfile.startswith('../'):
                        includedfile = includedfile[2:]
                    # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
                    self.maybe_write_file(includedfile, line)
                else:
                    # does it contain a redefinition of SIMDJSON_IMPLEMENTATION ?
                    s=redefines_simdjson_implementation.search(line)
                    if s:
                        old_implementation = self.implementation
                        self.implementation=s.group(1)
                        if old_implementation is None:
                            self.write(f"// defining SIMDJSON_IMPLEMENTATION to \"{self.implementation}\"\n// {line}")
                        else:
                            self.write(f"// redefining SIMDJSON_IMPLEMENTATION from \"{old_implementation}\" to \"{self.implementation}\"\n// {line}")
                    elif undefines_simdjson_implementation.search(line):
                        # Don't include #undef SIMDJSON_IMPLEMENTATION since we're handling it ourselves
                        # print(f"// {line}")
                        assert self.implementation is not None
                        self.write(f"// undefining SIMDJSON_IMPLEMENTATION from \"{self.implementation}\"\n// {line}")
                        self.implementation = None
                        continue
                    elif uses_simdjson_implementation.search(line):
                        assert self.implementation
                        # copy the line, with SIMDJSON_IMPLEMENTATION replace to what it is currently defined to
                        self.write(uses_simdjson_implementation.sub(self.implementation,line))
                    else:
                        self.write(line)
        if self.is_generic(filename):
            suffix = f" for {self.implementation}"
        else:
            suffix = ""
        self.write(f"/* end file {OSRELFILE}{suffix} */")


# Get the generation date from git, so the output is reproducible.
# The %ci specifier gives the unambiguous ISO 8601 format, and
# does not change with locale and timezone at time of generation.
# Forcing it to be UTC is difficult, because it needs to be portable
# between gnu date and busybox date.
try:
    proc = subprocess.run(['git', 'show', '-s', '--format=%ci', 'HEAD'],
                           stdout=subprocess.PIPE)
    print("the commandline is {}".format(proc.args))
    timestamp = proc.stdout.decode('utf-8').strip()
except:
    print("git not found, timestamp based on current time")
    timestamp = str(datetime.datetime.now())
print(f"timestamp is {timestamp}")

os.makedirs(AMALGAMATE_OUTPUT_PATH, exist_ok=True)
AMAL_H = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdjson.h")
AMAL_C = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdjson.cpp")
DEMOCPP = os.path.join(AMALGAMATE_OUTPUT_PATH, "amalgamate_demo.cpp")
README = os.path.join(AMALGAMATE_OUTPUT_PATH, "README.md")

Amalgamator.amalgamate(AMAL_H, "simdjson.h", [AMALGAMATE_INCLUDE_PATH], timestamp)

Amalgamator.amalgamate(AMAL_C, "simdjson.cpp", [AMALGAMATE_SOURCE_PATH, AMALGAMATE_INCLUDE_PATH], timestamp)

# copy the README and DEMOCPP
if SCRIPTPATH != AMALGAMATE_OUTPUT_PATH:
  shutil.copy2(os.path.join(SCRIPTPATH,"amalgamate_demo.cpp"),AMALGAMATE_OUTPUT_PATH)
  shutil.copy2(os.path.join(SCRIPTPATH,"README.md"),AMALGAMATE_OUTPUT_PATH)

print("Done with all files generation.")

print(f"Files have been written to directory: {AMALGAMATE_OUTPUT_PATH}/")
print(subprocess.run(['ls', '-la', AMAL_C, AMAL_H, DEMOCPP, README],
                     stdout=subprocess.PIPE).stdout.decode('utf-8').strip())
print("Done with all files generation.")


#
# Instructions to create demo
#

print("\nGiving final instructions:")
with open(README) as r:
    for line in r:
        print(line)
