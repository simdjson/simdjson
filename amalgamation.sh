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

SRCPATH="$SCRIPTPATH/src"
INCLUDEPATH="$SCRIPTPATH/include"

# this list excludes the "src/generic headers"
ALLCFILES="
simdjson.cpp
"

# order matters
ALLCHEADERS="
simdjson.h
"

found_includes=()

for file in ${ALLCFILES}; do
    test -e "$SRCPATH/$file" && continue
    echo "FATAL: source file [$SRCPATH/$file] not found."
    exit 127
done

for file in ${ALLCHEADERS}; do
    test -e "$INCLUDEPATH/$file" && continue
    echo "FATAL: source file [$INCLUDEPATH/$file] not found."
    exit 127
done

function doinclude()
{
    file=$1
    line="${@:2}"
    if [ -f $INCLUDEPATH/$file ]; then
        if [[ ! " ${found_includes[@]} " =~ " ${file} " ]]; then
            found_includes+=("$file")
            dofile $INCLUDEPATH/$file
        fi;
    elif [ -f $SRCPATH/$file ]; then
        # generic includes are included multiple times
        if [[ "${file}" == *'generic/'*'.h' ]]; then
            dofile $SRCPATH/$file
        elif [[ ! " ${found_includes[@]} " =~ " ${file} " ]]; then
            found_includes+=("$file")
            dofile $SRCPATH/$file
        else
            echo "/* $file already included: $line */"
        fi
    else
      # If we don't recognize it, just emit the #include
      echo "$line"
    fi
}

function dofile()
{
    # Last lines are always ignored. Files should end by an empty lines.
    RELFILE=${1#"$SCRIPTPATH/"}
    echo "/* begin file $RELFILE */"
    # echo "#line 8 \"$1\"" ## redefining the line/file is not nearly as useful as it sounds for debugging. It breaks IDEs.
    while IFS= read -r line || [ -n "$line" ];
    do
        if [[ "${line}" == '#include "'*'"'* ]]; then
            file=$(echo $line| cut -d'"' -f 2)

            if [[ "${file}" == '../'* ]]; then
                file=$(echo $file| cut -d'/' -f 2-)
            fi;

            # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
            doinclude $file $line
        else
            # Otherwise we simply copy the line
            echo "$line"
        fi
    done < "$1"
    echo "/* end file $RELFILE */"
}
timestamp=$(date)
echo "Creating ${AMAL_H}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > "${AMAL_H}"
{
    for h in ${ALLCHEADERS}; do
        doinclude $h "ERROR $h not found"
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

    for file in ${ALLCFILES}; do
        dofile "$SRCPATH/$file"
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
  if(argc < 2) {
    std::cerr << "Please specify at least one file name. " << std::endl;
  }
  const char * filename = argv[1];
  simdjson::dom::parser parser;
  auto [doc, error] = parser.load(filename); // do the parsing
  if (error) {
    std::cout << "parse failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
  } else {
    std::cout << "parse valid" << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  // parse_many
  const char * filename2 = argv[2];
  for (auto result : parser.load_many(filename2)) {
    error = result.error();
  }
  if (error) {
    std::cout << "parse_many failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
  } else {
    std::cout << "parse_many valid" << std::endl;
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
echo "c++ -O3 -std=c++17 -pthread -o ${CPPBIN} ${DEMOCPP}  && ./${CPPBIN} ../jsonexamples/twitter.json ../jsonexamples/amazon_cellphones.ndjson"

SINGLEHDR=$SCRIPTPATH/singleheader
echo "Copying files to $SCRIPTPATH/singleheader "
mkdir -p $SINGLEHDR
echo "c++ -O3 -std=c++17 -pthread -o ${CPPBIN} ${DEMOCPP}  && ./${CPPBIN} ../jsonexamples/twitter.json ../jsonexamples/amazon_cellphones.ndjson" > $SINGLEHDR/README.md
cp ${AMAL_C} ${AMAL_H}  ${DEMOCPP} $SINGLEHDR
ls $SINGLEHDR

cd $SINGLEHDR && c++ -O3 -std=c++17 -pthread -o ${CPPBIN} ${DEMOCPP}  && ./${CPPBIN} ../jsonexamples/twitter.json ../jsonexamples/amazon_cellphones.ndjson

lowercase(){
    echo "$1" | tr 'A-Z' 'a-z'
}

OS=`lowercase \`uname\``
