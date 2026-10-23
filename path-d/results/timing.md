# Path D workflow timing

One post-correctness run: **249.32 s**, 20,000 events / 196,606 jets.

Complete three-stage diagnostic workflow after environment setup, including imports, input/feature preparation, repeated kinematic evaluation, output and reference comparisons. Warm caches possible. Single run, not an isolated kernel or repeated timing estimate.

| Phase | Wall seconds |
|---|---:|
| nominal | 20.01 |
| systematics | 58.31 |
| jvt | 171.00 |

The earlier A/B/C means (237.26 / 290.70 / 167.70 s) use four runs with the first discarded. D is a single differently structured diagnostic workflow, so this is not a controlled evaluator-speed comparison.
