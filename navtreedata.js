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
    [ "The Basics", "md_doc_basics.html", [
      [ "Requirements", "md_doc_basics.html#autotoc_md0", null ],
      [ "Including simdjson", "md_doc_basics.html#autotoc_md1", null ],
      [ "Using simdjson with package managers", "md_doc_basics.html#autotoc_md2", null ],
      [ "Using simdjson as a CMake dependency", "md_doc_basics.html#autotoc_md3", null ],
      [ "Versions", "md_doc_basics.html#autotoc_md4", null ],
      [ "The basics: loading and parsing JSON documents", "md_doc_basics.html#autotoc_md5", null ],
      [ "Documents are iterators", "md_doc_basics.html#autotoc_md6", [
        [ "Parser, document and JSON scope", "md_doc_basics.html#autotoc_md7", null ]
      ] ],
      [ "string_view", "md_doc_basics.html#autotoc_md8", null ],
      [ "Avoiding pitfalls: enable development checks", "md_doc_basics.html#autotoc_md9", null ],
      [ "Using the parsed JSON", "md_doc_basics.html#autotoc_md10", [
        [ "Using the parsed JSON: additional examples", "md_doc_basics.html#autotoc_md11", null ]
      ] ],
      [ "Adding support for custom types", "md_doc_basics.html#autotoc_md12", [
        [ "1. Specialize <tt>simdjson::ondemand::value::get</tt> to get custom types (pre-C++20)", "md_doc_basics.html#autotoc_md13", null ],
        [ "2. Use <tt>tag_invoke</tt> for custom types (C++20)", "md_doc_basics.html#autotoc_md14", null ]
      ] ],
      [ "Minifying JSON strings without parsing", "md_doc_basics.html#autotoc_md15", null ],
      [ "UTF-8 validation (alone)", "md_doc_basics.html#autotoc_md16", null ],
      [ "JSON Pointer", "md_doc_basics.html#autotoc_md17", null ],
      [ "JSONPath", "md_doc_basics.html#autotoc_md18", null ],
      [ "Error handling", "md_doc_basics.html#autotoc_md19", [
        [ "Error handling examples without exceptions", "md_doc_basics.html#autotoc_md20", null ],
        [ "Disabling exceptions", "md_doc_basics.html#autotoc_md21", null ],
        [ "Exceptions", "md_doc_basics.html#autotoc_md22", null ],
        [ "Current location in document", "md_doc_basics.html#autotoc_md23", null ],
        [ "Checking for trailing content", "md_doc_basics.html#autotoc_md24", null ]
      ] ],
      [ "Rewinding", "md_doc_basics.html#autotoc_md25", null ],
      [ "Newline-Delimited JSON (ndjson) and JSON lines", "md_doc_basics.html#autotoc_md26", null ],
      [ "Parsing numbers inside strings", "md_doc_basics.html#autotoc_md27", null ],
      [ "Dynamic Number Types", "md_doc_basics.html#autotoc_md28", null ],
      [ "Raw strings from keys", "md_doc_basics.html#autotoc_md29", null ],
      [ "General direct access to the raw JSON string", "md_doc_basics.html#autotoc_md30", null ],
      [ "Storing directly into an existing string instance", "md_doc_basics.html#autotoc_md31", null ],
      [ "Thread safety", "md_doc_basics.html#autotoc_md32", null ],
      [ "Standard compliance", "md_doc_basics.html#autotoc_md33", null ],
      [ "Backwards compatibility", "md_doc_basics.html#autotoc_md34", null ],
      [ "Examples", "md_doc_basics.html#autotoc_md35", null ],
      [ "Performance tips", "md_doc_basics.html#autotoc_md36", null ],
      [ "Further reading", "md_doc_basics.html#autotoc_md37", null ]
    ] ],
    [ "The Document-Object-Model (DOM) front-end", "md_doc_dom.html", [
      [ "DOM vs On-Demand", "md_doc_dom.html#autotoc_md38", null ],
      [ "The Basics: Loading and Parsing JSON Documents using the DOM front-end", "md_doc_dom.html#autotoc_md39", null ],
      [ "Using the Parsed JSON", "md_doc_dom.html#autotoc_md40", [
        [ "Examples", "md_doc_dom.html#autotoc_md41", null ]
      ] ],
      [ "C++17 Support", "md_doc_dom.html#autotoc_md42", null ],
      [ "JSON Pointer", "md_doc_dom.html#autotoc_md43", null ],
      [ "JSONPath", "md_doc_dom.html#autotoc_md44", null ],
      [ "Error Handling", "md_doc_dom.html#autotoc_md45", [
        [ "Error Handling Example", "md_doc_dom.html#autotoc_md46", null ],
        [ "Exceptions", "md_doc_dom.html#autotoc_md47", null ]
      ] ],
      [ "Tree Walking and JSON Element Types", "md_doc_dom.html#autotoc_md48", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_dom.html#autotoc_md49", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_dom.html#autotoc_md50", null ],
      [ "Best Use of the DOM API", "md_doc_dom.html#autotoc_md51", null ],
      [ "Padding and Temporary Copies", "md_doc_dom.html#autotoc_md52", null ],
      [ "Performance Tips", "md_doc_dom.html#autotoc_md53", null ]
    ] ],
    [ "CPU Architecture-Specific Implementations", "md_doc_implementation_selection.html", [
      [ "Overview", "md_doc_implementation_selection.html#autotoc_md54", null ],
      [ "Runtime CPU Detection", "md_doc_implementation_selection.html#autotoc_md55", null ],
      [ "Inspecting the Detected Implementation", "md_doc_implementation_selection.html#autotoc_md56", null ],
      [ "Querying Available Implementations", "md_doc_implementation_selection.html#autotoc_md57", null ],
      [ "Manually Selecting the Implementation", "md_doc_implementation_selection.html#autotoc_md58", null ],
      [ "Checking that an Implementation can Run on your System", "md_doc_implementation_selection.html#autotoc_md59", null ]
    ] ],
    [ "iterate_many", "md_doc_iterate_many.html", [
      [ "Contents", "md_doc_iterate_many.html#autotoc_md60", null ],
      [ "Motivation", "md_doc_iterate_many.html#autotoc_md61", null ],
      [ "How it works", "md_doc_iterate_many.html#autotoc_md62", [
        [ "Context", "md_doc_iterate_many.html#autotoc_md63", null ],
        [ "Design", "md_doc_iterate_many.html#autotoc_md64", null ],
        [ "Threads", "md_doc_iterate_many.html#autotoc_md65", null ]
      ] ],
      [ "Support", "md_doc_iterate_many.html#autotoc_md66", null ],
      [ "API", "md_doc_iterate_many.html#autotoc_md67", null ],
      [ "Use cases", "md_doc_iterate_many.html#autotoc_md68", null ],
      [ "Tracking your position", "md_doc_iterate_many.html#autotoc_md69", null ],
      [ "Incomplete streams", "md_doc_iterate_many.html#autotoc_md70", null ],
      [ "Comma-separated documents", "md_doc_iterate_many.html#autotoc_md71", null ],
      [ "C++20 features", "md_doc_iterate_many.html#autotoc_md72", null ]
    ] ],
    [ "A Better Way to Parse Documents?", "md_doc_ondemand_design.html", [
      [ "Algorithm", "md_doc_ondemand_design.html#autotoc_md77", [
        [ "DOM Parsers", "md_doc_ondemand_design.html#autotoc_md73", null ],
        [ "Event-Based Parsers (SAX, SAJ, etc.)", "md_doc_ondemand_design.html#autotoc_md74", null ],
        [ "Schema-Based Parser Generators", "md_doc_ondemand_design.html#autotoc_md75", null ],
        [ "Type Blindness and Branch Misprediction", "md_doc_ondemand_design.html#autotoc_md76", null ],
        [ "Starting the iteration", "md_doc_ondemand_design.html#autotoc_md78", null ]
      ] ],
      [ "Design Features", "md_doc_ondemand_design.html#autotoc_md79", [
        [ "String Parsing", "md_doc_ondemand_design.html#autotoc_md80", null ],
        [ "Iteration Safety", "md_doc_ondemand_design.html#autotoc_md81", null ],
        [ "Benefits of the On-Demand Approach", "md_doc_ondemand_design.html#autotoc_md82", null ],
        [ "Limitations of the On-Demand Approach", "md_doc_ondemand_design.html#autotoc_md83", null ],
        [ "Applicability of the On-Demand Approach", "md_doc_ondemand_design.html#autotoc_md84", null ]
      ] ],
      [ "Checking Your CPU Selection (x64 systems)", "md_doc_ondemand_design.html#autotoc_md85", null ]
    ] ],
    [ "parse_many", "md_doc_parse_many.html", [
      [ "Contents", "md_doc_parse_many.html#autotoc_md86", null ],
      [ "Motivation", "md_doc_parse_many.html#autotoc_md87", null ],
      [ "Performance", "md_doc_parse_many.html#autotoc_md88", null ],
      [ "How it works", "md_doc_parse_many.html#autotoc_md89", [
        [ "Context", "md_doc_parse_many.html#autotoc_md90", null ],
        [ "Design", "md_doc_parse_many.html#autotoc_md91", null ],
        [ "Threads", "md_doc_parse_many.html#autotoc_md92", null ]
      ] ],
      [ "Support", "md_doc_parse_many.html#autotoc_md93", null ],
      [ "API", "md_doc_parse_many.html#autotoc_md94", null ],
      [ "Use cases", "md_doc_parse_many.html#autotoc_md95", null ],
      [ "Tracking your position", "md_doc_parse_many.html#autotoc_md96", null ],
      [ "Incomplete streams", "md_doc_parse_many.html#autotoc_md97", null ]
    ] ],
    [ "Performance Notes", "md_doc_performance.html", [
      [ "NDEBUG directive", "md_doc_performance.html#autotoc_md98", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_performance.html#autotoc_md99", null ],
      [ "Reusing string buffers", "md_doc_performance.html#autotoc_md100", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_performance.html#autotoc_md101", null ],
      [ "Large files and huge page support", "md_doc_performance.html#autotoc_md102", null ],
      [ "Number parsing", "md_doc_performance.html#autotoc_md103", null ],
      [ "Visual Studio", "md_doc_performance.html#autotoc_md104", null ],
      [ "Power Usage and Downclocking", "md_doc_performance.html#autotoc_md105", null ],
      [ "Free Padding", "md_doc_performance.html#autotoc_md106", null ]
    ] ],
    [ "Tape structure in simdjson", "md_doc_tape.html", [
      [ "Example", "md_doc_tape.html#autotoc_md108", [
        [ "The Tape", "md_doc_tape.html#autotoc_md109", null ]
      ] ],
      [ "General formal of the tape elements", "md_doc_tape.html#autotoc_md110", null ],
      [ "Simple JSON values", "md_doc_tape.html#autotoc_md111", null ],
      [ "Integer and Double values", "md_doc_tape.html#autotoc_md112", null ],
      [ "Root node", "md_doc_tape.html#autotoc_md113", null ],
      [ "Strings", "md_doc_tape.html#autotoc_md114", null ],
      [ "Arrays", "md_doc_tape.html#autotoc_md115", null ],
      [ "Objects", "md_doc_tape.html#autotoc_md116", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Modules", "modules.html", "modules" ],
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
"classsimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1document__stream_1_1iterator.html#adee0c974b66f734b3b67f5600bbe594f",
"classsimdjson_1_1dom_1_1element.html#a83adcf723abff34d7c1e8ec0e69d967b",
"group__array.html#ga1e023cb72adae168dc7157776976ad75",
"namespacesimdjson.html#a4ee1eccef6fb8cdcdaf794e3e3d9ee66",
"structsimdjson_1_1simdjson__result_3_01_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1document_01_4.html#a09cb25b160c9db3f5279ef513db3deac",
"structsimdjson_1_1simdjson__result_3_01dom_1_1element_01_4.html#a75e13d079118b2e532b872026debf1aa"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';