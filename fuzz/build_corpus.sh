#!/bin/sh
# builds a corpus from all json files in the source directory
# into a flat file named corpus.zip

set -eu

tmp=$(mktemp -d)

root=$(readlink -f "$(dirname "$0")/..")

find $root -type f -name "*.json" | while read -r json; do
 echo json=$json
 cp "$json" "$tmp"/$(sha1sum < "$json" |cut -f1 -d' ').json
done

echo $tmp
zip --junk-paths corpus.zip "$tmp"
rm -rf "$tmp"


