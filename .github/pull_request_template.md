

Our tests check whether you have introduced trailing white space. If such a test fails, please check the "artifacts button" above, which if you click it gives a link to a downloadable file to help you identify the issue. One can also run scripts/remove_trailing_whitespace.sh locally if you have a bash shell.

In this case, I think you just needed to merge master into the branch since this branch probably started before I un-whitespaced master.

If you plan to contribute to simdjson, please read our

CONTRIBUTING guide: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md and our
HACKING guide: https://github.com/simdjson/simdjson/blob/master/HACKING.md
