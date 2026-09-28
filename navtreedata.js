/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "simdjson", "index.html", [
    [ "The Basics", "md_doc_2basics.html", [
      [ "Requirements", "md_doc_2basics.html#autotoc_md0", null ],
      [ "Including simdjson", "md_doc_2basics.html#autotoc_md1", null ],
      [ "Using simdjson with package managers", "md_doc_2basics.html#autotoc_md2", null ],
      [ "Using simdjson as a CMake dependency", "md_doc_2basics.html#autotoc_md3", null ],
      [ "Versions", "md_doc_2basics.html#autotoc_md4", null ],
      [ "The basics: loading and parsing JSON documents", "md_doc_2basics.html#autotoc_md5", [
        [ "Advanced input options", "md_doc_2basics.html#autotoc_md6", null ]
      ] ],
      [ "Documents are iterators", "md_doc_2basics.html#autotoc_md7", [
        [ "Parser, document and JSON scope", "md_doc_2basics.html#autotoc_md8", null ]
      ] ],
      [ "string_view", "md_doc_2basics.html#autotoc_md9", null ],
      [ "u8string_view and u8string (C++20)", "md_doc_2basics.html#autotoc_md10", null ],
      [ "Avoiding pitfalls: enable development checks", "md_doc_2basics.html#autotoc_md11", null ],
      [ "Using the parsed JSON", "md_doc_2basics.html#autotoc_md12", [
        [ "Using the parsed JSON: additional examples", "md_doc_2basics.html#autotoc_md13", null ]
      ] ],
      [ "Adding support for custom types", "md_doc_2basics.html#autotoc_md14", [
        [ "1. Specialize <tt>simdjson::ondemand::value::get</tt> to get custom types (pre-C++20)", "md_doc_2basics.html#autotoc_md15", null ],
        [ "2. Use <tt>tag_invoke</tt> for custom types (C++20)", "md_doc_2basics.html#autotoc_md16", null ],
        [ "3. Using static reflection (C++26)", "md_doc_2basics.html#autotoc_md17", [
          [ "Special cases", "md_doc_2basics.html#autotoc_md18", null ],
          [ "Customizing (de)serialization with annotations", "md_doc_2basics.html#autotoc_md19", [
            [ "Renaming", "md_doc_2basics.html#autotoc_md20", null ],
            [ "Skipping", "md_doc_2basics.html#autotoc_md21", null ],
            [ "Default values", "md_doc_2basics.html#autotoc_md22", null ],
            [ "Rejecting unknown fields", "md_doc_2basics.html#autotoc_md23", null ],
            [ "Transparent structures", "md_doc_2basics.html#autotoc_md24", null ],
            [ "Flattening", "md_doc_2basics.html#autotoc_md25", null ],
            [ "Custom (de)serialization of a member", "md_doc_2basics.html#autotoc_md26", null ]
          ] ]
        ] ],
        [ "The simdjson::from shortcut (experimental, C++20)", "md_doc_2basics.html#autotoc_md27", [
          [ "Order-independent reflective deserialization (C++26)", "md_doc_2basics.html#autotoc_md28", null ]
        ] ]
      ] ],
      [ "Minifying JSON strings without parsing", "md_doc_2basics.html#autotoc_md29", null ],
      [ "UTF-8 validation (alone)", "md_doc_2basics.html#autotoc_md30", null ],
      [ "JSON Pointer", "md_doc_2basics.html#autotoc_md31", null ],
      [ "JSONPath", "md_doc_2basics.html#autotoc_md32", null ],
      [ "Using <tt>for_each_at_path_with_wildcard</tt> for JSONPath Queries (On-Demand)", "md_doc_2basics.html#autotoc_md33", [
        [ "Example Usage", "md_doc_2basics.html#autotoc_md34", null ]
      ] ],
      [ "C++20 Ranges Support", "md_doc_2basics.html#autotoc_md35", null ],
      [ "Key selectors", "md_doc_2basics.html#autotoc_md36", [
        [ "Binding fields straight to variables", "md_doc_2basics.html#autotoc_md37", null ],
        [ "Per-key callbacks", "md_doc_2basics.html#autotoc_md38", null ],
        [ "Mixing variables and callbacks", "md_doc_2basics.html#autotoc_md39", null ],
        [ "A single index-based callback", "md_doc_2basics.html#autotoc_md40", null ],
        [ "Nested objects", "md_doc_2basics.html#autotoc_md41", null ],
        [ "Error handling for key selectors", "md_doc_2basics.html#autotoc_md42", null ],
        [ "Compile-time restrictions", "md_doc_2basics.html#autotoc_md43", null ],
        [ "Inspecting the matching algorithm with <tt>describe()</tt>", "md_doc_2basics.html#autotoc_md44", null ]
      ] ],
      [ "Compile-Time JSONPath and JSON Pointer (C++26 Reflection)", "md_doc_2basics.html#autotoc_md45", null ],
      [ "Error handling", "md_doc_2basics.html#autotoc_md46", [
        [ "Error handling examples without exceptions", "md_doc_2basics.html#autotoc_md47", null ],
        [ "Disabling exceptions", "md_doc_2basics.html#autotoc_md48", null ],
        [ "Exceptions", "md_doc_2basics.html#autotoc_md49", null ],
        [ "Current location in document", "md_doc_2basics.html#autotoc_md50", null ],
        [ "Checking for trailing content", "md_doc_2basics.html#autotoc_md51", null ]
      ] ],
      [ "Rewinding", "md_doc_2basics.html#autotoc_md52", null ],
      [ "Newline-Delimited JSON (ndjson) and JSON lines", "md_doc_2basics.html#autotoc_md53", null ],
      [ "Parsing numbers inside strings", "md_doc_2basics.html#autotoc_md54", null ],
      [ "Dynamic Number Types", "md_doc_2basics.html#autotoc_md55", null ],
      [ "Infinity and NaN support", "md_doc_2basics.html#autotoc_md56", null ],
      [ "Raw strings from keys", "md_doc_2basics.html#autotoc_md57", null ],
      [ "General direct access to the raw JSON string", "md_doc_2basics.html#autotoc_md58", [
        [ "Raw JSON string for objects and arrays", "md_doc_2basics.html#autotoc_md59", null ]
      ] ],
      [ "Storing directly into an existing string instance", "md_doc_2basics.html#autotoc_md60", null ],
      [ "Thread safety", "md_doc_2basics.html#autotoc_md61", null ],
      [ "Limiting the maximum depth", "md_doc_2basics.html#autotoc_md62", null ],
      [ "Standard compliance", "md_doc_2basics.html#autotoc_md63", null ],
      [ "Backwards compatibility", "md_doc_2basics.html#autotoc_md64", null ],
      [ "Examples", "md_doc_2basics.html#autotoc_md65", null ],
      [ "Performance tips", "md_doc_2basics.html#autotoc_md66", null ],
      [ "Further reading", "md_doc_2basics.html#autotoc_md67", null ]
    ] ],
    [ "Builder", "md_doc_2builder.html", [
      [ "Overview: string_builder", "md_doc_2builder.html#autotoc_md68", null ],
      [ "Example: string_builder", "md_doc_2builder.html#autotoc_md69", [
        [ "C++20", "md_doc_2builder.html#autotoc_md70", null ]
      ] ],
      [ "C++26 static reflection", "md_doc_2builder.html#autotoc_md71", [
        [ "Without <tt>string_buffer</tt> instance", "md_doc_2builder.html#autotoc_md73", null ],
        [ "Extracting just some fields", "md_doc_2builder.html#autotoc_md74", null ],
        [ "Without <tt>string_buffer</tt> instance but with explicit error handling", "md_doc_2builder.html#autotoc_md75", null ],
        [ "Customization", "md_doc_2builder.html#autotoc_md76", null ],
        [ "Pretty formatted (fractured JSON)", "md_doc_2builder.html#autotoc_md77", null ]
      ] ]
    ] ],
    [ "Parse json at compile time", "md_doc_2compile__time.html", [
      [ "Introduction", "md_doc_2compile__time.html#autotoc_md79", null ],
      [ "Example", "md_doc_2compile__time.html#autotoc_md80", null ],
      [ "Concepts", "md_doc_2compile__time.html#autotoc_md81", null ],
      [ "Loading from disk", "md_doc_2compile__time.html#autotoc_md82", null ],
      [ "Limitations (compile-time errors)", "md_doc_2compile__time.html#autotoc_md83", null ]
    ] ],
    [ "Compile-Time JSONPath and JSON Pointer Accessors", "md_doc_2compile__time__accessors.html", [
      [ "Overview", "md_doc_2compile__time__accessors.html#autotoc_md85", null ],
      [ "Requirements", "md_doc_2compile__time__accessors.html#autotoc_md86", null ],
      [ "How It Works", "md_doc_2compile__time__accessors.html#autotoc_md87", null ],
      [ "Two Usage Modes", "md_doc_2compile__time__accessors.html#autotoc_md88", [
        [ "Mode 1: With Type Validation (Recommended)", "md_doc_2compile__time__accessors.html#autotoc_md89", null ],
        [ "Mode 2: Without Validation", "md_doc_2compile__time__accessors.html#autotoc_md90", null ]
      ] ],
      [ "JSONPath Syntax", "md_doc_2compile__time__accessors.html#autotoc_md91", [
        [ "Supported Syntax", "md_doc_2compile__time__accessors.html#autotoc_md92", null ],
        [ "Examples", "md_doc_2compile__time__accessors.html#autotoc_md93", null ]
      ] ],
      [ "JSON Pointer Syntax", "md_doc_2compile__time__accessors.html#autotoc_md94", [
        [ "Supported Syntax", "md_doc_2compile__time__accessors.html#autotoc_md95", null ],
        [ "Examples", "md_doc_2compile__time__accessors.html#autotoc_md96", null ]
      ] ],
      [ "API Reference", "md_doc_2compile__time__accessors.html#autotoc_md97", [
        [ "JSONPath Functions", "md_doc_2compile__time__accessors.html#autotoc_md98", null ],
        [ "JSON Pointer Functions", "md_doc_2compile__time__accessors.html#autotoc_md99", null ],
        [ "Direct Field Extraction", "md_doc_2compile__time__accessors.html#autotoc_md100", null ]
      ] ],
      [ "Complete Examples", "md_doc_2compile__time__accessors.html#autotoc_md101", [
        [ "Example 1: Validated Access", "md_doc_2compile__time__accessors.html#autotoc_md102", null ],
        [ "Example 2: Non-Validated Access", "md_doc_2compile__time__accessors.html#autotoc_md103", null ],
        [ "Example 3: Direct Extraction", "md_doc_2compile__time__accessors.html#autotoc_md104", null ]
      ] ],
      [ "Error Handling", "md_doc_2compile__time__accessors.html#autotoc_md105", null ],
      [ "Performance", "md_doc_2compile__time__accessors.html#autotoc_md106", null ],
      [ "Limitations", "md_doc_2compile__time__accessors.html#autotoc_md107", null ],
      [ "When to Use", "md_doc_2compile__time__accessors.html#autotoc_md108", null ],
      [ "See Also", "md_doc_2compile__time__accessors.html#autotoc_md109", null ]
    ] ],
    [ "The Document-Object-Model (DOM) front-end", "md_doc_2dom.html", [
      [ "DOM vs On-Demand", "md_doc_2dom.html#autotoc_md110", null ],
      [ "The Basics: Loading and Parsing JSON Documents using the DOM front-end", "md_doc_2dom.html#autotoc_md111", null ],
      [ "Using the Parsed JSON", "md_doc_2dom.html#autotoc_md112", [
        [ "Examples", "md_doc_2dom.html#autotoc_md113", null ]
      ] ],
      [ "Reverse Array Iteration", "md_doc_2dom.html#autotoc_md114", null ],
      [ "C++17 Support", "md_doc_2dom.html#autotoc_md115", null ],
      [ "C++20 Support", "md_doc_2dom.html#autotoc_md116", null ],
      [ "JSON Pointer", "md_doc_2dom.html#autotoc_md117", null ],
      [ "JSONPath", "md_doc_2dom.html#autotoc_md118", null ],
      [ "Using <tt>at_path_with_wildcard</tt> for JSONPath Queries", "md_doc_2dom.html#autotoc_md119", [
        [ "Example Usage", "md_doc_2dom.html#autotoc_md120", null ]
      ] ],
      [ "Error Handling", "md_doc_2dom.html#autotoc_md121", [
        [ "Error Handling Example", "md_doc_2dom.html#autotoc_md122", null ],
        [ "Exceptions", "md_doc_2dom.html#autotoc_md123", null ]
      ] ],
      [ "Tree Walking and JSON Element Types", "md_doc_2dom.html#autotoc_md124", null ],
      [ "Limiting the maximum depth", "md_doc_2dom.html#autotoc_md125", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_2dom.html#autotoc_md126", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_2dom.html#autotoc_md127", null ],
      [ "Best Use of the DOM API", "md_doc_2dom.html#autotoc_md128", null ],
      [ "Padding and Temporary Copies", "md_doc_2dom.html#autotoc_md129", null ],
      [ "Parsing Without Padding", "md_doc_2dom.html#autotoc_md130", null ],
      [ "Performance Tips", "md_doc_2dom.html#autotoc_md131", null ]
    ] ],
    [ "CPU Architecture-Specific Implementations", "md_doc_2implementation-selection.html", [
      [ "Overview", "md_doc_2implementation-selection.html#autotoc_md132", null ],
      [ "Runtime CPU Detection", "md_doc_2implementation-selection.html#autotoc_md133", null ],
      [ "Inspecting the Detected Implementation", "md_doc_2implementation-selection.html#autotoc_md134", null ],
      [ "Querying Available Implementations", "md_doc_2implementation-selection.html#autotoc_md135", null ],
      [ "Manually Selecting the Implementation", "md_doc_2implementation-selection.html#autotoc_md136", null ],
      [ "Checking that an Implementation can Run on your System", "md_doc_2implementation-selection.html#autotoc_md137", null ]
    ] ],
    [ "iterate_many", "md_doc_2iterate__many.html", [
      [ "Contents", "md_doc_2iterate__many.html#autotoc_md138", null ],
      [ "Motivation", "md_doc_2iterate__many.html#autotoc_md139", null ],
      [ "How it works", "md_doc_2iterate__many.html#autotoc_md140", [
        [ "Context", "md_doc_2iterate__many.html#autotoc_md141", null ],
        [ "Design", "md_doc_2iterate__many.html#autotoc_md142", null ],
        [ "Threads", "md_doc_2iterate__many.html#autotoc_md143", null ],
        [ "Parsing on many threads", "md_doc_2iterate__many.html#autotoc_md144", null ]
      ] ],
      [ "Support", "md_doc_2iterate__many.html#autotoc_md145", null ],
      [ "API", "md_doc_2iterate__many.html#autotoc_md146", null ],
      [ "Streaming directly from a memory-mapped file", "md_doc_2iterate__many.html#autotoc_md147", null ],
      [ "Use cases", "md_doc_2iterate__many.html#autotoc_md148", null ],
      [ "Tracking your position", "md_doc_2iterate__many.html#autotoc_md149", null ],
      [ "Incomplete streams", "md_doc_2iterate__many.html#autotoc_md150", null ],
      [ "Comma-separated documents", "md_doc_2iterate__many.html#autotoc_md151", [
        [ "Legacy <tt>allow_comma_separated</tt> parameter (deprecated)", "md_doc_2iterate__many.html#autotoc_md152", null ]
      ] ],
      [ "JSON Text Sequences (RFC 7464)", "md_doc_2iterate__many.html#autotoc_md153", null ],
      [ "JSON Array As A Document Stream", "md_doc_2iterate__many.html#autotoc_md154", null ],
      [ "C++20 features", "md_doc_2iterate__many.html#autotoc_md155", null ],
      [ "C++26 features (static reflection)", "md_doc_2iterate__many.html#autotoc_md156", [
        [ "Whitespace-delimited (default, NDJSON / JSON Lines)", "md_doc_2iterate__many.html#autotoc_md157", null ],
        [ "Comma-delimited documents", "md_doc_2iterate__many.html#autotoc_md158", null ],
        [ "A single JSON array as a stream of documents", "md_doc_2iterate__many.html#autotoc_md159", null ],
        [ "JSON Text Sequences (RFC 7464)", "md_doc_2iterate__many.html#autotoc_md160", null ]
      ] ]
    ] ],
    [ "A Better Way to Parse Documents?", "md_doc_2ondemand__design.html", [
      [ "Algorithm", "md_doc_2ondemand__design.html#autotoc_md165", [
        [ "DOM Parsers", "md_doc_2ondemand__design.html#autotoc_md161", null ],
        [ "Event-Based Parsers (SAX, SAJ, etc.)", "md_doc_2ondemand__design.html#autotoc_md162", null ],
        [ "Schema-Based Parser Generators", "md_doc_2ondemand__design.html#autotoc_md163", null ],
        [ "Type Blindness and Branch Misprediction", "md_doc_2ondemand__design.html#autotoc_md164", null ],
        [ "Starting the iteration", "md_doc_2ondemand__design.html#autotoc_md166", null ]
      ] ],
      [ "Design Features", "md_doc_2ondemand__design.html#autotoc_md167", [
        [ "String Parsing", "md_doc_2ondemand__design.html#autotoc_md168", null ],
        [ "Iteration Safety", "md_doc_2ondemand__design.html#autotoc_md169", null ],
        [ "Benefits of the On-Demand Approach", "md_doc_2ondemand__design.html#autotoc_md170", null ],
        [ "Limitations of the On-Demand Approach", "md_doc_2ondemand__design.html#autotoc_md171", null ],
        [ "Applicability of the On-Demand Approach", "md_doc_2ondemand__design.html#autotoc_md172", null ]
      ] ],
      [ "Checking Your CPU Selection (x64 systems)", "md_doc_2ondemand__design.html#autotoc_md173", null ]
    ] ],
    [ "parse_many", "md_doc_2parse__many.html", [
      [ "Contents", "md_doc_2parse__many.html#autotoc_md174", null ],
      [ "Motivation", "md_doc_2parse__many.html#autotoc_md175", null ],
      [ "Performance", "md_doc_2parse__many.html#autotoc_md176", null ],
      [ "How it works", "md_doc_2parse__many.html#autotoc_md177", [
        [ "Context", "md_doc_2parse__many.html#autotoc_md178", null ],
        [ "Design", "md_doc_2parse__many.html#autotoc_md179", null ],
        [ "Threads", "md_doc_2parse__many.html#autotoc_md180", null ]
      ] ],
      [ "Support", "md_doc_2parse__many.html#autotoc_md181", null ],
      [ "API", "md_doc_2parse__many.html#autotoc_md182", null ],
      [ "Use cases", "md_doc_2parse__many.html#autotoc_md183", null ],
      [ "Tracking your position", "md_doc_2parse__many.html#autotoc_md184", null ],
      [ "Streaming directly from a memory-mapped file", "md_doc_2parse__many.html#autotoc_md185", null ],
      [ "Incomplete streams", "md_doc_2parse__many.html#autotoc_md186", null ],
      [ "JSON Text Sequences (RFC 7464)", "md_doc_2parse__many.html#autotoc_md187", null ],
      [ "Comma-Separated Documents", "md_doc_2parse__many.html#autotoc_md188", null ],
      [ "JSON Array As A Document Stream", "md_doc_2parse__many.html#autotoc_md189", null ]
    ] ],
    [ "Performance Notes", "md_doc_2performance.html", [
      [ "NDEBUG macro", "md_doc_2performance.html#autotoc_md190", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_2performance.html#autotoc_md191", null ],
      [ "Reusing string buffers", "md_doc_2performance.html#autotoc_md192", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_2performance.html#autotoc_md193", null ],
      [ "Large files and huge page support", "md_doc_2performance.html#autotoc_md194", null ],
      [ "Number parsing", "md_doc_2performance.html#autotoc_md195", null ],
      [ "Visual Studio", "md_doc_2performance.html#autotoc_md196", null ],
      [ "Power Usage and Downclocking", "md_doc_2performance.html#autotoc_md197", null ],
      [ "Free Padding", "md_doc_2performance.html#autotoc_md198", null ]
    ] ],
    [ "Tape structure in simdjson", "md_doc_2tape.html", [
      [ "Example", "md_doc_2tape.html#autotoc_md200", [
        [ "The Tape", "md_doc_2tape.html#autotoc_md201", null ]
      ] ],
      [ "General formal of the tape elements", "md_doc_2tape.html#autotoc_md202", null ],
      [ "Simple JSON values", "md_doc_2tape.html#autotoc_md203", null ],
      [ "Integer and Double values", "md_doc_2tape.html#autotoc_md204", null ],
      [ "Big Integers", "md_doc_2tape.html#autotoc_md205", null ],
      [ "Root node", "md_doc_2tape.html#autotoc_md206", null ],
      [ "Strings", "md_doc_2tape.html#autotoc_md207", null ],
      [ "Arrays", "md_doc_2tape.html#autotoc_md208", null ],
      [ "Objects", "md_doc_2tape.html#autotoc_md209", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"amalgamated_8h_source.html",
"classsimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1value.html#a8e5f2ff46639e6c1cffb73d4e071e491",
"classsimdjson_1_1dom_1_1object_1_1iterator.html#a83b3c467632c017400f95ff05d2367b3",
"group__object.html#gaa0c31077aeadb6fc627d4c5750e58ecb",
"md_doc_2ondemand__design.html#autotoc_md163",
"structsimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1number.html#ab261642a30f5e26b0769ce701cd5b9f8"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';