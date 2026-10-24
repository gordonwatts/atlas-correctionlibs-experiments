from pathlib import Path
p=Path('/home/gwatts/atlas/correctionlib_tests/scripts/run-columnar-jvt.py')
s=p.read_text().replace('nominal.npz");counts=', 'nominal.npz")\ncounts=')
p.write_text(s)
