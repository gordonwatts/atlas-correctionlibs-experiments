"""Build the same adapter sources against the separately pinned Athena environment."""
from pathlib import Path
import shutil
r=Path(__file__).resolve().parents[1];src=r/"atlas-overlay";dest=r/"athena-overlay"
dest.mkdir(exist_ok=True)
for name in ["JetCalibTools","JetUncertainties","JetJvtEfficiency"]:
 if not (dest/name).exists():shutil.copytree(src/name,dest/name)
(dest/"CMakeLists.txt").write_text("""cmake_minimum_required(VERSION 3.25)
project(CorrectionLibAthenaOverlay VERSION 1.0.0 LANGUAGES CXX)
find_package(Athena 25.0.76 EXACT REQUIRED)
atlas_project(USE Athena 25.0.76)
atlas_cpack_setup()
""")

if not (dest/"CorrectionLibSmoke").exists():shutil.copytree(r/"athena-smoke-package",dest/"CorrectionLibSmoke")
