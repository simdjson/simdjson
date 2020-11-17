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

# this list excludes the "src/generic headers"
ALLCFILES = ["simdjson.cpp"]

# order matters
ALLCHEADERS = ["simdjson.h"]

found_includes = []

current_implementation=''

def doinclude(fid, file, line):
    p = os.path.join(AMALGAMATE_INCLUDE_PATH, file)
    pi = os.path.join(AMALGAMATE_SOURCE_PATH, file)

    if os.path.exists(p):
        # generic includes are included multiple times
        if re.match('.*generic/.*.h', file):
            dofile(fid, AMALGAMATE_INCLUDE_PATH, file)
        # begin/end_implementation are also included multiple times
        elif re.match('.*/begin.h', file):
            dofile(fid, AMALGAMATE_INCLUDE_PATH, file)
        elif re.match('.*/end.h', file):
            dofile(fid, AMALGAMATE_INCLUDE_PATH, file)
        elif file not in found_includes:
            found_includes.append(file)
            dofile(fid, AMALGAMATE_INCLUDE_PATH, file)
    elif os.path.exists(pi):
        # generic includes are included multiple times
        if re.match('.*generic/.*h', file):
            dofile(fid, AMALGAMATE_SOURCE_PATH, file)
        elif file not in found_includes:
            found_includes.append(file)
            dofile(fid, AMALGAMATE_SOURCE_PATH, file)
        else:
            print(f"//doinclude: {file} already included: {line}")
    else:
        # If we don't recognize it, just emit the #include
        print(line, file=fid)


def dofile(fid, prepath, filename):
    global current_implementation
    # print(f"// dofile: invoked with prepath={prepath}, filename={filename}",file=fid)
    file = os.path.join(prepath, filename)
    RELFILE = os.path.relpath(file, PROJECTPATH)
    # Last lines are always ignored. Files should end by an empty lines.
    print(f"/* begin file {RELFILE} */", file=fid)
    includepattern = re.compile('^#include "(.*)"')
    redefines_simdjson_implementation = re.compile('^#define SIMDJSON_IMPLEMENTATION (.*)')
    uses_simdjson_implementation = re.compile('SIMDJSON_IMPLEMENTATION([^_a-zA-Z0-9]|$)')
    with open(file, 'r') as fid2:
        for line in fid2:
            line = line.rstrip('\n')
            s = includepattern.search(line)
            if s:
                includedfile = s.group(1)
                # include all from simdjson.cpp except simdjson.h
                if includedfile == "simdjson.h" and filename == "simdjson.cpp":
                    print(line, file=fid)
                    continue

                if includedfile.startswith('../'):
                    includedfile = includedfile[2:]
                # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
                doinclude(fid, includedfile, line)
            else:
                # does it contain a redefinition of SIMDJSON_IMPLEMENTATION ?
                s=redefines_simdjson_implementation.search(line)
                if s:
                    current_implementation=s.group(1)
                    print(f"// redefining SIMDJSON_IMPLEMENTATION to \"{current_implementation}\"\n// {line}", file=fid)
                else:
                    # copy the line, with SIMDJSON_IMPLEMENTATION replace to what it is currently defined to
                    print(uses_simdjson_implementation.sub(current_implementation+"\\1",line), file=fid)
    print(f"/* end file {RELFILE} */", file=fid)


# Get the generation date from git, so the output is reproducible.
# The %ci specifier gives the unambiguous ISO 8601 format, and
# does not change with locale and timezone at time of generation.
# Forcing it to be UTC is difficult, because it needs to be portable
# between gnu date and busybox date.
timestamp = subprocess.run(['git', 'show', '-s', '--format=%ci', 'HEAD'],
                           stdout=subprocess.PIPE).stdout.decode('utf-8').strip()
print(f"timestamp is {timestamp}")

os.makedirs(AMALGAMATE_OUTPUT_PATH, exist_ok=True)
AMAL_H = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdjson.h")
AMAL_C = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdjson.cpp")
DEMOCPP = os.path.join(AMALGAMATE_OUTPUT_PATH, "amalgamate_demo.cpp")
README = os.path.join(AMALGAMATE_OUTPUT_PATH, "README.md")

print(f"Creating {AMAL_H}")
amal_h = open(AMAL_H, 'w')
print(f"/* auto-generated on {timestamp}. Do not edit! */", file=amal_h)
for h in ALLCHEADERS:
    doinclude(amal_h, h, f"ERROR {h} not found")

amal_h.close()

print(f"Creating {AMAL_C}")
amal_c = open(AMAL_C, 'w')
print(f"/* auto-generated on {timestamp}. Do not edit! */", file=amal_c)
for c in ALLCFILES:
    doinclude(amal_c, c, f"ERROR {c} not found")

amal_c.close()

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
