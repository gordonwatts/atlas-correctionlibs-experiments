# Snapshot scope

Git tracks the project sources, configuration, calibration payloads, documentation, scripts, lightweight timing/validation reports and the Path D dependency source copy. Large generated ROOT/NumPy datasets, builds, installed environments and downloaded wheels remain on disk and are ignored. `local-generated-artifacts.json` inventories the datasets by path and size; it is not a content backup.

The nested `external/correctionlib` checkout remains in place. Its pinned upstream commit, recursive submodule revisions and local patch are recorded here so its modified source can be reconstructed without nesting another repository in this one. Rebuild and environment scripts and existing dependency locks remain tracked. Existing acceptance reports are historical evidence, not a new validation run for this Git snapshot.
