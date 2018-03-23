# simdjson

A *research* library. The purpose of this repository is to support an eventual paper.

Goal: Speed up the parsing of JSON per se. No materialization.

Parsing gigabytes of JSON per second


## Todo

- Write unit tests
- Write bona fide, accurate benchmarks (with fair comparisons using good alternatives). Geoff wrote:
> 44% of the time is in the tree construction. Of the remaining 56%, pretty much half is in the code to discover structural characters and half is in the naive code to flatten that out into a vector of u32 offsets (the 'iterate over set bits' problem).
- Document better the code, make the code easier to use
- Add some measure of error handling (maybe optional)
- Geoff wrote:
> A future goal for Stage 3 will also be to thread together vectors or lists of similar structural elements (e.g. strings, objects, numbers, etc). A putative 'stage 4' will then be able to iterate in parallel fashion over these vectors (branchlessly, or at least without a pipeline-killing "Giant What I Am Doing Anyway" switch at the top) and transform them into more usable values. Some of this code is also inherently interesting (de-escaping strings, high speed atoi - an old favorite).
- Geoff wrote:
> I'm focusing on the tree construction at the moment. I think we can abstract the structural characters to 3 operations during that stage (UP, DOWN, SIDEWAYS), batch them, and build out tree structure in bulk with data-driven SIMD operations rather than messing around with branches. It's probably OK to have a table with 3^^5 or even 3^^6 entries, and it's still probably OK to have some hard cases eliminated and handled on a slow path (e.g. someone hits you with 6 scope closes in a row, forcing you to pop 24 bytes into your tree construction stack). (...) I spend some time with SIMD and found myself going in circles. There's an utterly fantastic solution in there somewhere involving turning the tree building code into a transformation over multiple abstracted symbols (e.g. UP/UP/SIDEWAYS/DOWN). It looked great until I smacked up against the fact that the oh-so-elegant solution I had sketched out had, on its critical path, an unaligned store followed by load partially overlapping with that store, to maintain the stack of 'up' pointers. Ugh. (...) So I worked through about 6 alternate solutions of various levels of pretentiousness none of which made me particularly happy.





## References

- https://tools.ietf.org/html/rfc7159
- Mison: A Fast JSON Parser for Data Analytics http://www.vldb.org/pvldb/vol10/p1118-li.pdf They build the index and parse at the same time. The index allows them to parse faster... so they don't materialize the index.  The speculative part is hastily described but they do hint that this is where much of the benefit comes from...
- The only public Mison implementation (in rust)  https://github.com/pikkr/pikkr
- http://rapidjson.org/md_doc_sax.html


Inspiring links:
- https://auth0.com/blog/beating-json-performance-with-protobuf/
- https://gist.github.com/shijuvar/25ad7de9505232c87034b8359543404a
- https://github.com/frankmcsherry/blog/blob/master/posts/2018-02-11.md

## Remarks on JSON parsing

- The JSON spec defines what a JSON parser is:
>  A JSON parser transforms a JSON text into another representation.  A JSON parser MUST accept all texts that conform to the JSON grammar.  A JSON parser MAY accept non-JSON forms or extensions. An implementation may set limits on the size of texts that it accepts.  An implementation may set limits on the maximum depth of nesting.  An implementation may set limits on the range and precision of numbers.  An implementation may set limits on the length and character contents of strings."

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

   - A string begins and ends with  quotation marks.  All Unicode characters may be placed within the   quotation marks, except for the characters that must be escaped:   quotation mark, reverse solidus, and the control characters (U+0000 through U+001F).

   - Regarding strings, Geoff wrote:
   > For example, in Stage 2 ("string detection") we could validate that the only place we saw backslashes was in places we consider "inside strings".


- Geoff's remark regarding the structure:
> next structural element
prev structural element
next structural element at the same level (i.e. skip over complex structures)
prev structural element at the same level
containing structural element ("up").



## Remarks on the code

- It seems that the use of  ``bzero`` is discouraged.
- Per input byte,  multiple bytes are allocated which could potentially be a problem when processing a very large document, hence one might want to be more incremental in practice maybe to minimize memory usage. For really large documents, there might be caching issues as well.
- The ``clmul`` thing is tricky but nice. (Geoff's remark:  find the spaces between quotes, is actually a ponderous way of doing parallel prefix over XOR, which a mathematically adept person would have realized could be done with clmul by -1. Not me, I had to look it up: http://bitmath.blogspot.com.au/2016/11/parallel-prefixsuffix-operations.html.)
- It is possible, though unlikely, that parallelizing the bitset decoding could be useful (https://lemire.me/blog/2018/03/08/iterating-over-set-bits-quickly-simd-edition/), and there is VCOMPRESSB (AVX-512)
