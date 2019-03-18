# Notes on simdjson

## Rationale:

The simdjson project serves two purposes:

1. It creates a useful library for parsing JSON data quickly.

2. It is a demonstration of the use of SIMD and pipelined programming techniques to perform a complex and irregular task. 
These techniques include the use of large registers and SIMD instructions to process large amounts of input data at once, 
to hold larger entities than can typically be held in a single General Purpose Register (GPR), and to perform operations 
that are not cheap to perform without use of a SIMD unit (for example table lookup using permute instructions). 

The other key technique is that the system is designed to minimize the number of unpredictable branches that must be taken 
to perform the task. Modern architectures are both wide and deep (4-wide pipelines with ~14 stages are commonplace). A 
recent Intel Architecture processor, for example, can perform 3 256-bit SIMD operations or 2 512-bit SIMD operations per
cycle as well as other operations on general purpose registers or with the load/store unit. An incorrectly predicted branch
will clear this pipeline. While it is rare that a programmer can achieve the maximum throughput on a machine, a developer 
may be missing the opportunity to carry out 56 operations for each branch miss. 

Many code-bases make use of SIMD and deeply pipelined, "non-branchy", processing for regular tasks. Numerical problems 
(e.g. "matrix multiply") or simple 'bulk search' tasks (e.g. "count all the occurrences of a given character in a text", 
"find the first occurrence of the string 'foo' in a text") frequently use this class of techniques. We are demonstrating 
that these techniques can be applied to much more complex and less regular tasks.

## Design:

### Stage 1: SIMD over bytes; bit vector processing over bytes.

The first stage of our processing must identify key points in our input: the 'structural characters' of JSON (curly and 
square braces, colon, and comma), the start and end of strings as delineated by double quote characters, other JSON 'atoms' 
that are not distinguishable by simple characters (constructs such as "true", "false", "null" and numbers), as well as 
discovering these characters and atoms in the presence of both quoting conventions and backslash escaping conventions. 

As such we follow the broad outline of the construction of a structural index as set forth in the Mison paper [XXX]; first, 
the discovery of odd-length sequences of backslash characters (which will cause quote characters immediately following to 
be escaped and not serve their quoting role but instead be literal charaters), second, the discovery of quote pairs (which 
cause structural characters within the quote pairs to also be merely literal characters and have no function as structural 
characters), then finally the discovery of structural characters not contained without the quote pairs.

We depart from the Mison paper in terms of method and overall design. In terms of method, the Mison paper uses iteration 
over bit vectors to discover backslash sequences and quote pairs; we introduce branch-free techniques to discover both of 
these properties.

We also make use of our ability to quickly detect whitespace in this early stage. We can use another bit-vector based 
transformation to discover locations in our data that follow a structural character or quote or whitespace and are not whitespace.  Excluding locations within strings, and the structural characters we have already discovered, 
these locations are the only place that we can expect to see the starts of the JSON 'atoms'. These locations are thus 
treated as 'structural' ('pseudo-structural characters').

This stage involves either SIMD processing over bytes or the manipulation of bit arrays that have 1 bit corresponding 
to 1 byte of input. As such, it can be quite inefficient for some inputs - it is possible to observe dozens of operations 
taking place to discover that there are in fact no odd-numbered sequences of backslashes or quotes in a given block of 
input. However, this inefficiency on such inputs is balanced by the fact that it costs no more to run this code over 
complex structured input, and the alternatives would generally involve running a number of unpredictable branches (for 
example, the loop branches in Mison that iterate over bit vectors).

### Stage 2: The transition from "SIMD over bytes" to "indices"

Our structural, pseudo-structural and other 'interesting' characters are relatively rare (TODO: quantify in detail - 
it's typically about 1 in 10). As such, continuing to process them as bit vectors will involve manipulating data structures 
that are relatively large as well as being fairly unpredictably spaced. We must transform these bitvectors of "interesting" 
locations into offsets.

Note that we can examine the character at the offset to discover what the original function of the item in the bitvector 
was. While the JSON structural characters and quotes are relatively self-explanatory (although working only with one offset 
at a time, we have lost the distinction between opening quotes and closing quotes, something that was available in Stage 1), 
it is a quirk of JSON that the legal atoms can all be distinguished from each other by their first character - 't' for 
'true', 'f' for 'false', 'n' for 'null' and the character class [0-9-] for numerical values.

Thus, the offset suffices, as long as we retain our original input.

Our current implementation involves a straightforward transformation of bitmaps to indices by use of the 'count trailing 
zeros' operation and the well-known operation to clear the lowest set bit. Note that this implementation introduces an 
unpredictable branch; unless there is a regular pattern in our bitmaps, we would expect to have at least one branch miss 
for each bitmap.

### Stage 3: Operation over indices

This now works over a dual structure.

1. The "state machine", whose role it is to validate the sequence of structural characters and ensure that the input is at least generally structured like valid JSON (after this stage, the only errors permissible should be malformed atoms and numbers). If and only if the "state machine" reached all accept states, then,

2. The "tape machine" will have produced valid output. The tape machine works blindly over characters writing records to tapes. These records create a lean but somewhat traversable linked structure that, for valid inputs, should represent what we need to know about the JSON input.

FIXME: a lot more detail is required on the operation of both these machines.
