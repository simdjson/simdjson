Files from https://github.com/plokhotnyuk/jsoniter-scala/tree/master/jsoniter-scala-benchmark/src/main/resources/com/github/plokhotnyuk/jsoniter_scala/benchmark

See issue:
https://github.com/lemire/simdjson/issues/70

The files che-*.geo.json are number-parsing stress tests.

```
$ for i in *.json ; do echo $i; ./parsingcompetition $i  ; done
che-1.geo.json
simdjson                                	:    4.841 cycles per input byte (best)    4.880 cycles per input byte (avg)    0.689 GB/s (error margin: 0.005 GB/s)
RapidJSON (accurate number parsing)     	:   18.326 cycles per input byte (best)   19.185 cycles per input byte (avg)    0.185 GB/s (error margin: 0.008 GB/s)
RapidJSON (insitu, accurate number parsing)	:   18.158 cycles per input byte (best)   18.957 cycles per input byte (avg)    0.187 GB/s (error margin: 0.008 GB/s)
nlohmann-json                           	:   90.423 cycles per input byte (best)   91.077 cycles per input byte (avg)    0.038 GB/s (error margin: 0.000 GB/s)

che-2.geo.json
simdjson                                	:    4.849 cycles per input byte (best)    4.882 cycles per input byte (avg)    0.687 GB/s (error margin: 0.005 GB/s)
RapidJSON (accurate number parsing)     	:   18.248 cycles per input byte (best)   19.197 cycles per input byte (avg)    0.186 GB/s (error margin: 0.009 GB/s)
RapidJSON (insitu, accurate number parsing)	:   18.178 cycles per input byte (best)   18.951 cycles per input byte (avg)    0.186 GB/s (error margin: 0.008 GB/s)
nlohmann-json                           	:   91.483 cycles per input byte (best)   91.842 cycles per input byte (avg)    0.037 GB/s (error margin: 0.000 GB/s)

che-3.geo.json
simdjson                                	:    4.862 cycles per input byte (best)    4.892 cycles per input byte (avg)    0.686 GB/s (error margin: 0.004 GB/s)
RapidJSON (accurate number parsing)     	:   18.316 cycles per input byte (best)   19.202 cycles per input byte (avg)    0.185 GB/s (error margin: 0.008 GB/s)
RapidJSON (insitu, accurate number parsing)	:   18.143 cycles per input byte (best)   18.957 cycles per input byte (avg)    0.187 GB/s (error margin: 0.008 GB/s)
nlohmann-json                           	:   91.462 cycles per input byte (best)   91.758 cycles per input byte (avg)    0.037 GB/s (error margin: 0.000 GB/s)
```

