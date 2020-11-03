#!/bin/sh
#
# removes trailing whitespace from source files and other

set -eu

# go to the git root dir
cd "$(git rev-parse --show-toplevel)"

#make a list of all files (null separated, to handle whitespace)
git ls-tree -r HEAD --name-only -z >all_files.null.txt

for suffix in cpp cmake h hpp js md py rb sh ; do
   echo "removing trailing whitespace from files with suffix $suffix"
   cat all_files.null.txt | grep -z '\.'$suffix'$' |xargs -n1 --null sed -i 's/[ \t]*$//'
done
rm all_files.null.txt
echo "done!"

