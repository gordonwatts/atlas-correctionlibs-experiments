# Declarative path D

This fourth path keeps A/B/C intact and moves correction physics, branching, precision points and per-jet stage order into versioned payload graphs.

- [Architecture and boundaries](docs/design.md)
- [Source traceability](docs/source-traceability.json)
- [Full acceptance report](results/validation-summary.md)
- [Workflow timing](results/timing.md)
- [Graph compiler](python/compile_graphs.py)
- [Thin runtime adapters](python/adapters.py)
- [Serializable schema](python/schema.py)
- [Exported JSON schema](schemas/graph-v1.json)
- [Optional CorrectionLib fork patch](patches/correctionlib-graph-v1.patch)

Run inside WSL2 atlas_al9, from /home/gwatts/atlas/correctionlib_tests:

    bash path-d/scripts/run.sh

After acceptance, an optional single full-workflow timing is:

    source path-d/setup.sh
    python path-d/scripts/benchmark.py
    python path-d/scripts/report.py

The fork is based on CorrectionLib v2.7.0 fd69c2ef33edbe9a1d4bc714430d6ccda374be00, retaining the required full-precision JSON patch. The new graph envelope requires correctionlib-2.7.0+graph-1; its separately bundled tables remain standard schema v2. This is loaded graph interpretation, not graph-specialized code generation.

Inspection copies in the Windows chat workspace are for reading only. All generation, build, execution and validation happen in the WSL workspace.
