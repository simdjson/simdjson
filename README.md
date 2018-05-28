# simdjson : Parsing gigabytes of JSON per second

A *research* library. The purpose of this repository is to support an eventual paper. The immediate goal is not to produce a library that would be used in production systems. Of course, the end game is, indeed, to have an impact on production system.

Goal: Speed up the parsing of JSON per se. No materialization.

## Architecture

The parser works in three stages:

- Stage 1. Identifies quickly structure elements, strings, and so forth. Currently, there is no validation (JSON is assumed to be correct).
- Stage 2. Involves the "flattening" of the data from stage 1, that is, convert bitsets into arrays of indexes.
- Stage 3. (Structure building) Involves constructing a "tree" of sort to navigate through the data.
- Stage 4. (Currently unimplemented) Iterate throw the structure without "stalling" (fighting back against latency)

## Todo (Priority)

- The top level FSM is wrong and yields an error. Need to hand-hack the treatment of the first level.
- The tape machine is not robust against invalid input. A large string of closing brace characters, for example, will crash the tape machine and we will not consult the state machine in time to know that this wasn't valid input. Furthermore there are valid inputs in terms of the state machine that will create undefined behavior if the JSON nesting depth exceeds what we have provisioned for. This is a straightforward fix.
- The atoms need to be verified for validity. It would be nice to have a branch-free version for this. This stage can operate over our trees.
- The tape machine's output is nearly unreadable both for debugging purposes and as a structure in itself. We don't know where each nested item starts, only where it ends - a linear traversal at depth N could track this information for entries to depth N+1, so I am leery of putting extra information on the tapes until we have a more full idea of what the final stages look like. 
- The tape machine's output is also cryptic in that it's not clear whether records point into the input (offsets) or point to other items on our tapes. The simplest trick to fix this would be to use negative offsets for the tapes (growing them upward) so negative values point to our tapes and positive values point to our input. Obviously the current situation is not sustainable as we could actually wind up not knowing what our tape values really mean!

## Todo (Secondary)

- Build up a paper (use overleaf.com)
- Write unit tests
- Write bona fide, accurate benchmarks (with fair comparisons using good alternatives). See https://github.com/Geal/parser_benchmarks
- Document better the code, make the code easier to use
- Add some measure of error handling. I no longer think error handling is optional; I think this could be a key differentiating feature over what I suspect Mison actually does. If Mison happily accepts broken JSON because of its much vaunted ability to skip input, is this a feature or a bug?





## Academic References

- T.Mu ̈hlbauer,W.Ro ̈diger,R.Seilbeck,A.Reiser,A.Kemper,andT.Neumann. Instant loading for main memory databases. PVLDB, 6(14):1702–1713, 2013. (SIMD-based CSV parsing)

- Mytkowicz, Todd, Madanlal Musuvathi, and Wolfram Schulte. "Data-parallel finite-state machines." ACM SIGARCH Computer Architecture News. Vol. 42. No. 1. ACM, 2014.

- Lu, Yifan, et al. "Tree structured data processing on GPUs." Cloud Computing, Data Science & Engineering-Confluence, 2017 7th International Conference on. IEEE, 2017.

- Sidhu, Reetinder. "High throughput, tree automata based XML processing using FPGAs." Field-Programmable Technology (FPT), 2013 International Conference on. IEEE, 2013.

- Dai, Zefu, Nick Ni, and Jianwen Zhu. "A 1 cycle-per-byte XML parsing accelerator." Proceedings of the 18th annual ACM/SIGDA international symposium on Field programmable gate arrays. ACM, 2010.

- Lin, Dan, et al. "Parabix: Boosting the efficiency of text processing on commodity processors." High Performance Computer Architecture (HPCA), 2012 IEEE 18th International Symposium on. IEEE, 2012. http://parabix.costar.sfu.ca/export/1783/docs/HPCA2012/final_ieee/final.pdf
> Using this parallel bit stream approach, the vast majority of conditional branches used to identify key positions and/or syntax errors at each parsing position are mostly eliminated, which, as Section 6.2 shows, minimizes branch misprediction penalties. Accurate parsing and parallel lexical analysis is done through processor-friendly equations that require neither speculation nor multithreading.

- Deshmukh, V. M., and G. R. Bamnote. "An empirical evaluation of optimization parameters in XML parsing for performance enhancement." Computer, Communication and Control (IC4), 2015 International Conference on. IEEE, 2015.
APA


- Moussalli, Roger, et al. "Efficient XML Path Filtering Using GPUs." ADMS@ VLDB. 2011.

- Jianliang, Ma, et al. "Parallel speculative dom-based XML parser." High Performance Computing and Communication & 2012 IEEE 9th International Conference on Embedded Software and Systems (HPCC-ICESS), 2012 IEEE 14th International Conference on. IEEE, 2012.

- Li, Y., Katsipoulakis, N.R., Chandramouli, B., Goldstein, J. and Kossmann, D., 2017. Mison: a fast JSON parser for data analytics. Proceedings of the VLDB Endowment, 10(10), pp.1118-1129. http://www.vldb.org/pvldb/vol10/p1118-li.pdf
> They build the index and parse at the same time. The index allows them to parse faster... so they don't materialize the index.  The speculative part is hastily described but they do hint that this is where much of the benefit comes from...


- Cameron, Robert D., et al. "Parallel scanning with bitstream addition: An xml case study." European Conference on Parallel Processing. Springer, Berlin, Heidelberg, 2011.

- Cameron, Robert D., Kenneth S. Herdy, and Dan Lin. "High performance XML parsing using parallel bit stream technology." Proceedings of the 2008 conference of the center for advanced studies on collaborative research: meeting of minds. ACM, 2008.

- Shah, Bhavik, et al. "A data parallel algorithm for XML DOM parsing." International XML Database Symposium. Springer, Berlin, Heidelberg, 2009.

- Cameron, Robert D., and Dan Lin. "Architectural support for SWAR text processing with parallel bit streams: the inductive doubling principle." ACM Sigplan Notices. Vol. 44. No. 3. ACM, 2009.

- Amagasa, Toshiyuki, Mana Seino, and Hiroyuki Kitagawa. "Energy-Efficient XML Stream Processing through Element-Skipping Parsing." Database and Expert Systems Applications (DEXA), 2013 24th International Workshop on. IEEE, 2013.

- Medforth, Nigel Woodland. "icXML: Accelerating Xerces-C 3.1. 1 using the Parabix Framework." (2013).

- Zhang, Qiang Scott. Embedding Parallel Bit Stream Technology Into Expat. Diss. Simon Fraser University, 2010.

- Cameron, Robert D., et al. "Fast Regular Expression Matching with Bit-parallel Data Streams."

- Lin, Dan. Bits filter: a high-performance multiple string pattern matching algorithm for malware detection. Diss. School of Computing Science-Simon Fraser University, 2010.

- Yang, Shiyang. Validation of XML Document Based on Parallel Bit Stream Technology. Diss. Applied Sciences: School of Computing Science, 2013.

-  N. Nakasato, "Implementation of a parallel tree method on a GPU", Journal of Computational Science, vol. 3, no. 3, pp. 132-141, 2012.

## References

- [Parsing JSON is a Minefield 💣](http://seriot.ch/parsing_json.php)
- https://tools.ietf.org/html/rfc7159
- The only public Mison implementation (in rust)  https://github.com/pikkr/pikkr
- http://rapidjson.org/md_doc_sax.html
- https://github.com/Geal/parser_benchmarks/tree/master/json
- Gron: A command line tool that makes JSON greppable https://news.ycombinator.com/item?id=16727665
- GoogleGson https://github.com/google/gson
- Jackson https://github.com/FasterXML/jackson
- https://www.yelp.com/dataset_challenge
- RapidJSON. http://rapidjson.org/

Inspiring links:
- https://auth0.com/blog/beating-json-performance-with-protobuf/
- https://gist.github.com/shijuvar/25ad7de9505232c87034b8359543404a
- https://github.com/frankmcsherry/blog/blob/master/posts/2018-02-11.md


Validating UTF-8 takes no more than 0.7 cycles per byte:
- https://github.com/lemire/fastvalidate-utf-8 https://lemire.me/blog/2018/05/16/validating-utf-8-strings-using-as-little-as-0-7-cycles-per-byte/
 

## Remarks on JSON parsing

- The JSON spec defines what a JSON parser is:
>  A JSON parser transforms a JSON text into another representation.  A JSON parser MUST accept all texts that conform to the JSON grammar.  A JSON parser MAY accept non-JSON forms or extensions. An implementation may set limits on the size of texts that it accepts.  An implementation may set limits on the maximum depth of nesting.  An implementation may set limits on the range and precision of numbers.  An implementation may set limits on the length and character contents of strings."


- JSON is not JavaScript:

> All JSON is Javascript but NOT all Javascript is JSON. So {property:1} is invalid because property does not have double quotes around it. {'property':1} is also invalid, because it's single quoted while the only thing that can placate the JSON specification is double quoting. JSON is even fussy enough that {"property":.1} is invalid too, because you should have of course written {"property":0.1}. Also, don't even think about having comments or semicolons, you guessed it: they're invalid. (credit:https://github.com/elzr/vim-json)

- The  structural characters are:


      begin-array     =  [ left square bracket
      begin-object    =  { left curly bracket
      end-array       =  ] right square bracket
      end-object      =  } right curly bracket
      name-separator  = : colon
      value-separator = , comma

- It might be useful/important to prune "insignificant" white space characters. Maybe.

- Parsing and validating numbers fast could be potentially interesting,  but the spec allows many things. (Geoff wrote: "Parsing numbers (especially floating point ones!) and other atoms is fiddly but doable.")

- Error handling is a nice problem. Of course, we have to define what we mean by an error and which error types the parser should be responsible for.  Here are a few things that could/should probably trigger an error:

   - An array structure is represented as square brackets surrounding zero  or more values (or elements).  Elements are separated by commas.

   - An object structure is represented as a pair of curly brackets surrounding zero or more name/value pairs (or members).  A name is a string.  A single colon comes after each name, separating the name from the value.  A single comma separates a value from a following name.

   - Values must be one of false / null / true / object / array / number / string

   - A string begins and ends with  quotation marks.  All Unicode characters may be placed within the   quotation marks, except for the characters that must be escaped:   quotation mark, reverse solidus, and the control characters (U+0000 through U+001F). We can probably safely assume that strings are in UTF-8. [Decoding UTF-8 is fun](https://github.com/skeeto/branchless-utf8/blob/master/utf8.h). However, any character can be escaped in JSON string and escaping them might be required? Well, maybe you can quickly check whether a string needs escaping.

   - Regarding strings, Geoff wrote:
   > For example, in Stage 2 ("string detection") we could validate that the only place we saw backslashes was in places we consider "inside strings".



### Pseudo-structural elements

A character is pseudo-structural if and only if:

1. Not enclosed in quotes, AND
2. Is a non-whitespace character, AND
3. It's preceding chararacter is either:
(a) a structural character, OR
(b) whitespace.

This helps as we redefine some new characters as pseudo-structural such as the characters 1, 1, G, n in the following:

> { "foo" : 1.5, "bar" : 1.5   GEOFF_IS_A_DUMMY bla bla , "baz", null } 

## Remarks on the code

- It seems that the use of  ``bzero`` is discouraged.
- Per input byte,  multiple bytes are allocated which could potentially be a problem when processing a very large document, hence one might want to be more incremental in practice maybe to minimize memory usage. For really large documents, there might be caching issues as well.
- The ``clmul`` thing is tricky but nice. (Geoff's remark:  find the spaces between quotes, is actually a ponderous way of doing parallel prefix over XOR, which a mathematically adept person would have realized could be done with clmul by -1. Not me, I had to look it up: http://bitmath.blogspot.com.au/2016/11/parallel-prefixsuffix-operations.html.)
- It is possible, though maybe unlikely, that parallelizing the bitset decoding could be useful (https://lemire.me/blog/2018/03/08/iterating-over-set-bits-quickly-simd-edition/), and there is VCOMPRESSB (AVX-512)

## Future work

 Long term we should keep in mind the idea that what would be cool is a method to extract something like this code from an abstract description of something closer to a grammar.
