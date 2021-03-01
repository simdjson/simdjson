
# Tape structure in simdjson

We parse a JSON document to a tape. A tape is an array of 64-bit values. Each node encountered in the JSON document is written to the tape using one or more 64-bit tape elements; the layout of the tape is in "document order": elements are stored as they are encountered in the JSON document.

Throughout, little endian encoding is assumed. The tape is indexed starting at 0 (the first element is at index 0).

## Example

It is sometimes useful to start with an example. Consider the following JSON document:

```json
{
	"Image": {
		"Width": 800,
		"Height": 600,
		"Title": "View from 15th Floor",
		"Thumbnail": {
			"Url": "http://www.example.com/image/481989943",
			"Height": 125,
			"Width": 100
		},
		"Animated": false,
		"IDs": [116, 943, 234, 38793]
	}
}
```

The following is a dump of the content of the tape, with the first number of each line representing the index of a tape element.

### The Tape
| index | element (64 bit word)                                               |
| ----- | ------------------------------------------------------------------- |
| 0     | r	// pointing to 39 (right after last node)                         |
| 1     | {	// pointing to next tape location 38 (first node after the scope) |
| 2     | string "Image"                                                      |
| 3     | {	// pointing to next tape location 37 (first node after the scope) |
| 4     | string "Width"                                                      |
| 5     | integer 800                                                         |
| 7     | string "Height"                                                     |
| 8     | integer 600                                                         |
| 10    | string "Title"                                                      |
| 11    | string "View from 15th Floor"                                       |
| 12    | string "Thumbnail"                                                  |
| 13    | {	// pointing to next tape location 23 (first node after the scope) |
| 14    | string "Url"                                                        |
| 15    | string "http://www.example.com/image/481989943"                     |
| 16    | string "Height"                                                     |
| 17    | integer 125                                                         |
| 19    | string "Width"                                                      |
| 20    | integer 100                                                         |
| 22    | }	// pointing to previous tape location 13 (start of the scope)     |
| 23    | string "Animated"                                                   |
| 24    | false                                                               |
| 25    | string "IDs"                                                        |
| 26    | [	// pointing to next tape location 36 (first node after the scope) |
| 27    | integer 116                                                         |
| 29    | integer 943                                                         |
| 31    | integer 234                                                         |
| 33    | integer 38793                                                       |
| 35    | ]	// pointing to previous tape location 26 (start of the scope)     |
| 36    | }	// pointing to previous tape location 3 (start of the scope)      |
| 37    | }	// pointing to previous tape location 1 (start of the scope)      |
| 38    | r	// pointing to 0 (start root)                                     |



## General formal of the tape elements

Most tape elements are written as `('c' << 56) + x` where `'c'` is some ASCII character determining the type of the element (out of 't', 'f', 'n', 'l', 'u', 'd', '"', '{', '}', '[', ']' ,'r') and where `x` is a 56-bit value called the payload. The payload is normally interpreted as an unsigned 56-bit integer. Note that 56-bit integers can be quite large.


Performance consideration: We believe that accessing the tape in regular units of 64 bits is more important for performance than saving memory.

## Simple JSON values

Simple JSON nodes are represented with one tape element:

- null is  represented as the 64-bit value `('n' << 56)` where `'n'` is the 8-bit code point values (in ASCII) corresponding to the letter `'n'`.
- true is  represented as the 64-bit value `('t' << 56)`.
- false is  represented as the 64-bit value `('f' << 56)`.


## Integer and Double values

Integer values are represented as two 64-bit tape elements:
- The 64-bit value `('l' << 56)` followed by the 64-bit integer value literally. Integer values are assumed to be signed 64-bit values, using two's complement notation.
- The 64-bit value `('u' << 56)` followed by the 64-bit integer value literally. Integer values are assumed to be unsigned 64-bit values.


Float values are represented as two 64-bit tape elements:
- The 64-bit value `('d' << 56)` followed by the 64-bit double value literally in standard IEEE 754 notation.

Performance consideration: We store numbers of the main tape because we believe that locality of reference is helpful for performance.

## Root node

Each JSON document will have two special 64-bit tape elements representing a root node, one at the beginning and one at the end.

- The first 64-bit tape element contains the value `('r' << 56) + x` where `x` is the location on the tape of the last root element.
- The last 64-bit tape element contains the value `('r' << 56)`.

All of the parsed document is located between these two 64-bit tape elements.

Hint: We can read the first tape element to determine the length of the tape.


## Strings

We prefix the string data itself by a 32-bit header to be interpreted as a 32-bit integer. It indicates the length of the string. The actual string data starts at an offset of 4 bytes.

We store string values using UTF-8 encoding with null termination on a separate tape. A string value is represented on the main tape as the 64-bit tape element `('"' << 56) + x` where the payload `x` is the location on the string tape of the null-terminated string.

## Arrays

JSON arrays are represented using two 64-bit tape elements.

- The first 64-bit tape element contains the value `('[' << 56) + (c << 32) + x` where the payload `x` is 1 + the index of the second 64-bit tape element on the tape  as a 32-bit integer and where `c` is the count of the number of elements (immediate children) in the array, satured to a 24-bit value (meaning that it cannot exceed 16777215 and if the real count exceeds 16777215, 16777215 is stored).  Note that the exact count of elements can always be computed by iterating (e.g., when it is 16777215 or higher).
- The second 64-bit tape element contains the value `(']' << 56) + x` where the payload `x` contains the index of the first 64-bit tape element on the tape.

All the content of the array is located between these two tape elements, including arrays and objects.

Performance consideration: We can skip the content of an array entirely by accessing the first 64-bit tape element, reading the payload and moving to the corresponding index on the tape.

## Objects

JSON objects are represented using two 64-bit tape elements.

- The first 64-bit tape element contains the value `('{' << 56) + (c << 32) + x` where the payload `x` is 1 + the index of the second 64-bit tape element on the tape as a 32-bit integer and where `c` is the count of the number of key-value pairs (immediate children) in the array, satured to a 24-bit value (meaning that it cannot exceed 16777215 and if the real count exceeds 16777215, 16777215 is stored). Note that the exact count of key-value pairs can always be computed by iterating (e.g., when it is 16777215 or higher).
- The second 64-bit tape element contains the value `('}' << 56) + x` where the payload `x` contains the index of the first 64-bit tape element on the tape.

In-between these two tape elements, we alternate between key (which must be strings) and values. A value could be an object or an array.

All the content of the object is located between these two tape elements, including arrays and objects.

Performance consideration: We can skip the content of an object entirely by accessing the first 64-bit tape element, reading the payload and moving to the corresponding index on the tape.
