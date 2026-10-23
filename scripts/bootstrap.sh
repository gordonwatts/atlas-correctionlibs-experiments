#!/usr/bin/env bash
set +e
cd /home/gwatts/atlas/correctionlib_tests
python3 scripts/download-dependencies.py || exit $?
source setup.sh
setup_status=$?
if [ "$setup_status" -ne 0 ]; then exit "$setup_status"; fi
set -e
mkdir -p external logs payloads results
if ! test -d external/correctionlib/.git; then git clone --recursive --branch v2.7.0 --depth 1 https://github.com/cms-nanoAOD/correctionlib.git external/correctionlib; fi
if ! test -d venv; then python -m venv --system-site-packages venv; fi
source venv/bin/activate
python -m pip install --no-index --find-links external/wheels -r config/requirements-wheels.lock
if git -C external/correctionlib apply --check "$PWD/patches/correctionlib-full-precision-json.patch"; then
  git -C external/correctionlib apply "$PWD/patches/correctionlib-full-precision-json.patch"
else
  git -C external/correctionlib apply --reverse --check "$PWD/patches/correctionlib-full-precision-json.patch" || exit 1
fi
python -m pip install --no-index --no-build-isolation ./external/correctionlib
git -C external/correctionlib rev-parse HEAD > external/correctionlib.commit
python -m pip freeze > external/python-packages.txt
correction config --version
