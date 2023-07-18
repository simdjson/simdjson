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
from typing import Dict, Iterable, List, Optional, Set, TextIO

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

BUILTIN_BEGIN_H = "include/simdjson/builtin/begin.h"
BUILTIN_END_H = "include/simdjson/builtin/end.h"
IMPLEMENTATION_DETECTION_H = "include/simdjson/implementation_detection.h"

def any_match(patterns: Iterable[str], string: str):
    return any([re.match(pattern, string) for pattern in patterns])

class DirectoryDependencies:
    def __init__(self, dependencies_file: str, amalgamated_dependencies: List[str]):
        self.dependencies_file = dependencies_file
        # Dependencies from {directory}/dependencies.h
        self.amalgamated_dependencies = amalgamated_dependencies
        # filename -> editor-only dependencies
        self.extra_dependencies = set(amalgamated_dependencies)

    def add_editor_only_dependency(self, file: str, included_file: str):
        dir = os.path.dirname(file)
        # Exclude the base.h hack to include begin.h and implementation_selection.h
        if file.endswith('/base.h') and (included_file.endswith('/begin.h') or included_file == IMPLEMENTATION_DETECTION_H):
            return
        # Includes of generic files don't need to be in dependencies (they need to be in amalgamated.h, really, though we're not checking that):
        if included_file.startswith('include/simdjson/generic/') or included_file.startswith('src/generic/'):
            return
        assert included_file in self.amalgamated_dependencies, f"{included_file} is included in {file} but not in {self.dependencies_file} (which has {self.amalgamated_dependencies})!"
        if included_file in self.extra_dependencies:
            self.extra_dependencies.remove(included_file)

class Amalgamator:
    @classmethod
    def amalgamate(cls, output_path: str, file: str, roots: List[str], timestamp: str):
        print(f"Creating {output_path}")
        fid = open(output_path, 'w')
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=fid)
        amalgamator = cls(fid, roots)
        amalgamator.maybe_write_file(file, "", "")
        amalgamator.validate_dependencies()
        fid.close()

    def __init__(self, fid: TextIO, roots: List[str]):
        self.fid = fid
        self.roots = roots
        self.builtin_implementation = False
        self.implementation: Optional[str] = None
        self.found_includes: List[str] = []
        self.found_generic_includes: List[tuple[str, str]] = []
        self.amalgamated_defined = False
        self.editor_only_region = False
        self.include_stack: List[str] = []
        self.dependencies: Dict[str, DirectoryDependencies] = {}

    def is_generic(self, file: str):
        return (
            file.startswith('include/simdjson/generic/') or
            file.startswith('src/generic/')
        ) and not file.endswith('/dependencies.h')


    def find_file_root(self, filename: str):
        result = None
        for root in self.roots:
            if os.path.exists(os.path.join(root, filename)):
                assert result is None, "{file} exists in both {result} and {root}!"
                result = root

        return result

    def maybe_write_file(self, file: str, including_file: str, else_line: str):
        # These files get written out every time they are included, and must be included during SIMDJSON_AMALGAMATED
        if re.search(r'(/|^)(arm64|fallback|haswell|icelake|ppc64|westmere)(\.\w+|)?(/|$)', file):
            assert self.amalgamated_defined, f"{file} included from {including_file} without defining SIMDJSON_AMALGAMATED!"
        elif self.is_generic(file):
            # Generic files may only be included from their actual implementations.
            assert re.search(r'(/|^)(arm64|fallback|generic|haswell|icelake|ppc64|westmere)(\.\w+|)?(/|$)', file)
            # Generic files can only ever be written out once for simdjson.h and once for simdjson.cpp
            assert self.amalgamated_defined, f"{file} included from {including_file} without defining SIMDJSON_AMALGAMATED!"
            assert self.implementation is not None, f"generic file {file} included from {including_file} without defining SIMDJSON_IMPLEMENTATION!"
            assert (file, self.implementation) not in self.found_generic_includes, f"generic file {file} included from {including_file} a second time for {self.implementation}!"
            self.found_generic_includes.append((file, self.implementation))
        else:
            # Other files are only output once, and may not be included during SIMDJSON_AMALGAMATED
            assert not self.amalgamated_defined, f"{file} included from {including_file} while SIMDJSON_AMALGAMATED is defined!"
            if file in self.found_includes:
                self.write(f"/* skipped duplicate {else_line} */")
                return
            self.found_includes.append(file)

        self.write(f"/* including {self.file_to_str(file)}: {else_line} */")
        self.write_file(file)

    def write(self, line: str):
        print(line, file=self.fid)

    def absolute_file_path(self, filename: str):
        root = self.find_file_root(filename)
        assert root

        file = os.path.join(root, filename)
        assert os.path.exists(file)
        # Windows use \ as a directory separator, but we do not want that:
        return file.replace('\\', '/')

    def project_relative_file(self, filename: str):
        file = os.path.relpath(self.absolute_file_path(filename), PROJECTPATH)
        # Windows use \ as a directory separator, but we do not want that:
        return file.replace('\\','/')

    def file_to_str(self, file: str):
        if self.is_generic(file):
            assert self.implementation
            return f"{file} for {self.implementation}"
        return file

    def write_file(self, file: str):
        # Detect cyclic dependencies
        assert file not in self.include_stack, f"Cyclic include: {self.include_stack} -> {file}"
        self.include_stack.append(file)

        dir = os.path.dirname(file)
        dependencies: List[str] = []

        self.write(f"/* begin file {self.file_to_str(file)} */")

        if file == BUILTIN_BEGIN_H:
            assert self.implementation is None
            assert not self.builtin_implementation
            self.builtin_implementation = True
            self.implementation = "SIMDJSON_BUILTIN_IMPLEMENTATION"

        assert not self.editor_only_region
        with open(os.path.join(PROJECTPATH, file), 'r') as fid2:
            for line in fid2:
                line = line.rstrip('\n')

                # Ignore lines inside #ifndef SIMDJSON_AMALGAMATED
                if re.search(r'^#ifndef\s+SIMDJSON_AMALGAMATED\s*$', line):
                    assert self.amalgamated_defined, f"{file} uses #ifndef SIMDJSON_AMALGAMATED without a prior #define SIMDJSON_AMALGAMATED: {self.include_stack}"
                    assert not self.editor_only_region, f"{file} uses #ifndef SIMDJSON_AMALGAMATED twice in a row"
                    self.editor_only_region = True

                # Handle ignored lines (and ending ignore blocks)
                end_ignore = re.search(r'^#endif\s*//\s*SIMDJSON_AMALGAMATED\s*$', line)
                if self.editor_only_region:
                    self.write(f"/* amalgamation skipped (editor-only): {line} */")

                    # Includes in here are "local includes" which must be added to dependencies.h but not processed.
                    included = re.search(r'^#include "([^"]*)"', line)
                    if included:
                        included_file = self.project_relative_file(included.group(1))
                        if file.startswith('include/simdjson/generic/') or file.startswith('src/generic/'):
                            generic_dir = dir
                        elif dir == 'src' or dir == 'include':
                            generic_dir = f"{dir}/generic"
                        else:
                            generic_dir = f"{os.path.dirname(dir)}/generic"
                        assert generic_dir in self.dependencies, f"{file} included before {generic_dir}/dependencies.h: {self.include_stack}!"
                        # Track all local dependencies so we can check at the end
                        self.dependencies[generic_dir].add_editor_only_dependency(file, included_file)

                    if end_ignore:
                        self.editor_only_region = False
                    continue
                assert not end_ignore, f"{file} has #endif // SIMDJSON_AMALGAMATED without #ifndef SIMDJSON_AMALGAMATED"

                # Handle #include lines
                included = re.search(r'^#include "([^"]*)"', line)
                if included:
                    included_file = self.project_relative_file(included.group(1))
                    dependencies.append(included_file)
                    # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
                    self.maybe_write_file(included_file, file, line)
                    continue

                # Handle defining and replacing SIMDJSON_IMPLEMENTATION
                defined = re.search(r'^#define\s+SIMDJSON_IMPLEMENTATION\s+(.+)$', line)
                if defined:
                    old_implementation = self.implementation
                    self.implementation = defined.group(1)
                    if old_implementation is None:
                        self.write(f'/* defining SIMDJSON_IMPLEMENTATION to "{self.implementation}" */')
                    else:
                        self.write(f'/* redefining SIMDJSON_IMPLEMENTATION from "{old_implementation}" to "{self.implementation}" */')
                elif re.search(r'^#undef\s+SIMDJSON_IMPLEMENTATION\s*$', line):
                    # Don't include #undef SIMDJSON_IMPLEMENTATION since we're handling it ourselves
                    self.write(f'/* undefining SIMDJSON_IMPLEMENTATION from "{self.implementation}" */')
                    self.implementation = None
                elif re.search(r'\bSIMDJSON_IMPLEMENTATION\b', line) and file != IMPLEMENTATION_DETECTION_H:
                    # copy the line, with SIMDJSON_IMPLEMENTATION replace to what it is currently defined to
                    assert self.implementation, f"Use of SIMDJSON_IMPLEMENTATION while not defined in {file}: {line}"
                    line = re.sub(r'\bSIMDJSON_IMPLEMENTATION\b',self.implementation,line)

                # Handle defining and undefining SIMDJSON_AMALGAMATED
                defined = re.search(r'^#define\s+SIMDJSON_AMALGAMATED\s*$', line)
                if defined:
                    assert not self.amalgamated_defined, f"{file} redefines SIMDJSON_AMALGAMATED"
                    self.amalgamated_defined = True
                    self.write(f'/* defining SIMDJSON_AMALGAMATED */')
                elif re.search(r'^#undef\s+SIMDJSON_AMALGAMATED\s*$', line):
                    assert self.amalgamated_defined, f"{file} undefines SIMDJSON_AMALGAMATED without defining it"
                    self.write(f'/* undefining SIMDJSON_AMALGAMATED */')
                    self.amalgamated_defined = False

                self.write(line)

            assert not self.editor_only_region, f"{file} ended without #endif // SIMDJSON_AMALGAMATED"

        self.write(f"/* end file {self.file_to_str(file)} */")

        if file == 'include/simdjson/builtin/begin.h':
            # begin.h redefined SIMDJSON_IMPLEMENTATION multiple times
            assert self.builtin_implementation
            self.implementation = "SIMDJSON_BUILTIN_IMPLEMENTATION"
        elif file == 'include/simdjson/builtin/end.h':
            assert self.implementation is None
            assert self.builtin_implementation
            self.implementation = None

        self.include_stack.pop()

        if file.endswith('/dependencies.h'):
            self.dependencies[dir] = DirectoryDependencies(file, dependencies)

    def validate_dependencies(self):
        for dependencies in self.dependencies.values():
            assert len(dependencies.extra_dependencies) == 0, f"{dependencies.dependencies_file} includes unnecessary files not included in {os.path.dirname(dependencies.dependencies_file)}/*.h: {dependencies.extra_dependencies}"

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

Amalgamator.amalgamate(AMAL_H, "include/simdjson.h", [AMALGAMATE_INCLUDE_PATH], timestamp)
Amalgamator.amalgamate(AMAL_C, "src/simdjson.cpp", [AMALGAMATE_SOURCE_PATH, AMALGAMATE_INCLUDE_PATH], timestamp)

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
