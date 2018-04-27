I looks like RapidJSON can process inputs at 4 cycles per byte. Rapidjson has a function that skips spaces and comments... it is obviously optimized with some SSE... 

Everything else just a series of recursive calls with switch cases. It starts like this...

```
SkipWhitespaceAndComments<parseFlags>(is);
ParseValue<parseFlags>(is, handler);
```

... which is just this...

```
    void ParseValue(InputStream& is, Handler& handler) {
        switch (is.Peek()) {
            case 'n': ParseNull  <parseFlags>(is, handler); break;
            case 't': ParseTrue  <parseFlags>(is, handler); break;
            case 'f': ParseFalse <parseFlags>(is, handler); break;
            case '"': ParseString<parseFlags>(is, handler); break;
            case '{': ParseObject<parseFlags>(is, handler); break;
            case '[': ParseArray <parseFlags>(is, handler); break;
            default :
                      ParseNumber<parseFlags>(is, handler);
                      break;

        }
    }
```


What RapidJSON does not do:

- Lazy evaluation (it figures out the content of strings and numbers)
- It does not do computed gotos

Geoff's Loose Theory on Why This Is Better Than You'd Think

The if there was such a thing as the high church of branch-free programming would allege the above code is a branch-miss nightmare. However, there are very likely regular patterns among our results here both due to the JSON grammar - if you just took a ParseObject you're going to be doing a ParseString pretty soon now - and due to the fact that many of the large scale inputs we have are very regular (i.e. contained objects are close to having a "schema" even though that's not a formal part of JSON, and dynamically we may do reasonably well).

Even if we take a branch miss per structural character we are looking at 14 cycles or so of penalty - which given a structural character rate of 0.1 gets us to 1.4. In fact, they do better than that (at least at a high level) as they may not be taking unpredictable branches on all structural characters - they may do very well with processing ":" characters in objects as well as "," characters in objects and arrays, and likely handle the second quote character in a string without much problem. So the penalty of branchiness is perhaps not enormous.

Also in their favor is that that SIMD scanning or simple input traversal is always appropriate to the actual input, while we are having to do "everything everywhere" (we are looking for odd-length backslash sequences outside of strings, and for structural characters inside of strings).

To investigate:

 - At 4 cycles per byte, this result is quite a bit faster than most benchmarks. The Mison paper and all the JSON benchmarks I've dug up suggest 100-300 Megabytes per second, but 4 cycles or 4.7 cycles per byte would be close to 700-800MBps on most systems. This this because those benchmarks are more onerous than what RapidJSON is doing in our context?
 
