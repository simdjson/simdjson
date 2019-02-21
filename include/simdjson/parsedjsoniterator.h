#ifndef PARSED_JSON_ITERATOR_H
# define PARSED_JSON_ITERATOR_H

#include <cstddef>
#include <cstdint>
#include <ostream>

// Forward-declare ParsedJson
struct ParsedJson;

struct ParsedJsonIterator {

    explicit ParsedJsonIterator(ParsedJson &pj_);
    ~ParsedJsonIterator();

    ParsedJsonIterator(const ParsedJsonIterator &o);

    ParsedJsonIterator(ParsedJsonIterator &&o);

    bool isOk() const;

    // useful for debuging purposes
    size_t get_tape_location() const;

    // useful for debuging purposes
    size_t get_tape_length() const;

    // returns the current depth (start at 1 with 0 reserved for the fictitious root node)
    size_t get_depth() const;

    // A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
    // The root node has type 'r'.
    uint8_t get_scope_type() const;

    // move forward in document order
    bool move_forward();

    // retrieve the character code of what we're looking at:
    // [{"sltfn are the possibilities
    uint8_t get_type()  const;

    // get the int64_t value at this node; valid only if we're at "l"
    int64_t get_integer()  const;

    // get the string value at this node (NULL ended); valid only if we're at "
    // note that tabs, and line endings are escaped in the returned value (see print_with_escapes)
    // return value is valid UTF-8
    const char * get_string() const;

    // get the double value at this node; valid only if
    // we're at "d"
    double get_double()  const;

    bool is_object_or_array() const;

    bool is_object() const;

    bool is_array() const;

    bool is_string() const;

    bool is_integer() const;

    bool is_double() const;

    static bool is_object_or_array(uint8_t type);

    // when at {, go one level deep, looking for a given key
    // if successful, we are left pointing at the value,
    // if not, we are still pointing at the object ({)
    // (in case of repeated keys, this only finds the first one)
    bool move_to_key(const char * key);

    // throughout return true if we can do the navigation, false
    // otherwise

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move forward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, { and [.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    // valid if we're not at the end of a scope (returns true).
    bool next();

    // Withing a given scope (series of nodes at the same depth within either an
    // array or an object), we move backward.
    // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true when starting at the end
    // of the scope.
    // At the object ({) or at the array ([), you can issue a "down" to visit their content.
    bool prev();

    // Moves back to either the containing array or object (type { or [) from
    // within a contained scope.
    // Valid unless we are at the first level of the document
    bool up();


    // Valid if we're at a [ or { and it starts a non-empty scope; moves us to start of
    // that deeper scope if it not empty.
    // Thus, given [true, null, {"a":1}, [1,2]], if we are at the { node, we would move to the
    // "a" node.
    bool down();

    // move us to the start of our current scope,
    // a scope is a series of nodes at the same level
    void to_start_scope();

    // void to_end_scope();              // move us to
    // the start of our current scope; always succeeds

    // print the thing we're currently pointing at
    bool print(std::ostream &os, bool escape_strings = true) const;
    typedef struct {size_t start_of_scope; uint8_t scope_type;} scopeindex_t;

private:

    ParsedJsonIterator& operator=(const ParsedJsonIterator& other) ;

    ParsedJson &pj;
    size_t depth;
    size_t location;     // our current location on a tape
    size_t tape_length;
    uint8_t current_type;
    uint64_t current_val;
    scopeindex_t *depthindex;

};

#endif