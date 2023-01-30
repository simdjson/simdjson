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
      [ "C++11 Support and string_view", "md_doc_basics.html#autotoc_md8", null ],
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
        [ "Current location in document", "md_doc_basics.html#autotoc_md18", null ]
      ] ],
      [ "Rewinding", "md_doc_basics.html#autotoc_md19", null ],
      [ "Direct Access to the Raw String", "md_doc_basics.html#autotoc_md20", null ],
      [ "Newline-Delimited JSON (ndjson) and JSON lines", "md_doc_basics.html#autotoc_md21", null ],
      [ "Parsing Numbers Inside Strings", "md_doc_basics.html#autotoc_md22", null ],
      [ "Dynamic Number Types", "md_doc_basics.html#autotoc_md23", null ],
      [ "Raw Strings", "md_doc_basics.html#autotoc_md24", null ],
      [ "Thread Safety", "md_doc_basics.html#autotoc_md25", null ],
      [ "Standard Compliance", "md_doc_basics.html#autotoc_md26", null ],
      [ "Backwards Compatibility", "md_doc_basics.html#autotoc_md27", null ]
    ] ],
    [ "The Document-Object-Model (DOM) front-end", "md_doc_dom.html", [
      [ "DOM vs On Demand", "md_doc_dom.html#autotoc_md28", null ],
      [ "The Basics: Loading and Parsing JSON Documents using the DOM front-end", "md_doc_dom.html#autotoc_md29", null ],
      [ "Using the Parsed JSON", "md_doc_dom.html#autotoc_md30", [
        [ "Examples", "md_doc_dom.html#autotoc_md31", null ]
      ] ],
      [ "C++17 Support", "md_doc_dom.html#autotoc_md32", null ],
      [ "JSON Pointer", "md_doc_dom.html#autotoc_md33", null ],
      [ "Error Handling", "md_doc_dom.html#autotoc_md34", [
        [ "Error Handling Example", "md_doc_dom.html#autotoc_md35", null ],
        [ "Exceptions", "md_doc_dom.html#autotoc_md36", null ]
      ] ],
      [ "Tree Walking and JSON Element Types", "md_doc_dom.html#autotoc_md37", null ],
      [ "Reusing the parser for maximum efficiency", "md_doc_dom.html#autotoc_md38", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_dom.html#autotoc_md39", null ],
      [ "Best Use of the DOM API", "md_doc_dom.html#autotoc_md40", null ],
      [ "Padding and Temporary Copies", "md_doc_dom.html#autotoc_md41", null ]
    ] ],
    [ "CPU Architecture-Specific Implementations", "md_doc_implementation_selection.html", [
      [ "Overview", "md_doc_implementation_selection.html#autotoc_md42", null ],
      [ "Runtime CPU Detection", "md_doc_implementation_selection.html#autotoc_md43", null ],
      [ "Inspecting the Detected Implementation", "md_doc_implementation_selection.html#autotoc_md44", null ],
      [ "Querying Available Implementations", "md_doc_implementation_selection.html#autotoc_md45", null ],
      [ "Manually Selecting the Implementation", "md_doc_implementation_selection.html#autotoc_md46", null ],
      [ "Checking that an Implementation can Run on your System", "md_doc_implementation_selection.html#autotoc_md47", null ]
    ] ],
    [ "iterate_many", "md_doc_iterate_many.html", [
      [ "Contents", "md_doc_iterate_many.html#autotoc_md48", null ],
      [ "Motivation", "md_doc_iterate_many.html#autotoc_md49", null ],
      [ "How it works", "md_doc_iterate_many.html#autotoc_md50", [
        [ "Context", "md_doc_iterate_many.html#autotoc_md51", null ],
        [ "Design", "md_doc_iterate_many.html#autotoc_md52", null ],
        [ "Threads", "md_doc_iterate_many.html#autotoc_md53", null ]
      ] ],
      [ "Support", "md_doc_iterate_many.html#autotoc_md54", null ],
      [ "API", "md_doc_iterate_many.html#autotoc_md55", null ],
      [ "Use cases", "md_doc_iterate_many.html#autotoc_md56", null ],
      [ "Tracking your position", "md_doc_iterate_many.html#autotoc_md57", null ],
      [ "Incomplete streams", "md_doc_iterate_many.html#autotoc_md58", null ]
    ] ],
    [ "A Better Way to Parse Documents?", "md_doc_ondemand_design.html", [
      [ "Algorithm", "md_doc_ondemand_design.html#autotoc_md63", [
        [ "DOM Parsers", "md_doc_ondemand_design.html#autotoc_md59", null ],
        [ "Event-Based Parsers (SAX, SAJ, etc.)", "md_doc_ondemand_design.html#autotoc_md60", null ],
        [ "Schema-Based Parser Generators", "md_doc_ondemand_design.html#autotoc_md61", null ],
        [ "Type Blindness and Branch Misprediction", "md_doc_ondemand_design.html#autotoc_md62", null ],
        [ "Starting the iteration", "md_doc_ondemand_design.html#autotoc_md64", null ]
      ] ],
      [ "Design Features", "md_doc_ondemand_design.html#autotoc_md65", [
        [ "String Parsing", "md_doc_ondemand_design.html#autotoc_md66", null ],
        [ "Iteration Safety", "md_doc_ondemand_design.html#autotoc_md67", null ],
        [ "Benefits of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md68", null ],
        [ "Limitations of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md69", null ],
        [ "Applicability of the On Demand Approach", "md_doc_ondemand_design.html#autotoc_md70", null ]
      ] ],
      [ "Checking Your CPU Selection (x64 systems)", "md_doc_ondemand_design.html#autotoc_md71", null ]
    ] ],
    [ "parse_many", "md_doc_parse_many.html", [
      [ "Contents", "md_doc_parse_many.html#autotoc_md72", null ],
      [ "Motivation", "md_doc_parse_many.html#autotoc_md73", null ],
      [ "Performance", "md_doc_parse_many.html#autotoc_md74", null ],
      [ "How it works", "md_doc_parse_many.html#autotoc_md75", [
        [ "Context", "md_doc_parse_many.html#autotoc_md76", null ],
        [ "Design", "md_doc_parse_many.html#autotoc_md77", null ],
        [ "Threads", "md_doc_parse_many.html#autotoc_md78", null ]
      ] ],
      [ "Support", "md_doc_parse_many.html#autotoc_md79", null ],
      [ "API", "md_doc_parse_many.html#autotoc_md80", null ],
      [ "Use cases", "md_doc_parse_many.html#autotoc_md81", null ],
      [ "Tracking your position", "md_doc_parse_many.html#autotoc_md82", null ],
      [ "Incomplete streams", "md_doc_parse_many.html#autotoc_md83", null ]
    ] ],
    [ "Performance Notes", "md_doc_performance.html", [
      [ "Reusing the parser for maximum efficiency", "md_doc_performance.html#autotoc_md84", null ],
      [ "Reusing string buffers", "md_doc_performance.html#autotoc_md85", null ],
      [ "Server Loops: Long-Running Processes and Memory Capacity", "md_doc_performance.html#autotoc_md86", null ],
      [ "Large files and huge page support", "md_doc_performance.html#autotoc_md87", null ],
      [ "Number parsing", "md_doc_performance.html#autotoc_md88", null ],
      [ "Visual Studio", "md_doc_performance.html#autotoc_md89", null ],
      [ "Power Usage and Downclocking", "md_doc_performance.html#autotoc_md90", null ]
    ] ],
    [ "Tape structure in simdjson", "md_doc_tape.html", [
      [ "Example", "md_doc_tape.html#autotoc_md92", [
        [ "The Tape", "md_doc_tape.html#autotoc_md93", null ]
      ] ],
      [ "General formal of the tape elements", "md_doc_tape.html#autotoc_md94", null ],
      [ "Simple JSON values", "md_doc_tape.html#autotoc_md95", null ],
      [ "Integer and Double values", "md_doc_tape.html#autotoc_md96", null ],
      [ "Root node", "md_doc_tape.html#autotoc_md97", null ],
      [ "Strings", "md_doc_tape.html#autotoc_md98", null ],
      [ "Arrays", "md_doc_tape.html#autotoc_md99", null ],
      [ "Objects", "md_doc_tape.html#autotoc_md100", null ]
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
"annotated.html",
"classsimdjson_1_1_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1object__iterator.html#a4bda7b73aebddb2b5a66670d70110bc5",
"classsimdjson_1_1dom_1_1object_1_1iterator.html#ac82e68be1f179df92423d879114fbf55",
"logger_8h_source.html",
"ppc64_2bitmask_8h_source.html",
"structsimdjson_1_1simdjson__result_3_01_s_i_m_d_j_s_o_n___i_m_p_l_e_m_e_n_t_a_t_i_o_n_1_1ondemand_1_1value_01_4.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';