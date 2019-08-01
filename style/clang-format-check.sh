#!/bin/bash
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
BASE=$SCRIPTPATH/..
cd $BASE

STYLE=$(which clang-format)
if [ $? -ne 0 ]; then
	echo "clang-format not installed. Unable to check source file format policy." >&2
	exit 1
fi
OURSTYLE='' # defer to .clang-format
OURCONTENT="include benchmark tools tests src"
RE=0
ALLFILES=$(find $OURCONTENT -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.cc' -o -name '*.hh')
for FILE in $ALLFILES; do
  echo "checking $FILE"
  eval "$STYLE  $OURSTYLE $BASE/$FILE" | cmp -s $BASE/$FILE -
  if [ $? -ne 0 ]; then
        echo "$BASE/$FILE does not respect the coding style." >&2
        echo "consider typing $STYLE -i $BASE/$FILE $OURSTYLE to fix the problem." >&2
        RE=1
  fi
done

exit $RE
