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
import json
from typing import Dict, List, Optional, Set, TextIO, Union, cast

# Check for Python 3, this does not actually work.
if sys.version_info < (3, 0):
    sys.stdout.write("Sorry, requires Python 3.x or better\n")
    sys.exit(1)

rules = """

Amalgamation Rules for simdjson
==============================

This script creates a single compilation unit (amalgamation) from multiple source files,
allowing the entire library to be compiled as one file while supporting multiple CPU
architectures. Implementation-specific code is conditionally included at compile time.

For more details, see HACKING.md.

Key Concepts:
- **Amalgamated File**: A file that is conditionally included in the amalgamation process.
  These are wrapped in #ifndef SIMDJSON_CONDITIONAL_INCLUDE blocks and are included
  based on the target implementation (e.g., ARM64, x86). They include implementation-specific
  files (e.g., arm64.h) and generic files (e.g., under generic/). Amalgamated files have
  associated dependency files (dependencies.h) to track includes.

- **Amalgamator File**: A file that orchestrates the inclusion of amalgamated files.
  Examples: arm64.h, arm64/implementation.h, generic/amalgamated.h. These are not
  themselves amalgamated but control conditional inclusions.

- **Free Dependency File**: A top-level header that is always included unconditionally.
  These do not have dependency files and represent the public API (e.g., main headers).

- **Implementation-Specific File**: A file tied to a specific CPU architecture or
  instruction set (e.g., arm64/, haswell/). These must be amalgamated.

- **Generic File**: A shared file (under generic/ or simdjson/generic/) that contains
  common code included once per implementation.

- **Builtin File**: Special files under simdjson/builtin/ that handle the builtin
  implementation, a fallback/default implementation used when no optimized implementation
  is available.

- **Conditional Include Block**: A section wrapped in #ifndef SIMDJSON_CONDITIONAL_INCLUDE
  for editor-only or implementation-specific content.

Use amalgation_helper.py to generate an HTML report to help you understand the status of each file.

Inclusion Rules:
1. All implementation-specific files must be within SIMDJSON_CONDITIONAL_INCLUDE blocks.

2. Top-level headers (free dependency files) must not be included in any
   SIMDJSON_CONDITIONAL_INCLUDE block.

3. Generic files must be included only in amalgamator files.

4. Amalgamated files can only include other amalgamated files.

5. Free dependency files can only include amalgamator files or other free files,
   not amalgamated files directly.

6. We fail if an implementation-specific file is included more than once in the same block.

7. We fail if a generic file is included more than once per implementation in the same block.

8. Dependency files (dependencies.h) must list all editor-only includes for completeness.

Tips:
- Use <> for includes to search system paths first, avoiding accidental local includes.
- The script validates that all .h and .cpp files are included or deprecated.

If adding a new implementation, edit the IMPLEMENTATIONS list in this script.

"""

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

RelativeRoot = str # Literal['src','include'] # Literal not supported in Python 3.7 (CI)
RELATIVE_ROOTS: List[RelativeRoot] = ['src', 'include' ]
Implementation = str # Literal['arm64', 'fallback', 'haswell', 'icelake', 'ppc64', 'westmere', 'lsx', 'lasx'] # Literal not supported in Python 3.7 (CI)
IMPLEMENTATIONS: List[Implementation] = [ 'arm64', 'haswell', 'icelake', 'lasx', 'lsx', 'ppc64', 'rvv-vls', 'westmere', 'fallback' ]
GENERIC_INCLUDE = "simdjson/generic"
GENERIC_SRC = "generic"
BUILTIN = "simdjson/builtin"
BUILTIN_BEGIN_H = f"{BUILTIN}/begin.h"
BUILTIN_END_H = f"{BUILTIN}/end.h"
IMPLEMENTATION_DETECTION_H = "simdjson/implementation_detection.h"
DEPRECATED_FILES = set(["simdjson/simdjson.h", "simdjson/jsonioutil.h"])

class SimdjsonFile:
    def __init__(self, repository: 'SimdjsonRepository', root: RelativeRoot, include_path: str):
        self.repository = repository
        self.root = root
        self.include_path = include_path
        self.includes: List[SimdjsonFile] = []
        self.included_from: Set[SimdjsonFile] = set()
        self.editor_only_includes: List[SimdjsonFile] = []
        self.editor_only_included_from: Set[SimdjsonFile] = set()
        self.processed: Optional[bool] = None

    def __str__(self):
        return self.include_path

    def dump(self):
        return {
            'root': self.root,
            'include_path': self.include_path,
            'includes': [include.include_path for include in self.includes],
            'included_from': [included_from.include_path for included_from in self.included_from],
            'editor_only_includes': [editor_only_include.include_path for editor_only_include in self.editor_only_includes],
            'editor_only_included_from': [editor_only_included_from.include_path for editor_only_included_from in self.editor_only_included_from],
            'processed': self.processed,
            'dependency_file': self.dependency_file.include_path if self.dependency_file else None,
            'is_amalgamator': self.is_amalgamator,
            'implementation': self.implementation,
        }
    def json(self):
        return json.dumps(self.dump(), indent=4, sort_keys=True, ensure_ascii=False)

    def __repr__(self):
        return self.include_path

    def __lt__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path < other
    def __le__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path <= other
    def __eq__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path == other
    def __ne__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path != other
    def __gt__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path > other
    def __ge__(self, other: Union['SimdjsonFile', str]):
        if isinstance(other, SimdjsonFile):
            other = other.include_path
        return self.include_path >= other
    def __hash__(self):
        return hash(self.include_path)

    @property
    def project_relative_path(self):
        return f"{self.root}/{self.include_path}"

    @property
    def absolute_path(self):
        return os.path.join(self.repository.project_path, self.root, self.include_path)

    @property
    def is_generic(self):
        return self.include_path.startswith('generic/') or self.include_path.startswith('simdjson/generic/')

    @property
    def include_dir(self):
        return os.path.dirname(self.include_path)

    @property
    def filename(self):
        return os.path.basename(self.include_path)

    @property
    def implementation(self) -> Optional[Implementation]:
        match = re.search(f'(^|/)({"|".join(IMPLEMENTATIONS)})', self.include_path)
        if match:
            return cast(Implementation, str(match.group(2)))

    @property
    def dependency_file(self):
        if self.is_dependency_file:
            return None

        if self.implementation:
            # src/arm64.cpp, etc. -> generic/dependencies.h
            if self.include_dir == '':
                return self.repository["generic/dependencies.h"]

            # simdjson/arm64/ondemand.h
            if self.filename == 'ondemand.h':
                return self.repository["simdjson/generic/ondemand/dependencies.h"]
            if self.filename == 'builder.h':
                return self.repository["simdjson/generic/builder/dependencies.h"]

            # simdjson/arm64.h, simdjson/arm64/*.h
            else:
                return self.repository["simdjson/generic/dependencies.h"]

        if self.include_path.startswith('generic/') or self.include_path.startswith('simdjson/generic/'):
            return self.repository[f"{self.include_dir}/dependencies.h"]

        return None

    @property
    def is_amalgamator(self):
        if self.implementation:
            return (self.root == 'src' or
                    self.include_dir == 'simdjson' or
                    self.filename == 'ondemand.h' or
                    self.filename == 'builder.h' or
                    self.filename == 'implementation.h')
        else:
            return self.filename == 'amalgamated.h'

    @property
    def is_free_dependency(self):
        return self.dependency_file is None

    @property
    def is_conditional_include(self):
        return self.dependency_file is not None

    @property
    def is_dependency_file(self):
        return self.filename == 'dependencies.h'

    def add_include(self, include: 'SimdjsonFile'):
        print(f"    Adding include: {self} includes {include}")
        if self.is_conditional_include:
            # If I have a dependency file, I can only include something that has a dependency file.
            assert include.is_conditional_include, f"Error: Amalgamated file '{self}' is trying to include '{include}', but '{include}' is not an amalgamated file. Amalgamated files can only include other amalgamated files to maintain conditional inclusion structure. Check the inclusion rules in the script's 'rules' variable. {rules}"
            # TODO make sure we only include amalgamated files that are guaranteed to be included with us (or before us)
            # if include.amalgamator_file:
            #     assert include.amalgamator_file == self, f"{self} cannot include {include}: it should be included from {include.amalgamator_file} instead."
        else:
            assert include.is_amalgamator or not include.is_conditional_include, f"Error: Free dependency file '{self}' is trying to include '{include}', which is an amalgamated file. Free dependency files (top-level headers) can only include amalgamator files or other free files, not amalgamated files directly. This prevents improper layering. Move the include to an amalgamator or restructure dependencies. {rules}"

        self.includes.append(include)
        include.included_from.add(self)

    def add_editor_only_include(self, include: 'SimdjsonFile'):
        assert self.is_conditional_include, f"Error: File '{self}' uses '#ifndef SIMDJSON_CONDITIONAL_INCLUDE', but '{self}' is not an amalgamated file. Conditional include blocks are only allowed in amalgamated files (those with dependencies). Remove the conditional block or ensure the file is amalgamated. {rules}"
        if not include.is_conditional_include:
            assert self.dependency_file, f"Error: In '{self}', editor-only include of '{include}' requires a dependency file, but '{self}' has none. Ensure '{self}' has an associated dependencies.h file. {rules}"
        # TODO make sure we only include amalgamated files that are guaranteed to be included with us (or before us)
        # elif include.amalgamator_file:
        #     assert self.is_amalgamated_before(self.amalgamator_file), f"{self} cannot include {include}: it should be included from {include.amalgamator_file} instead."

        self.editor_only_includes.append(include)
        include.editor_only_included_from.add(self)

    def validate_free_dependency_file(self):
        if self.is_dependency_file:
            extra_include_set = set(self.includes)
            for file in self.repository:
                if file.dependency_file == self:
                    for editor_only_include in file.editor_only_includes:
                        if not editor_only_include.is_conditional_include:
                            assert editor_only_include in self.includes, f"Error: Dependency file '{self}' is missing an include for '{editor_only_include}', which is editor-only included in '{file}'. Add '{editor_only_include}' to '{self}' to ensure completeness. {rules}"
                            if editor_only_include in extra_include_set:
                                extra_include_set.remove(editor_only_include)

            assert len(extra_include_set) == 0, f"Error: Dependency file '{self}' includes {extra_include_set}, which are not used in any amalgamated files. Remove these unnecessary includes to clean up dependencies. {rules}"

class SimdjsonRepository:
    def __init__(self, project_path: str, relative_roots: List[RelativeRoot]):
        self.project_path = project_path
        self.relative_roots = relative_roots
        self.files: Dict[str, SimdjsonFile] = {}

    def validate_free_dependency_files(self):
        for file in self:
            file.validate_free_dependency_file()

    def __len__(self):
        return len(self.files)

    def __contains__(self, include_path: Union[str,SimdjsonFile]):
        if isinstance(include_path, SimdjsonFile):
            return include_path.include_path in self.files
        else:
            return include_path in self.files

    def __getitem__(self, include_path: str):
        if include_path not in self.files:
            root = self._included_filename_root(include_path)
            if not root:
                return None
            self.files[include_path] = SimdjsonFile(self, root, include_path)
        return self.files[include_path]

    def __iter__(self):
        return iter(self.files.values())

    def _included_filename_root(self, filename: str):
        result = None
        for relative_root in self.relative_roots:
            if os.path.exists(os.path.join(self.project_path, relative_root, filename)):
                assert result is None, f"Error: File '{filename}' exists in both '{result}' and '{relative_root}' directories. Files must be unique across roots to avoid ambiguity. Rename or move the duplicate file."
                result = relative_root
        return result

    def validate_all_files_used(self, root: RelativeRoot):
        assert root in self.relative_roots, f"Error: Root '{root}' is not a valid relative root. Valid roots are {self.relative_roots}. Check the root parameter passed to validate_all_files_used."
        absolute_root = os.path.join(self.project_path, root)
        all_files = set([
            os.path.relpath(os.path.join(dir, file).replace('\\', '/'), absolute_root)
            for dir, _, files in os.walk(absolute_root)
            for file in files
            if file.endswith('.h') or file.endswith('.cpp')
        ])
        used_files = set([file.include_path for file in self if file.root == root])
        all_files.difference_update(used_files)
        all_files.difference_update(DEPRECATED_FILES)
        assert len(all_files) == 0, f"Error: The following files in '{root}' are not used in the amalgamation: {sorted(all_files)}. All .h and .cpp files must be included or added to DEPRECATED_FILES. Check for missing includes or deprecate unused files."

class Amalgamator:
    @classmethod
    def amalgamate(cls, output_path: str, filename: str, roots: List[RelativeRoot], timestamp: str, version: str):
        print(f"Creating {output_path}")
        fid = open(output_path, 'w')
        print(f"/* auto-generated on {timestamp}. version {version} Do not edit! */", file=fid)
        amalgamator = cls(fid, SimdjsonRepository(PROJECTPATH, roots))
        file = amalgamator.repository[filename]
        assert file, f"Error: Starting file '{filename}' not found in paths {[os.path.join(PROJECTPATH, root) for root in roots]}. Ensure the file exists and the paths are correct."
        amalgamator.maybe_write_file(file, None, "")
        amalgamator.repository.validate_free_dependency_files()
        fid.close()
        return amalgamator.repository

    def __init__(self, fid: TextIO, repository: SimdjsonRepository):
        self.fid = fid
        self.repository = repository
        self.builtin_implementation = False
        self.implementation: Optional[str] = None
        self.found_includes: Set[SimdjsonFile] = set()
        self.found_includes_per_conditional_block: Set[SimdjsonFile] = set()
        self.found_generic_includes: List[tuple[SimdjsonFile, str]] = []
        self.in_conditional_include_block = False
        self.editor_only_region = False
        self.include_stack: List[SimdjsonFile] = []

    def maybe_write_file(self, file: SimdjsonFile, including_file: Optional[SimdjsonFile], else_line: str):
        if file.is_conditional_include:
            if file.is_generic:
                # Generic files get written out once per implementation in a well-defined order
                assert (file, self.implementation) not in self.found_generic_includes, f"Error: Generic file '{file}' is being included a second time for implementation '{self.implementation}' from '{including_file}'. Generic files should be included only once per implementation to avoid duplication."
                assert self.implementation, f"Error: Attempting to write generic file '{file}', but no implementation is currently set. Generic files require an active implementation context."
                self.found_generic_includes.append((file, self.implementation))
            else:
                # Other amalgamated files, on the other hand, may only be included once per *amalgamation*
                if file not in self.found_includes_per_conditional_block:
                    self.found_includes_per_conditional_block.add(file)
        else:
            if file in self.found_includes:
                self.write(f"/* skipped duplicate {else_line} */")
                return
            self.found_includes.add(file)

        self.write(f"/* including {self.file_to_str(file)}: {else_line} */")
        self.write_file(file)

    def write(self, line: str):
        print(line, file=self.fid)

    def file_to_str(self, file: SimdjsonFile):
        if file.is_generic and file.is_conditional_include:
            assert self.implementation, f"Error: In file_to_str for '{file}', implementation is not set. This is required for generic conditional files."
            return f"{file} for {self.implementation}"
        return file

    def write_file(self, file: SimdjsonFile):
        # Detect cyclic dependencies
        assert file not in self.include_stack, f"Error: Cyclic include detected: {self.include_stack} -> {file}. Remove the circular dependency by restructuring includes."
        self.include_stack.append(file)

        file.processed = False

        self.write(f"/* begin file {self.file_to_str(file)} */")

        if file == BUILTIN_BEGIN_H:
            assert self.implementation is None, f"Error: Starting builtin implementation, but '{self.implementation}' is already set. Builtin should start with no prior implementation."
            assert not self.builtin_implementation, f"Error: Builtin implementation is already active ({self.builtin_implementation}). Cannot start again."
            self.builtin_implementation = True
            self.implementation = "SIMDJSON_BUILTIN_IMPLEMENTATION"

        assert not self.editor_only_region, f"Error: Already in an editor-only region when starting to write '{file}'. Ensure proper nesting of conditional blocks."
        with open(file.absolute_path, 'r') as fid2:
            for line in fid2:
                line = line.rstrip('\n')

                # Ignore #pragma once, it causes warnings if it ends up in a .cpp file
                if re.search(r'^#pragma once$', line):
                    continue

                # Ignore lines inside #ifndef SIMDJSON_CONDITIONAL_INCLUDE
                if re.search(r'^#ifndef\s+SIMDJSON_CONDITIONAL_INCLUDE\s*$', line):
                    assert file.is_conditional_include, f"Error: File '{file}' uses '#ifndef SIMDJSON_CONDITIONAL_INCLUDE', but it's not an amalgamated file. Conditional includes are only for amalgamated files. {rules}"
                    assert self.in_conditional_include_block, f"Error: File '{file}' uses '#ifndef SIMDJSON_CONDITIONAL_INCLUDE' without a prior '#define SIMDJSON_CONDITIONAL_INCLUDE'. Ensure the define comes first. Stack: {self.include_stack}. {rules}"
                    assert not self.editor_only_region, f"Error: File '{file}' uses '#ifndef SIMDJSON_CONDITIONAL_INCLUDE' twice in a row. Ensure conditional blocks are properly nested and closed. {rules}"
                    self.editor_only_region = True

                # Handle ignored lines (and ending ignore blocks)
                end_ignore = re.search(r'^#endif\s*//\s*SIMDJSON_CONDITIONAL_INCLUDE\s*$', line)
                if self.editor_only_region:
                    self.write(f"/* amalgamation skipped (editor-only): {line} */")

                    # Add the editor-only include so we can check dependencies.h for completeness later
                    included = re.search(r'^#include\s+["<]([^">]*)[">]', line)
                    if included:
                        included_file = self.repository[included.group(1)]
                        if included_file:
                            file.add_editor_only_include(included_file)
                    if end_ignore:
                        self.editor_only_region = False
                    continue

                assert not end_ignore, f"Error: File '{file}' has '#endif // SIMDJSON_CONDITIONAL_INCLUDE' without a matching '#ifndef'. Ensure proper conditional block structure. {rules}"

                # Handle #include lines
                included = re.search(r'^#include\s+["<]([^">]*)[">]', line)
                if included:
                    # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
                    included_file = self.repository[included.group(1)]
                    if included_file:
                        file.add_include(included_file)
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
                elif re.search(r'\bSIMDJSON_IMPLEMENTATION\b', line) and file.include_path != IMPLEMENTATION_DETECTION_H:
                    # copy the line, with SIMDJSON_IMPLEMENTATION replace to what it is currently defined to
                    assert self.implementation, f"Error: In '{file}', line '{line}' uses SIMDJSON_IMPLEMENTATION, but it's not defined. Ensure SIMDJSON_IMPLEMENTATION is set before use. {rules}"
                    line = re.sub(r'\bSIMDJSON_IMPLEMENTATION\b',self.implementation,line)

                # Handle defining and undefining SIMDJSON_CONDITIONAL_INCLUDE
                defined = re.search(r'^#define\s+SIMDJSON_CONDITIONAL_INCLUDE\s*$', line)
                if defined:
                    assert not file.is_conditional_include, f"Error: Amalgamated file '{file}' defines SIMDJSON_CONDITIONAL_INCLUDE, which is not allowed. Only non-amalgamated files can define it. {rules}"
                    assert not self.in_conditional_include_block, f"Error: File '{file}' redefines SIMDJSON_CONDITIONAL_INCLUDE while already in a conditional block. Avoid redefinition. {rules}"
                    self.in_conditional_include_block = True
                    self.found_includes_per_conditional_block.clear()
                    self.write(f'/* defining SIMDJSON_CONDITIONAL_INCLUDE */')
                elif re.search(r'^#undef\s+SIMDJSON_CONDITIONAL_INCLUDE\s*$', line):
                    assert not file.is_conditional_include, f"Error: Amalgamated file '{file}' undefines SIMDJSON_CONDITIONAL_INCLUDE, which is not allowed. Only non-amalgamated files can undefine it. {rules}"
                    assert self.in_conditional_include_block, f"Error: File '{file}' undefines SIMDJSON_CONDITIONAL_INCLUDE without having defined it first. Ensure proper define/undefine pairing. {rules}"
                    self.write(f'/* undefining SIMDJSON_CONDITIONAL_INCLUDE */')
                    self.in_conditional_include_block = False

                self.write(line)

            assert not self.editor_only_region, f"Error: File '{file}' ended without closing the '#endif // SIMDJSON_CONDITIONAL_INCLUDE'. Ensure all conditional blocks are properly closed. {rules}"

        self.write(f"/* end file {self.file_to_str(file)} */")

        if file.include_path == BUILTIN_BEGIN_H:
            # begin.h redefined SIMDJSON_IMPLEMENTATION multiple times
            assert self.builtin_implementation, f"Error: Processing BUILTIN_BEGIN_H, but builtin implementation is not active."
            self.implementation = "SIMDJSON_BUILTIN_IMPLEMENTATION"
        elif file.include_path == BUILTIN_END_H:
            assert self.implementation is None, f"Error: Processing BUILTIN_END_H, but implementation is still set to '{self.implementation}'. It should be None."
            assert self.builtin_implementation, f"Error: Processing BUILTIN_END_H, but builtin implementation is not active."
            self.implementation = None

        file.processed = True

        self.include_stack.pop()

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

def validate_implementations():
    """Scan include/simdjson/*/implementation.h and compare with IMPLEMENTATIONS list."""
    include_simdjson_path = os.path.join(PROJECTPATH, 'include', 'simdjson')
    found_implementations = set()

    if os.path.exists(include_simdjson_path):
        for item in os.listdir(include_simdjson_path):
            item_path = os.path.join(include_simdjson_path, item)
            if os.path.isdir(item_path):
                impl_h_path = os.path.join(item_path, 'implementation.h')
                if os.path.exists(impl_h_path) and item != 'builtin':  # Ignore builtin
                    found_implementations.add(item)

    expected_implementations = set(IMPLEMENTATIONS)
    if found_implementations != expected_implementations:
        missing = expected_implementations - found_implementations
        extra = found_implementations - expected_implementations
        warning = "Warning: IMPLEMENTATIONS list does not match found implementations.\n"
        if missing:
            warning += f"Missing in filesystem: {sorted(missing)}\n"
        if extra:
            warning += f"Extra in filesystem: {sorted(extra)}\n"
        print(warning)
        return False
    return True

def read_version():
    with open(os.path.join(PROJECTPATH, 'include/simdjson/simdjson_version.h')) as f:
        return re.search(r'\d+\.\d+\.\d+', f.read()).group(0)

version = read_version()
if not validate_implementations():
    print("Validation failed. Please update IMPLEMENTATIONS list in amalgamate.py.")
    sys.exit(1)
else:
    print("implementation validated")
Amalgamator.amalgamate(AMAL_H, "simdjson.h", ['include'], timestamp, version).validate_all_files_used('include')
Amalgamator.amalgamate(AMAL_C, "simdjson.cpp", ['src', 'include'], timestamp, version).validate_all_files_used('src')

# copy the README and DEMOCPP
if SCRIPTPATH != AMALGAMATE_OUTPUT_PATH:
  shutil.copy2(os.path.join(SCRIPTPATH,"amalgamate_demo.cpp"),AMALGAMATE_OUTPUT_PATH)
  shutil.copy2(os.path.join(SCRIPTPATH,"README.md"),AMALGAMATE_OUTPUT_PATH)




def create_zip():
    import zipfile
    outdir = AMALGAMATE_OUTPUT_PATH

    path = os.path.join(outdir, "singleheader.zip")
    print(f"Creating {path}")
    with zipfile.ZipFile(path, 'w') as zf:
        for name in ["simdjson.cpp", "simdjson.h"]:
            source = os.path.join(outdir, name)
            print(f"Adding {source}")
            zf.write(source, name)
    print(f"Created {path}")
create_zip()
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
