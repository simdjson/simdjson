#!/bin/bash
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
BASE=$SCRIPTPATH/..
cd $BASE

STYLE=$(which clang-format)
if [ $? -ne 0 ]; then
	echo "clang-format not installed. Unable to check source file format policy." >&2
	exit 1
fi
OURSTYLE="" # defer to .clang-format
OURCONTENT="include benchmark tools tests src"
RE=0
BASE=$(git rev-parse --show-toplevel)
ALLFILES=$(find $OURCONTENT -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.cc' -o -name '*.hh')
for FILE in $ALLFILES; do
  eval "$STYLE $OURSTYLE $BASE/$FILE" | cmp -s $BASE/$FILE -
  if [ $? -ne 0 ]; then
        echo "$BASE/$FILE does not respect the coding style. Formatting. " >&2
        eval "$STYLE $OURSTYLE -i $BASE/$FILE"
        RE=1
  fi
done

exit $RE
