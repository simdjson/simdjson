#!/bin/bash
########################################################################
# Generates an "amalgamation build" for roaring. Inspired by similar
# script used by whefs.
########################################################################
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

echo "We are about to amalgamate all simdjson files into one source file. "
echo "See https://www.sqlite.org/amalgamation.html and https://en.wikipedia.org/wiki/Single_Compilation_Unit for rationale. "

AMAL_H="simdjson.h"
AMAL_C="simdjson.cpp"

# order does not matter
ALLCFILES=$( ( [ -d $SCRIPTPATH/.git ] && ( type git >/dev/null 2>&1 ) &&  ( git ls-files $SCRIPTPATH/src/*.c $SCRIPTPATH/src/**/*c ) ) ||  ( find $SCRIPTPATH/src -name '*.c' ) )

# order matters
ALLCHEADERS="
$SCRIPTPATH/include/simdjson/portability.h
$SCRIPTPATH/include/simdjson/common_defs.h
$SCRIPTPATH/include/simdjson/jsoncharutils.h
$SCRIPTPATH/include/simdjson/jsonformatutils.h
$SCRIPTPATH/include/simdjson/jsonioutil.h
$SCRIPTPATH/include/simdjson/simdprune_tables.h
$SCRIPTPATH/include/simdjson/simdutf8check.h
$SCRIPTPATH/include/simdjson/jsonminifier.h
$SCRIPTPATH/include/simdjson/parsedjson.h
$SCRIPTPATH/include/simdjson/stage1_find_marks.h
$SCRIPTPATH/include/simdjson/stage2_flatten.h
$SCRIPTPATH/include/simdjson/stringparsing.h
$SCRIPTPATH/include/simdjson/numberparsing.h
$SCRIPTPATH/include/simdjson/stage34_unified.h
$SCRIPTPATH/include/simdjson/jsonparser.h
"

for i in ${ALLCHEADERS} ${ALLCFILES}; do
    test -e $i && continue
    echo "FATAL: source file [$i] not found."
    exit 127
done


function stripinc()
{
    sed -e '/# *include *"/d' -e '/# *include *<simdjson\//d'
}
function dofile()
{
    echo "/* begin file $1 */"
    # echo "#line 8 \"$1\"" ## redefining the line/file is not nearly as useful as it sounds for debugging. It breaks IDEs.
    stripinc < $1
    echo "/* end file $1 */"
}

timestamp=$(date)
echo "Creating ${AMAL_H}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > "${AMAL_H}"
{
    for h in ${ALLCHEADERS}; do
        dofile $h
    done
} >> "${AMAL_H}"


echo "Creating ${AMAL_C}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > "${AMAL_C}"
{
    echo "#include \"${AMAL_H}\""

    echo ""
    echo "/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */"
    echo "#ifdef DMALLOC"
    echo "#include \"dmalloc.h\""
    echo "#endif"
    echo ""

    for h in ${ALLCFILES}; do
        dofile $h
    done
} >> "${AMAL_C}"



DEMOCPP="amalgamation_demo.cpp"
echo "Creating ${DEMOCPP}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > "${DEMOCPP}"
cat <<< '
#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  const char * filename = argv[1]; 
  std::string_view p = get_corpus(filename);
  ParsedJson pj = build_parsed_json(p); // do the parsing
  if( ! pj.isValid() ) {
    std::cout << "not valid" << std::endl;
  } else {
    std::cout << "valid" << std::endl;
  }
  return EXIT_SUCCESS;
}
' >>  "${DEMOCPP}"

echo "Done with all files generation. "

echo "Files have been written to  directory: $PWD "
ls -la ${AMAL_C} ${AMAL_H}  ${DEMOCPP}

echo "Giving final instructions:"


CPPBIN=${DEMOCPP%%.*}

echo "Try :"
echo "c++ -march=native -O3 -std=c++11 -o ${CPPBIN} ${DEMOCPP}  && ./${CPPBIN} "

lowercase(){
    echo "$1" | tr 'A-Z' 'a-z'
}

OS=`lowercase \`uname\``

