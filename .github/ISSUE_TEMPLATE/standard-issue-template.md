---
name: Standard issue template
about: Issue
title: ''
labels: ''
assignees: ''

---

Before submitting an issue, please ensure that you have read the documentation:

* Basics is an overview of how to use simdjson and its APIs: https://github.com/simdjson/simdjson/blob/master/doc/basics.md
* Performance shows some more advanced scenarios and how to tune for them: https://github.com/simdjson/simdjson/blob/master/doc/performance.md
* Contributing: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md
* We follow the [JSON specification as described by RFC 8259](https://www.rfc-editor.org/rfc/rfc8259.txt) (T. Bray, 2017).

We do not make changes to simdjson without clearly identifiable benefits, which typically means either performance improvements, bug fixes or new features. Avoid bike-shedding: we all have opinions about how to write code, but we want to focus on what makes simdjson objectively better.

Is your issue:

1. A bug report? If so, please point at a reproducible test. Indicate whether you are willing or able to provide a bug fix as a pull request.

2. A build issue? If so, provide all possible details regarding your system configuration. If we cannot reproduce your issue, we cannot fix it.

3. A feature request? Please provide a clear rationale for the feature. Be advised that simdjson is a community-based project: you should consider providing help.

4. A documentation issue? Can you suggest an improvement?


If you plan to contribute to simdjson, please read our 
* CONTRIBUTING guide: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md and our
* HACKING guide: https://github.com/simdjson/simdjson/blob/master/HACKING.md
