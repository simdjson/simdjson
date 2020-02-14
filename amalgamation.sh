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
error.cpp
jsonioutil.cpp
jsonminifier.cpp
jsonparser.cpp
stage1_find_marks.cpp
stage2_build_tape.cpp
document.cpp
document/parser.cpp
"

# order matters
ALLCHEADERS="
simdjson/simdjson_version.h
simdjson/portability.h
simdjson/isadetection.h
simdjson/jsonformatutils.h
simdjson/simdjson.h
simdjson/common_defs.h
simdjson/padded_string.h
simdjson/jsonioutil.h
simdjson/jsonminifier.h
simdjson/document.h
simdjson/document/iterator.h
simdjson/document/parser.h
simdjson/parsedjson.h
simdjson/stage1_find_marks.h
simdjson/stage2_build_tape.h
simdjson/jsonparser.h
simdjson/jsonstream.h
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
  simdjson::padded_string p = simdjson::get_corpus(filename);
  simdjson::document doc;
  if (!simdjson::document::try_parse(p, doc)) { // do the parsing
    std::cout << "document::try_parse not valid" << std::endl;
  } else {
    std::cout << "document::try_parse valid" << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  //JsonStream
  const char * filename2 = argv[2];
  simdjson::padded_string p2 = simdjson::get_corpus(filename2);
  simdjson::document::parser parser;
  simdjson::JsonStream js{p2};
  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;

  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
            parse_res = js.json_parse(parser);
  }

  if( ! parser.is_valid()) {
    std::cout << "JsonStream not valid" << std::endl;
  } else {
    std::cout << "JsonStream valid" << std::endl;
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
