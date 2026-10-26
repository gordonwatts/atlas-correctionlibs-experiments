#!/usr/bin/env python3
"""Build an experimental x86_64/CPython wheel from the current WSL build.

Run after setup.sh in the authoritative checkout. This stages files under a new
output directory and leaves the existing install, venv, and results untouched.
"""
import argparse
import base64
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile


def needed(path):
    output = subprocess.check_output(["readelf", "-d", str(path)], text=True)
    return re.findall(r"\(NEEDED\).*?\[(.*?)\]", output)


def add_startup_hook(wheel, hook):
    """Put the .pth in site-packages, with a valid wheel RECORD entry."""
    name = "atlas_complete_corrections_startup.pth"
    content = hook.read_bytes()
    temporary = wheel.with_name(wheel.name + ".tmp")
    with zipfile.ZipFile(wheel) as source, zipfile.ZipFile(temporary, "w") as target:
        record = next(x for x in source.namelist() if x.endswith(".dist-info/RECORD"))
        for member in source.infolist():
            if member.filename != record:
                target.writestr(member, source.read(member.filename))
        target.writestr(name, content)
        rows = list(csv.reader(io.StringIO(source.read(record).decode())))
        digest = base64.urlsafe_b64encode(hashlib.sha256(content).digest()).rstrip(b"=").decode()
        rows.insert(-1, [name, "sha256=" + digest, str(len(content))])
        data = io.StringIO()
        csv.writer(data, lineterminator="\n").writerows(rows)
        target.writestr(record, data.getvalue())
    temporary.replace(wheel)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--native", type=Path, help="separately built _atlas_corrections extension")
    parser.add_argument("--python-tag", default="cp313", choices=("cp39", "cp313", "cp314"))
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output.resolve()
    if output.exists():
        parser.error(f"output already exists: {output}")
    stage = output / "stage"
    package = stage / "atlas_complete_corrections"
    libs = package / "libs"
    libs.mkdir(parents=True)

    native = args.native.resolve() if args.native else next((root / "install/python").glob("_atlas_corrections*.so"))
    if f"cpython-{args.python_tag.removeprefix('cp')}" not in native.name:
        parser.error(f"extension does not match {args.python_tag}: {native}")
    shutil.copy2(native, package / native.name)
    shutil.copytree(root / "payloads", package / "payloads")
    shutil.copytree(root / "python/atlas_correctionlib", package / "atlas_correctionlib",
                    ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))

    facade = (root / "c-nice/atlas_jet_corrections.py").read_text()
    facade = facade.replace("from _atlas_corrections import Nominal", "from ._atlas_corrections import Nominal")
    facade = facade.replace("def load(cls, bundle):", "def load(cls, bundle=None):")
    facade = facade.replace("obj.bundle = Path(bundle).resolve()", "obj.bundle = Path(bundle_path() if bundle is None else bundle).resolve()")
    facade = facade.replace("from pathlib import Path", "from pathlib import Path\nfrom . import bundle_path", 1)
    (package / "facade.py").write_text(facade)

    subpackage = package / "atlas_correctionlib"
    for path in subpackage.glob("*.py"):
        source = path.read_text()
        source = source.replace("from _atlas_corrections import ", "from .._atlas_corrections import ")
        source = source.replace("import _atlas_corrections", "from .. import _atlas_corrections")
        if path.name == "systematics.py":
            source = source.replace("import numpy as np, correctionlib", "import numpy as np")
            source = source.replace("  self.tables=correctionlib.CorrectionSet", "  import correctionlib\n  self.tables=correctionlib.CorrectionSet")
            source = source.replace("   self._tables=correctionlib.CorrectionSet", "   import correctionlib\n   self._tables=correctionlib.CorrectionSet")
        path.write_text(source)

    # ldd lists the transitive closure while the release environment is active.
    resolved = subprocess.check_output(["ldd", str(native)], text=True)
    library_paths = {}
    for line in resolved.splitlines():
        match = re.match(r"\s*(\S+)\s+=>\s+(/\S+)", line)
        if not match:
            continue
        name, location = match.groups()
        if location.startswith(("/lib", "/usr/lib")):
            continue
        library_paths[name] = Path(location)
    if "libcorrectionlib.so" not in library_paths or "liblwtnn.so" not in library_paths:
        raise RuntimeError("Missing correctionlib or lwtnn in linked library closure")
    for name, location in library_paths.items():
        shutil.copy2(location, libs / name)

    # Load dependencies before dependents; preloading by SONAME lets the Python
    # extension resolve all NEEDED entries without an external LD_LIBRARY_PATH.
    order = []
    visited = set()

    def visit(name):
        if name in visited:
            return
        visited.add(name)
        for dependency in needed(libs / name):
            if dependency in library_paths:
                visit(dependency)
        order.append(name)

    for name in library_paths:
        visit(name)
    (package / "runtime_libs.json").write_text(json.dumps(order, indent=2) + "\n")
    (package / "__init__.py").write_text('''"""Complete local ATLAS jet correction bundle (experimental)."""
import ctypes
import json
from pathlib import Path

_HERE = Path(__file__).resolve().parent
for _name in json.loads((_HERE / "runtime_libs.json").read_text()):
    ctypes.CDLL(str(_HERE / "libs" / _name), mode=ctypes.RTLD_GLOBAL)

def bundle_path():
    """Path to the wheel's complete frozen correction payloads."""
    return _HERE / "payloads"

from .facade import JetCorrections
from .atlas_correctionlib.systematics import Systematics
from .atlas_correctionlib.jvt import Jvt

__all__ = ["JetCorrections", "Systematics", "Jvt", "bundle_path"]
''')
    (stage / "pyproject.toml").write_text('''[build-system]
requires = ["setuptools", "wheel"]
build-backend = "setuptools.build_meta"
''')
    (stage / "setup.py").write_text('''from pathlib import Path
from setuptools import setup, find_packages, Distribution

class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True

root = Path("atlas_complete_corrections")
data = [str(p.relative_to(root)) for p in root.rglob("*") if p.is_file() and p.suffix != ".py"]
setup(name="atlas-complete-corrections", version="0.1.0", packages=find_packages(),
      package_data={"atlas_complete_corrections": data}, include_package_data=True,
      python_requires=">=3.9,<3.15", install_requires=["numpy>=2,<3", "awkward>=2,<3"],
      distclass=BinaryDistribution, zip_safe=False)
''')
    subprocess.run([sys.executable, "setup.py", "bdist_wheel", "--plat-name", "linux_x86_64",
                    "--dist-dir", str(output / "dist")], cwd=stage, check=True,
                   stdout=(output / "build.log").open("w"), stderr=subprocess.STDOUT)
    wheel = next((output / "dist").glob("*.whl"))
    if f"-{args.python_tag}-{args.python_tag}-" not in wheel.name:
        subprocess.run([sys.executable, "-m", "wheel", "tags", "--python-tag", args.python_tag,
                        "--abi-tag", args.python_tag, str(wheel)], check=True,
                       stdout=(output / "retag.log").open("w"), stderr=subprocess.STDOUT)
        wheel.unlink()
        wheel = next((output / "dist").glob("*.whl"))
    add_startup_hook(wheel, Path(__file__).with_name("atlas_complete_corrections_startup.pth"))
    print(wheel)
    print(f"wheel size: {wheel.stat().st_size} bytes")
    print(f"bundled {len(library_paths)} shared libraries and {len(list((package / 'payloads').rglob('*')))} payload entries")


if __name__ == "__main__":
    main()
