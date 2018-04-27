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
