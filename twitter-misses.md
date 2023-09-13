
# twitter.json Branch Miss Variants

| Contain | Scalars | Strings   |  Cycles |   Instrs | Misses | Docs/sec |
|---------|---------|-----------|--------:|---------:|-------:|---------:|
| orig    | orig    | orig      | 43.1557 | 140.4823 |    262 |   5051.3 |
| orig    | orig    | unescaped | 40.9559 | 137.7313 |    210 |   5251.4 |
| orig    | orig    | ascii     | 40.8708 | 137.7313 |    219 |   5420.6 |
| orig    | orig    | empty     | 38.6624 | 134.2775 |    193 |   5634.1 |
| orig    | 1digit  | orig      | 41.7827 | 134.5389 |    299 |   5171.5 |
| orig    | 1digit  | unescaped | 39.1917 | 131.7879 |    244 |   5394.8 |
| orig    | 1digit  | ascii     | 39.3877 | 131.7879 |    242 |   5560.8 |
| orig    | 1digit  | empty     | 37.1633 | 128.3341 |    204 |   5796.6 |
| orig    | str     | orig      | 46.2681 | 128.7201 |    193 |   4816.3 |
| orig    | str     | unescaped | 43.7883 | 125.9691 |    186 |   5022.5 |
| orig    | str     | ascii     | 43.9507 | 125.9691 |    177 |   5147.6 |
| orig    | str     | empty     | 41.1975 | 122.5153 |    110 |   5397.7 |
| array   | orig    | orig      | 50.1138 | 171.0958 |    469 |   4548.1 |
| array   | orig    | unescaped | 47.4255 | 168.3448 |    429 |   4738.2 |
| array   | orig    | ascii     | 47.5435 | 168.3448 |    426 |   4851.1 |
| array   | orig    | empty     | 45.5023 | 164.8910 |    406 |   5006.6 |
| array   | 1digit  | orig      | 47.5542 | 165.1527 |    386 |   4693.9 |
| array   | 1digit  | unescaped | 45.8350 | 162.4017 |    342 |   4862.4 |
| array   | 1digit  | ascii     | 45.9391 | 162.4017 |    335 |   4973.8 |
| array   | 1digit  | empty     | 43.3705 | 158.9479 |    262 |   5205.8 |
| array   | str     | orig      | 52.0014 | 160.6582 |    331 |   4416.4 |
| array   | str     | unescaped | 49.3208 | 157.9072 |    272 |   4604.5 |
| array   | str     | ascii     | 49.2947 | 157.9072 |    271 |   4728.9 |
| array   | str     | empty     | 46.3058 | 154.4534 |    179 |   4967.3 |
| flat    | orig    | orig      | 43.3339 | 159.8664 |    407 |   5049.4 |
| flat    | orig    | unescaped | 41.7361 | 157.1154 |    384 |   5188.4 |
| flat    | orig    | ascii     | 41.6981 | 157.1154 |    393 |   5330.9 |
| flat    | orig    | empty     | 39.4102 | 153.6616 |    386 |   5570.0 |
| flat    | 1digit  | orig      | 41.7734 | 153.9234 |    364 |   5180.8 |
| flat    | 1digit  | unescaped | 40.0340 | 151.1724 |    308 |   5343.7 |
| flat    | 1digit  | ascii     | 39.8567 | 151.1724 |    313 |   5507.9 |
| flat    | 1digit  | empty     | 37.3433 | 147.7186 |    251 |   5781.1 |
| flat    | str     | orig      | 46.2458 | 149.4289 |    471 |   4821.1 |
| flat    | str     | unescaped | 44.4161 | 146.6778 |    413 |   4985.2 |
| flat    | str     | ascii     | 44.4998 | 146.6778 |    414 |   5102.1 |
| flat    | str     | empty     | 41.0894 | 143.2241 |      3 |   5419.1 |

## Container State Transition Miss Reduction

| Contain       | orig orig | orig ascii | orig empty | 1digit orig | 1digit ascii | 1digit empty | str orig | str ascii | str empty |
|---------------|----------:|-----------:|-----------:|------------:|-------------:|-------------:|---------:|----------:|----------:|
| orig -> array |      -207 |       -207 |       -213 |         -87 |          -93 |          -58 |     -138 |       -94 |       -69 |
| orig -> flat  |      -145 |       -174 |       -193 |         -65 |          -71 |          -47 |     -278 |      -237 |       107 |
| array -> flat |        62 |         33 |         20 |          22 |           22 |           11 |     -140 |      -143 |       176 |

## Scalar State Transition Miss Reduction

| Scalars        | orig orig | orig ascii | orig empty | array orig | array ascii | array empty | flat orig | flat ascii | flat empty |
|----------------|----------:|-----------:|-----------:|-----------:|------------:|------------:|----------:|-----------:|-----------:|
| orig -> 1digit |       -37 |        -23 |        -11 |         83 |          91 |         144 |        43 |         80 |        135 |
| orig -> str    |        69 |         42 |         83 |        138 |         155 |         227 |       -64 |        -21 |        383 |
| 1digit -> str  |       106 |         65 |         94 |         55 |          64 |          83 |      -107 |       -101 |        248 |

## String State Transition Miss Reduction

| Strings            | orig orig | orig 1digit | orig str | array orig | array 1digit | array str | flat orig | flat 1digit | flat str |
|--------------------|----------:|------------:|---------:|-----------:|-------------:|----------:|----------:|------------:|---------:|
| orig -> unescaped  |        52 |          55 |        7 |         40 |           44 |        59 |        23 |          56 |       58 |
| orig -> ascii      |        43 |          57 |       16 |         43 |           51 |        60 |        14 |          51 |       57 |
| orig -> empty      |        69 |          95 |       83 |         63 |          124 |       152 |        21 |         113 |      468 |
| unescaped -> ascii |        -9 |           2 |        9 |          3 |            7 |         1 |        -9 |          -5 |       -1 |
| unescaped -> empty |        17 |          40 |       76 |         23 |           80 |        93 |        -2 |          57 |      410 |
| ascii -> empty     |        26 |          38 |       67 |         20 |           73 |        92 |         7 |          62 |      411 |
