Short title (summary):

Description
- What did you change and why? (1-3 sentences)
- Issue reproduced / related issue: link the issue if relevant (e.g. #123)

Type of change
- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / cleanup
- [ ] Documentation / tests
- [ ] Other (please describe):

How to verify / test
- Add additional tests to verify bugs or new features.
- If you claim performance gains, you should provide benchmark numbers using high quality benchmarking code.


Please read before contributing:
- CONTRIBUTING: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md
- HACKING: https://github.com/simdjson/simdjson/blob/master/HACKING.md



If you can, we recommend running our tests with the sanitizers turned on.
For non-Visual Studio users, it is as easy as doing:



```bash
cmake -B build -D SIMDJSON_SANITIZE=ON -D SIMDJSON_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build
```


Our CI checks, among other things, for trailing whitespace. If a test fails for that reason,
use the "artifacts" button to download the artifact and inspect the problematic lines,
or run `scripts/remove_trailing_whitespace.sh` locally if you have a bash shell and `sed`.


Checklist before submitting
- [ ] I added/updated tests covering my change (if applicable)
- [ ] Code builds locally and passes my check
- [ ] Documentation / README updated if needed
- [ ] Commits are atomic and messages are clear
- [ ] I linked the related issue (if applicable)

Final notes
- For large PRs, prefer smaller incremental PRs or request staged review.

Thanks for the contribution!

