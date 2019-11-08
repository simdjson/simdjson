#!/bin/sh
#
# Builds a corpus from all json files in the source directory.
# The files are renamed to the sha1 of their content, and suffixed
# .json. The files are zipped into a flat file named corpus.zip

set -eu

tmp=$(mktemp -d)

root=$(readlink -f "$(dirname "$0")/..")

find $root -type f -name "*.json" | while read -r json; do
 cp "$json" "$tmp"/$(sha1sum < "$json" |cut -f1 -d' ').json
done

zip --junk-paths -r corpus.zip "$tmp"
rm -rf "$tmp"
