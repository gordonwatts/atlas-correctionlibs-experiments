from pathlib import Path
import subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');e=r/'external/correctionlib'
p=e/'src/correction.cc';s=p.read_text()
assert s.count('const auto bins = std::get<_NonUniformBins>(bins_);')==1
p.write_text(s.replace('const auto bins = std::get<_NonUniformBins>(bins_);','const auto& bins = std::get<_NonUniformBins>(bins_);'))
(r/'vendor-patches/bin-edge-reference.patch').write_bytes(subprocess.check_output(['git','diff'],cwd=e))
subprocess.run(['git','-c','user.name=Gordon Watts','-c','user.email=gwatts@uw.edu','commit','-am','Avoid copying non-uniform bin edges during evaluation'],cwd=e,check=True)
