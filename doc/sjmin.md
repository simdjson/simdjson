# sjmin: minify JSON from the command line

`sjmin` removes the insignificant white space from a JSON document. It is a
thin command-line front end to `simdjson::minify`, which uses the same SIMD
kernels as the parser. The manual page is [`man/sjmin.1`](../man/sjmin.1).

```console
$ printf '{ "name" : "Ada Lovelace", "tags" : [ 1, 2 ] }\n' | sjmin
{"name":"Ada Lovelace","tags":[1,2]}
```

- [Building and installing](#building-and-installing)
- [Usage](#usage)
- [Options](#options)
- [Exit status](#exit-status)
- [Examples](#examples)
- [What sjmin does and does not do](#what-sjmin-does-and-does-not-do)
- [Using the library instead](#using-the-library-instead)

## Building and installing

`sjmin` is built in developer mode, or on its own with
`SIMDJSON_BUILD_TOOLS`:

```bash
cmake -B build -D SIMDJSON_BUILD_TOOLS=ON
cmake --build build --target sjmin
cmake --install build
```

The install step copies `bin/sjmin` and the manual page
`share/man/man1/sjmin.1` under the install prefix (the paths follow
`GNUInstallDirs`). With `SIMDJSON_BUILD_TOOLS=ON`, the
[`sjq`](../tools/sjq/README.md) processor is built and installed as well;
pass `-D SIMDJSON_SJQ=OFF` to build `sjmin` alone.

`sjmin` has no dependency other than simdjson. It does not need exceptions.

## Usage

```
sjmin [-o OUT_FILE] [FILE]
sjmin -h | --help
sjmin -V | --version
```

`sjmin` reads `FILE`, or standard input when `FILE` is absent or is `-`,
and writes the minified document to standard output. It reads one input:
a second file name is a usage error.

No line feed is written after the document. Add one with `echo` if a
terminal or a line-oriented tool needs it:

```bash
sjmin data.json; echo
```

On Windows, standard input and standard output are switched to binary mode,
so the bytes are copied as they are, without line-ending translation.

## Options

| Option | Description |
|--------|-------------|
| `-o FILE`, `--output FILE` | Write the result to `FILE` instead of standard output. `FILE` may be the input file: the input is read completely before the output is opened. `-o -` means standard output. |
| `-h`, `--help` | Print a short help to standard output and exit with status 0. |
| `-V`, `--version` | Print the version and exit with status 0. |
| `--` | End of options: the next argument is a file name, even if it starts with `-`. |

The version line names the program and the simdjson release it was built
from:

```console
$ sjmin --version
sjmin (simdjson) 5.3.0
```

Options can appear before or after the file name. Short options cannot be
combined (`-ho` is an unknown option) and the file name after `-o` is a
separate argument (`-o out.json`, not `-oout.json` or `--output=out.json`).
An unknown option or a missing file name after `-o` prints a message and a
hint to standard error:

```console
$ sjmin --pretty data.json
sjmin: unknown option --pretty
Try 'sjmin --help' for more information.
```

## Exit status

| Status | Meaning |
|--------|---------|
| 0 | Success. `--help` and `--version` also exit with 0. |
| 1 | The input could not be read, the input contains a string that is not closed, or the output could not be opened or written. |
| 2 | Invalid command line: unknown option, missing file name after `-o`, or more than one input file. |

Error messages go to standard error and start with `sjmin:`. When reading
or minifying an input fails, the message also names the input (`-` for
standard input):

```console
$ printf '{"a": "unterminated' | sjmin
sjmin: -: UNCLOSED_STRING: A string is opened, but never closed.
$ sjmin missing.json
sjmin: missing.json: IO_ERROR: Error reading the file.
```

Nothing is written to the output when the input fails. The output file is
opened only after the document has been minified, so an error in the input
leaves an existing output file untouched.

## Examples

Minify a file to standard output:

```bash
sjmin data.json
```

Minify a download into a file:

```bash
curl -s https://example.com/data.json | sjmin -o data.min.json
```

Minify a file in place:

```bash
sjmin -o data.json data.json
```

Minify every `.json` file in a directory:

```bash
for f in *.json; do sjmin -o "${f%.json}.min.json" "$f"; done
```

Read a file whose name starts with a dash:

```bash
sjmin -- -weird-name.json
```

Validate and minify, stopping on invalid JSON (with
[`sjq`](../tools/sjq/README.md) or `jq`):

```bash
sjq empty data.json && sjmin data.json
```

## What sjmin does and does not do

`sjmin` removes the four JSON white-space characters (space, tab, line feed
and carriage return) when they appear outside of strings. Everything else
is copied byte for byte:

- strings, including their escape sequences and the white space inside
  them, are unchanged;
- numbers keep their spelling (`1.0E+2` stays `1.0E+2`);
- keys keep their order, and duplicate keys are kept;
- a UTF-8 byte order mark at the start of the input is kept.

Minifying is idempotent: running `sjmin` on its own output gives the same
bytes.

`sjmin` does **not** parse or validate the document. Only one error in the
content is detected: a string that is not closed before the end of the
input. In particular:

- Invalid JSON produces invalid output, and removing white space can change
  its meaning. `[1 2]` becomes `[12]`, and `nul l` becomes `null`. Validate
  first if the input is not trusted.
- The UTF-8 encoding is not checked.
- An empty input gives an empty output and exit status 0.
- Several documents in one input, as in [JSON Lines or
  NDJSON](https://jsonlines.org), are glued together, because the line feeds
  between them are removed:

  ```console
  $ printf '{"a":1}\n{"b":2}\n' | sjmin
  {"a":1}{"b":2}
  ```

  Minify such files one line at a time, or use `sjq -c .`, which writes one
  compact document per line.

The whole input is held in memory, with its minified copy: expect to use a
little more than twice the size of the document.

## Using the library instead

`sjmin` is a wrapper around one function. In C++, call it directly:

```cpp
#include "simdjson.h"

simdjson::padded_string json;
auto error = simdjson::padded_string::load("data.json").get(json);
if (error) {
  // the file could not be read
}
std::unique_ptr<char[]> buffer{new char[json.size()]};
size_t new_length{};
error = simdjson::minify(json.data(), json.size(), buffer.get(), new_length);
if (error) {
  // the input contains an unclosed string
}
// buffer.get() now holds new_length bytes of minified JSON
```

The output buffer must hold at least as many bytes as the input. To minify
a document that you have parsed, and so validated, use `simdjson::minify(element)`
with the DOM API. With On-Demand, `simdjson::to_json_string` is not a
minifier: it returns the raw JSON text of a value with only the leading and
trailing whitespace removed, and it does not validate it. See the "Output to
strings" and "Minifying JSON strings without parsing" sections of
[basics.md](basics.md).
