# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = ["atlas-complete-corrections"]
# [tool.uv.sources]
# atlas-complete-corrections = { path = "./atlas_complete_corrections-0.1.0-cp314-cp314-linux_x86_64.whl" }
# ///

"""Run with `uv run run_local_wheel.py` after placing the wheel beside it."""

import sys
from atlas_complete_corrections import JetCorrections, bundle_path

assert sys.version_info[:2] == (3, 14)
assert (bundle_path() / "nominal.json.gz").is_file()
assert JetCorrections.load().bundle == bundle_path()
print("Python", sys.version.split()[0], "bundle", bundle_path())
