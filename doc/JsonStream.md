# JsonStream
- [Motivations](#Motivations)
- [Performance](#Performance)
- [How it works](#How it works)
- [Support](#Support)
- [API](#API)
- [Exemples](#Exemples)


## Motivations
The main motivation for this piece of software is to achieve maximum speed and offer
better quality of life in parsing files containing multiple JSON documents.
## Performance
[![Chart.png](https://i.postimg.cc/1X2gVHg4/Chart.png)](https://postimg.cc/5Q59Z8SM)
## How it works
## Support
Since we want to offer flexibility and not restrict ourselves to a specific file
format, we support any file that contains any amount of valid JSON document, separated by one
or more character that is considered a whitespace by the JSON spec. Anything that is
 not whitespace will be parsed as a JSON document and could lead to failure.

Whitespace Characters: 
- **Space**
- **Linefeed**
- **Carriage return**
- **Horizontal tab**
- **Nothing**




Some official formats **(non-exhaustive list)**:
- [Newline-Delimited JSON (NDJSON)](http://ndjson.org/)
- [JSON lines (JSONL)](http://jsonlines.org/)
- [RFC 7464](https://datatracker.ietf.org/doc/rfc7464/)
- [More on Wikipedia...](https://en.wikipedia.org/wiki/JSON_streaming)

## API
## Exemples

Here is a simple exemple, using single header simdjson:
```cpp
#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"

int parse_file(const char *filename) {
    simdjson::padded_string p = simdjson::get_corpus(filename);
    simdjson::ParsedJson pj;
    simdjson::JsonStream js{p};
    int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
    
    while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
            parse_res = js.json_parse(pj);

            //Do something with pj...
        }
}
```



