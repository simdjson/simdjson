---
name: Standard issue template
about: Issue
title: ''
labels: ''
assignees: ''

---

Before submitting an issue, please ensure that you have read the documentation:

* [Basics](doc/basics.md) is an overview of how to use simdjson and its APIs.
* [Performance](doc/performance.md) shows some more advanced scenarios and how to tune for them.
* [Implementation Selection](doc/implementation-selection.md) describes runtime CPU detection and how you can work with it.
* [Contributing](https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md)

Keep in mind that we do not make changes to simdjson without clearly identifiable benefits, which typically means either performance improvements, bug fixes or new features. Avoid bike-shedding: we all have opinions about how to write code, but we want to focus on what makes simdjson objectively better.

Is your issue:

1. A bug report? If so, please point at a reproducible test. Indicate whether you are willing or able to provide a bug fix as a pull request.

2. A build issue? If so, provide all possible details regarding your system configuration. If we cannot reproduce your issue, we cannot fix it.

3. A feature request? Please provide a clear rationale for the feature. Be advised that simdjson is a community-based project: you should consider providing help.

4. A documentation issue? Can you suggest an improvement?
