Contributing
============

The simdjson library is an open project written in C++. Contributions are invited. Contributors
agree to the project's license.

We have an extensive list of issues, and contributions toward any of these issues is invited.
Contributions can take the form of code samples, better documentation or design ideas. 

In particular, the following contributions are invited:

- The library is focused on performance. Well-documented performance optimization are invited.
- Fixes to known or newly discovered bugs are always welcome. Typically, a bug fix should come with
  a test demonstrating that the bug has been fixed.
- The simdjson library is advanced software and maintainability and flexibility are always a
  concern. Specific contributions to improve maintainability and flexibility are invited.

We discourage the following types of contributions:

- Code refactoring. We all have our preferences as to how code should be written, but unnecessary
  refactoring can waste time and introduce new bugs. If you believe that refactoring is needed, you
  first must explain how it helps in concrete terms. Does it improve the performance?
- Applications of new language features for their own sake. Using advanced C++ language constructs
  is actually a negative as it may reduce portability (to old compilers, old standard libraries and
  systems) and reduce accessibility (to programmers that have not kept up), so it must be offsetted
  by clear gains like performance or maintainability. When in doubt, avoid advanced C++ features
  (beyond C++11).
- Style formatting. In general, please abstain from reformatting code just to make it look prettier.
  Though code formatting is important, it can also be a waste of time if several contributors try to
  tweak the code base toward their own preference. Please do not introduce unneeded white-space
  changes.

In short, most code changes should either bring new features or better performance. We want to avoid unmotivated code changes.

Guidelines
----------

Contributors are encouraged to :

- Document their changes. Though we do not enforce a rule regarding code comments, we prefer that non-trivial algorithms and techniques be somewhat documented in the code.
- Follow as much as possible the existing code style. We do not enforce a specific code style, but we prefer consistency.
- Modify as few lines of code as possible when working on an issue. The more lines you modify, the harder it is for your fellow human beings to understand what is going on.
- Tools may report "problems" with the code, but we never delegate programming to tools: if there is a problem with the code, we need to understand it. Thus we will not "fix" code merely to please a static analyzer if we do not understand.
- Provide tests for any new feature. We will not merge a new feature without tests.

Code of Conduct
---------------

Though we do not have a formal code of conduct, we will not tolerate bullying, bigotry or
intimidation. Everyone is welcome to contribute.

Getting Started Hacking
-----------------------

An overview of simdjson's directory structure, with pointers to architecture and design
considerations and other helpful notes, can be found at [HACKING.md](HACKING.md).
