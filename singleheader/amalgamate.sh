#!/usr/bin/env bash
########################################################################
# Generates an "amalgamation build" for simdjson. Inspired by similar
# script used by whefs.
########################################################################
set -e


SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
PROJECTPATH="$(dirname $SCRIPTPATH)"
echo "Project at "$PROJECTPATH

echo "We are about to amalgamate all simdjson files into one source file. "
echo "See https://www.sqlite.org/amalgamation.html and https://en.wikipedia.org/wiki/Single_Compilation_Unit for rationale. "

if [ -z "$AMALGAMATE_SOURCE_PATH" ]; then AMALGAMATE_SOURCE_PATH="$PROJECTPATH/src"; fi
if [ -z "$AMALGAMATE_INCLUDE_PATH" ]; then AMALGAMATE_INCLUDE_PATH="$PROJECTPATH/include"; fi
if [ -z "$AMALGAMATE_OUTPUT_PATH" ]; then AMALGAMATE_OUTPUT_PATH="$SCRIPTPATH"; fi

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
    test -e "$AMALGAMATE_SOURCE_PATH/$file" && continue
    echo "FATAL: source file [$AMALGAMATE_SOURCE_PATH/$file] not found."
    exit 127
done

for file in ${ALLCHEADERS}; do
    test -e "$AMALGAMATE_INCLUDE_PATH/$file" && continue
    echo "FATAL: source file [$AMALGAMATE_INCLUDE_PATH/$file] not found."
    exit 127
done

function doinclude()
{
    file=$1
    line="${@:2}"
    if [ -f $AMALGAMATE_INCLUDE_PATH/$file ]; then
        if [[ ! " ${found_includes[@]} " =~ " ${file} " ]]; then
            found_includes+=("$file")
            dofile $AMALGAMATE_INCLUDE_PATH $file
        fi
    elif [ -f $AMALGAMATE_SOURCE_PATH/$file ]; then
        # generic includes are included multiple times
        if [[ "${file}" == *'generic/'*'.h' ]]; then
            dofile $AMALGAMATE_SOURCE_PATH $file
        # begin/end_implementation are also included multiple times
        elif [[ "${file}" == *'begin_implementation.h' ]]; then
            dofile $AMALGAMATE_SOURCE_PATH $file
        elif [[ "${file}" == *'end_implementation.h' ]]; then
            dofile $AMALGAMATE_SOURCE_PATH $file
        elif [[ ! " ${found_includes[@]} " =~ " ${file} " ]]; then
            found_includes+=("$file")
            dofile $AMALGAMATE_SOURCE_PATH $file
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
    file="$1/$2"
    RELFILE=${file#"$PROJECTPATH/"}
    # Last lines are always ignored. Files should end by an empty lines.
    echo "/* begin file $RELFILE */"
    # echo "#line 8 \"$1\"" ## redefining the line/file is not nearly as useful as it sounds for debugging. It breaks IDEs.
    while IFS= read -r line || [ -n "$line" ];
    do
        if [[ "${line}" == '#include "'*'"'* ]]; then
            file=$(echo $line| cut -d'"' -f 2)
            # include all from simdjson.cpp except simdjson.h
            if [ "${file}" == "simdjson.h" ] && [ "${2}" == "simdjson.cpp" ]; then
                echo "$line"
                continue
            fi

            if [[ "${file}" == '../'* ]]; then
                file=$(echo $file| cut -d'/' -f 2-)
            fi

            # we explicitly include simdjson headers, one time each (unless they are generic, in which case multiple times is fine)
            doinclude $file $line
        else
            # Otherwise we simply copy the line
            echo "$line"
        fi
    done < "$file"
    echo "/* end file $RELFILE */"
}

timestamp=$(date)
mkdir -p $AMALGAMATE_OUTPUT_PATH

AMAL_H="${AMALGAMATE_OUTPUT_PATH}/simdjson.h"
AMAL_C="${AMALGAMATE_OUTPUT_PATH}/simdjson.cpp"
DEMOCPP="${AMALGAMATE_OUTPUT_PATH}/amalgamate_demo.cpp"
README="${AMALGAMATE_OUTPUT_PATH}/README.md"

echo "Creating ${AMAL_H}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > ${AMAL_H}
{
    for h in ${ALLCHEADERS}; do
        doinclude $h "ERROR $h not found"
    done
} >> ${AMAL_H}


echo "Creating ${AMAL_C}..."
echo "/* auto-generated on ${timestamp}. Do not edit! */" > ${AMAL_C}
{
    for file in ${ALLCFILES}; do
        dofile $AMALGAMATE_SOURCE_PATH $file
    done
} >> ${AMAL_C}

# copy amalgamate_demo.cpp and README.md only if AMALGAMATE_OUTPUT_PATH is not the same as SCRIPTPATH
if [ ! ${SCRIPTPATH} -ef ${AMALGAMATE_OUTPUT_PATH} ]; then
    cp -f "${SCRIPTPATH}/amalgamate_demo.cpp" "${DEMOCPP}"
    cp -f "${SCRIPTPATH}/README.md" "${README}"
fi

echo "Done with all files generation."

echo "Files have been written to directory: ${AMALGAMATE_OUTPUT_PATH}/"
ls -la ${AMAL_C} ${AMAL_H} ${DEMOCPP} ${README}

#
# Instructions to create demo
#
echo ""
echo "Giving final instructions:"

cat ${README}

lowercase(){
    echo "$1" | tr 'A-Z' 'a-z'
}

OS=`lowercase \`uname\``
