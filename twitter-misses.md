
# twitter.json Branch Miss Variants

| Contain | Scalars | Strings   |  Cycles |   Instrs | Misses | Docs/sec |
|---------|---------|-----------|---------|----------|--------|----------|
| orig    | orig    | orig      | 43.3673 | 140.4823 |    270 |   5046.7 |
| orig    | orig    | unescaped | 41.1452 | 137.7313 |    226 |   5233.6 |
| orig    | orig    | ascii     | 41.2052 | 137.7313 |    227 |   5394.0 |
| orig    | orig    | empty     | 39.0695 | 134.2775 |    204 |   5611.4 |
| orig    | 1digit  | orig      | 41.9665 | 134.5389 |    299 |   5149.0 |
| orig    | 1digit  | unescaped | 39.6112 | 131.7879 |    251 |   5381.3 |
| orig    | 1digit  | ascii     | 39.6832 | 131.7879 |    249 |   5527.6 |
| orig    | 1digit  | empty     | 37.3531 | 128.3341 |    206 |   5773.6 |
| orig    | str     | orig      | 46.8964 | 128.7201 |    198 |   4763.2 |
| orig    | str     | unescaped | 44.3903 | 125.9691 |    191 |   4973.5 |
| orig    | str     | ascii     | 44.1389 | 125.9691 |    181 |   5127.3 |
| orig    | str     | empty     | 41.7417 | 122.5153 |    114 |   5353.6 |
| array   | orig    | orig      | 50.2381 | 171.0958 |    480 |   4519.9 |
| array   | orig    | unescaped | 47.6907 | 168.3448 |    434 |   4690.3 |
| array   | orig    | ascii     | 47.5435 | 168.3448 |    433 |   4830.0 |
| array   | orig    | empty     | 45.7856 | 164.8910 |    419 |   4994.6 |
| array   | 1digit  | orig      | 48.4116 | 165.1527 |    393 |   4668.6 |
| array   | 1digit  | unescaped | 45.9916 | 162.4017 |    350 |   4860.8 |
| array   | 1digit  | ascii     | 46.0187 | 162.4017 |    352 |   4973.8 |
| array   | 1digit  | empty     | 43.3807 | 158.9479 |    265 |   5205.8 |
| array   | str     | orig      | 52.4549 | 160.6582 |    336 |   4398.3 |
| array   | str     | unescaped | 49.6504 | 157.9072 |    272 |   4582.0 |
| array   | str     | ascii     | 49.3938 | 157.9072 |    271 |   4716.9 |
| array   | str     | empty     | 46.6289 | 154.4534 |    180 |   4936.9 |
| flat    | orig    | orig      | 43.4425 | 159.8664 |    418 |   5032.2 |
| flat    | orig    | unescaped | 41.9821 | 157.1154 |    398 |   5167.3 |
| flat    | orig    | ascii     | 41.9992 | 157.1154 |    401 |   5319.8 |
| flat    | orig    | empty     | 39.4102 | 153.6616 |    393 |   5568.4 |
| flat    | 1digit  | orig      | 41.8933 | 153.9234 |    375 |   5164.1 |
| flat    | 1digit  | unescaped | 40.1416 | 151.1724 |    308 |   5332.6 |
| flat    | 1digit  | ascii     | 40.1736 | 151.1724 |    323 |   5484.0 |
| flat    | 1digit  | empty     | 37.4876 | 147.7186 |    255 |   5774.7 |
| flat    | str     | orig      | 46.6180 | 149.4289 |    473 |   4803.2 |
| flat    | str     | unescaped | 44.5677 | 146.6778 |    416 |   4964.7 |
| flat    | str     | ascii     | 44.5673 | 146.6778 |    420 |   5091.8 |
| flat    | str     | empty     | 41.2035 | 143.2241 |      3 |   5392.7 |

## Container State Transition Miss Reduction

| Contain       | orig orig | orig ascii | orig empty | 1digit orig | 1digit ascii | 1digit empty | str orig | str ascii | str empty |
|---------------|-----------|------------|------------|-------------|--------------|--------------|----------|-----------|-----------|
| orig -> array |      -210 |       -206 |       -215 |         -94 |         -103 |          -59 |     -138 |       -90 |       -66 |
| orig -> flat  |      -148 |       -174 |       -189 |         -76 |          -74 |          -49 |     -275 |      -239 |       111 |
| array -> flat |        62 |         32 |         26 |          18 |           29 |           10 |     -137 |      -149 |       177 |

## Scalar State Transition Miss Reduction

| Scalars        | orig orig | orig ascii | orig empty | array orig | array ascii | array empty | flat orig | flat ascii | flat empty |
|----------------|-----------|------------|------------|------------|-------------|-------------|-----------|------------|------------|
| orig -> 1digit |       -29 |        -22 |         -2 |         87 |          81 |         154 |        43 |         78 |        138 |
| orig -> str    |        72 |         46 |         90 |        144 |         162 |         239 |       -55 |        -19 |        390 |
| 1digit -> str  |       101 |         68 |         92 |         57 |          81 |          85 |       -98 |        -97 |        252 |

## String State Transition Miss Reduction

| Strings            | orig orig | orig 1digit | orig str | array orig | array 1digit | array str | flat orig | flat 1digit | flat str |
|--------------------|-----------|-------------|----------|------------|--------------|-----------|-----------|-------------|----------|
| orig -> unescaped  |        44 |          48 |        7 |         46 |           43 |        64 |        20 |          67 |       57 |
| orig -> ascii      |        43 |          50 |       17 |         47 |           41 |        65 |        17 |          52 |       53 |
| orig -> empty      |        66 |          93 |       84 |         61 |          128 |       156 |        25 |         120 |      470 |
| unescaped -> ascii |        -1 |           2 |       10 |          1 |           -2 |         1 |        -3 |         -15 |       -4 |
| unescaped -> empty |        22 |          45 |       77 |         15 |           85 |        92 |         5 |          53 |      413 |
| ascii -> empty     |        23 |          43 |       67 |         14 |           87 |        91 |         8 |          68 |      417 |
