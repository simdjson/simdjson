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
      [ "The Basics: Loading and Parsing JSON Documents", "md_doc_basics.html#autotoc_md5", null ],
      [ "Documents are Iterators", "md_doc_basics.html#autotoc_md6", [
        [ "Parser, Document and JSON Scope", "md_doc_basics.html#autotoc_md7", null ]
      ] ],
      [ "string_view", "md_doc_basics.html#autotoc_md8", null ],
      [ "Using the Parsed JSON", "md_doc_basics.html#autotoc_md9", [
        [ "Using the Parsed JSON: Additional examples", "md_doc_basics.html#autotoc_md10", null ]
      ] ],
      [ "Minifying JSON strings without parsing", "md_doc_basics.html#autotoc_md11", null ],
      [ "UTF-8 validation (alone)", "md_doc_basics.html#autotoc_md12", null ],
      [ "JSON Pointer", "md_doc_basics.html#autotoc_md13", null ],
      [ "Error Handling", "md_doc_basics.html#autotoc_md14", [
        [ "Error Handling Examples without Exceptions", "md_doc_basics.html#autotoc_md15", null ],
        [ "Disabling Exceptions", "md_doc_basics.html#autotoc_md16", null ],
        [ "Exceptions", "md_doc_basics.html#autotoc_md17", null ],
        [ "Current location in document", "md_doc_basics.html#autotoc_md18", null ],
        [ "Checking for trailing content", "md_doc_basics.html#autotoc_md19", null ]
      ] ],
      [ "Rewinding", "md_doc_basics.html#autotoc_md20", null ],
      [ "Newline-Delimited JSON (ndjson) and JSON lines", "md_doc_basics.html#autotoc_md21", null ],
      [ "Parsing Numbers Inside Strings", "md_doc_basics.html#autotoc_md22", null ],
      [ "Dynamic Number Types", "md_doc_basics.html#autotoc_md23", null ],
      [ "Raw Strings", "md_doc_basics.html#autotoc_md24", null ],
      [ "General Direct Access to the Raw JSON String", "md_doc_basics.html#autotoc_md25", null ],
      [ "Thread Safety", "md_doc_basics.html#autotoc_md26", null ],
      [ "Standard Compliance", "md_doc_basics.html#autotoc_md27", null ],
      [ "Backwards Compatibility", "md_doc_basics.html#autotoc_md28", null ],
      [ "Examples", "md_doc_basics.html#autotoc_md29", null ],
      [ "Performance Tips", "md_doc_basics.html#autotoc_md30", null ]
    ] ],
    [ "The Document-Object-Model (DOM) front-end", "md_doc_dom.html", [
      [ "DOM vs On Demand", "md_doc_dom.html#autotoc_md31", null ],
      [ "The Basics: Loading and Parsing JSON Documents using the DOM front-end", "md_doc_dom.html#autotoc_md32", null ],
      [ "Using the Parsed JSON", "md_doc_dom.html#autotoc_md33", [
        [ "Examples", "md_doc_dom.html#autotoc_md34", null ]
      ] ],
      [ "C++17 Support", "md_doc_dom.html#autotoc_md35", null ],
      [ "JSON Pointer", "md_doc_dom.html#autotoc_md36", null ],
      [ "Error Handling", "md_doc_dom.html#autotoc_md37", [
        [ "Error Handling Example", "md_doc_dom.html#autotoc_md38", null ],
        [ "Exceptions", "md_doc_dom.html#autotoc_md39", null ]
      ] ],
      [ "Tree Walking and JSON Element Types", "md_doc_dom.html#autotoc_md40", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_dom.html#autotoc_md41", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_dom.html#autotoc_md42", null ],
      [ "Best Use of the DOM API", "md_doc_dom.html#autotoc_md43", null ],
      [ "Padding and Temporary Copies", "md_doc_dom.html#autotoc_md44", null ],
      [ "Performance Tips", "md_doc_dom.html#autotoc_md45", null ]
    ] ],
    [ "CPU Architecture-Specific Implementations", "md_doc_implementation_selection.html", [
      [ "Overview", "md_doc_implementation_selection.html#autotoc_md46", null ],
      [ "Runtime CPU Detection", "md_doc_implementation_selection.html#autotoc_md47", null ],
      [ "Inspecting the Detected Implementation", "md_doc_implementation_selection.html#autotoc_md48", null ],
      [ "Querying Available Implementations", "md_doc_implementation_selection.html#autotoc_md49", null ],
      [ "Manually Selecting the Implementation", "md_doc_implementation_selection.html#autotoc_md50", null ],
      [ "Checking that an Implementation can Run on your System", "md_doc_implementation_selection.html#autotoc_md51", null ]
    ] ],
    [ "iterate_many", "md_doc_iterate_many.html", [
      [ "Contents", "md_doc_iterate_many.html#autotoc_md52", null ],
      [ "Motivation", "md_doc_iterate_many.html#autotoc_md53", null ],
      [ "How it works", "md_doc_iterate_many.html#autotoc_md54", [
        [ "Context", "md_doc_iterate_many.html#autotoc_md55", null ],
        [ "Design", "md_doc_iterate_many.html#autotoc_md56", null ],
        [ "Threads", "md_doc_iterate_many.html#autotoc_md57", null ]
      ] ],
      [ "Support", "md_doc_iterate_many.html#autotoc_md58", null ],
      [ "API", "md_doc_iterate_many.html#autotoc_md59", null ],
      [ "Use cases", "md_doc_iterate_many.html#autotoc_md60", null ],
      [ "Tracking your position", "md_doc_iterate_many.html#autotoc_md61", null ],
      [ "Incomplete streams", "md_doc_iterate_many.html#autotoc_md62", null ],
      [ "Comma-separated documents", "md_doc_iterate_many.html#autotoc_md63", null ]
    ] ],
    [ "A Better Way to Parse Documents?", "md_doc_ondemand_design.html", [
      [ "Algorithm", "md_doc_ondemand_design.html#autotoc_md68", [
        [ "DOM Parsers", "md_doc_ondemand_design.html#autotoc_md64", null ],
        [ "Event-Based Parsers (SAX, SAJ, etc.)", "md_doc_ondemand_design.html#autotoc_md65", null ],
        [ "Schema-Based Parser Generators", "md_doc_ondemand_design.html#autotoc_md66", null ],
        [ "Type Blindness and Branch Misprediction", "md_doc_ondemand_design.html#autotoc_md67", null ],
        [ "Starting the iteration", "md_doc_ondemand_design.html#autotoc_md69", null ]
      ] ],
      [ "Design Features", "md_doc_ondemand_design.html#autotoc_md70", [
        [ "String Parsing", "md_doc_ondemand_design.html#autotoc_md71", null ],
        [ "Iteration Safety", "md_doc_ondemand_design.html#autotoc_md72", null ],
        [ "Benefits of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md73", null ],
        [ "Limitations of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md74", null ],
        [ "Applicability of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md75", null ]
      ] ],
      [ "Checking Your CPU Selection (x64 systems)", "md_doc_ondemand_design.html#autotoc_md76", null ]
    ] ],
    [ "parse_many", "md_doc_parse_many.html", [
      [ "Contents", "md_doc_parse_many.html#autotoc_md77", null ],
      [ "Motivation", "md_doc_parse_many.html#autotoc_md78", null ],
      [ "Performance", "md_doc_parse_many.html#autotoc_md79", null ],
      [ "How it works", "md_doc_parse_many.html#autotoc_md80", [
        [ "Context", "md_doc_parse_many.html#autotoc_md81", null ],
        [ "Design", "md_doc_parse_many.html#autotoc_md82", null ],
        [ "Threads", "md_doc_parse_many.html#autotoc_md83", null ]
      ] ],
      [ "Support", "md_doc_parse_many.html#autotoc_md84", null ],
      [ "API", "md_doc_parse_many.html#autotoc_md85", null ],
      [ "Use cases", "md_doc_parse_many.html#autotoc_md86", null ],
      [ "Tracking your position", "md_doc_parse_many.html#autotoc_md87", null ],
      [ "Incomplete streams", "md_doc_parse_many.html#autotoc_md88", null ]
    ] ],
    [ "Performance Notes", "md_doc_performance.html", [
      [ "NDEBUG directive", "md_doc_performance.html#autotoc_md89", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_performance.html#autotoc_md90", null ],
      [ "Reusing string buffers", "md_doc_performance.html#autotoc_md91", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_performance.html#autotoc_md92", null ],
      [ "Large files and huge page support", "md_doc_performance.html#autotoc_md93", null ],
      [ "Number parsing", "md_doc_performance.html#autotoc_md94", null ],
      [ "Visual Studio", "md_doc_performance.html#autotoc_md95", null ],
      [ "Power Usage and Downclocking", "md_doc_performance.html#autotoc_md96", null ]
    ] ],
    [ "Tape structure in simdjson", "md_doc_tape.html", [
      [ "Example", "md_doc_tape.html#autotoc_md98", [
        [ "The Tape", "md_doc_tape.html#autotoc_md99", null ]
      ] ],
      [ "General formal of the tape elements", "md_doc_tape.html#autotoc_md100", null ],
      [ "Simple JSON values", "md_doc_tape.html#autotoc_md101", null ],
      [ "Integer and Double values", "md_doc_tape.html#autotoc_md102", null ],
      [ "Root node", "md_doc_tape.html#autotoc_md103", null ],
      [ "Strings", "md_doc_tape.html#autotoc_md104", null ],
      [ "Arrays", "md_doc_tape.html#autotoc_md105", null ],
      [ "Objects", "md_doc_tape.html#autotoc_md106", null ]
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
"",
"classsimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1object.html#aa14649f261616070b04829c9ba6b28da",
"classsimdjson_1_1dom_1_1object_1_1iterator.html#a2049ae539dca6999cc33300d212a5014",
"logger-inl_8h.html#a273e5f25cfdef1064a5225a47d85eb4f",
"namespacesimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand.html#ac915f0c06e5ab0363e09593bba651330ab45cffe084dd3d20d928bee85e7b0f21",
"structsimdjson_1_1simdjson__result_3_01_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1document__reference_01_4.html#ab10a7c98d14d3a40aef1a8b1e9cd29d2"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';