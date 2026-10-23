# Proposed upstream precision fix

Base: cms-nanoAOD/correctionlib v2.7.0, commit fd69c2ef33edbe9a1d4bc714430d6ccda374be00.

The patch in patches/correctionlib-full-precision-json.patch adds RapidJSON kParseFullPrecisionFlag to JSON parsing entry points. Schema version and evaluation nodes are unchanged.

A reproducible example is the JSON constant 2.5384821903340438. The default parser in this build returns 2.538482190334044, while full-precision parsing preserves the original double. tests/test_payload_precision.py checks 500 deterministic double round trips; it fails without the patch and passes with it.

For the ATLAS demonstration, one-bit changes in response constants led to eight failing near-zero masses in the full sample after four-vector reconstruction. With full-precision parsing, every nominal intermediate pt, eta, phi and mass matches the authoritative AnalysisBase output exactly over 196,606 jets.

The patch is prepared locally for review; no upstream issue or pull request has been submitted. A proposed upstream test should cover string, uncompressed-file and compressed-file input. Full-precision parsing affects initialization; it does not change event-time expression evaluation.
