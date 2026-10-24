from pathlib import Path
import hashlib, json, re, shlex, shutil, subprocess
root=Path('/home/gwatts/atlas/correctionlib_tests')
out=root/'optimization-campaign/28-lookup-comparison-v2'
out.mkdir(exist_ok=False)
source=out/'test-src';source.mkdir()
for name in ('backend.hxx','systematics.hxx','efficiency.hxx'):
    shutil.copy2(root/'src'/name,source/name)
p=source/'systematics.hxx';text=p.read_text()
old='return oneInput?correction->evaluate({pt}):correction->evaluate({pt,eta});'
assert old in text
text=text.replace('bool oneInput=false;','bool oneInput=false;\n  Lookup* lookup=nullptr;')
text=text.replace(old,'return useRootLookup?lookup->root(pt,eta):('+old[len('return '):-1]+');')
old='return {c,c->inputs().size()==1};'
assert old in text
text=text.replace(old,'return {c,c->inputs().size()==1,bindLookup(c->name(),c)};')
p.write_text(text)
flags=(root/'build/CMakeFiles/_atlas_corrections.dir/flags.make').read_text()
includes=shlex.split(re.search(r'CXX_INCLUDES = (.*)',flags)[1])
link=shlex.split((root/'build/CMakeFiles/_atlas_corrections.dir/link.txt').read_text())
libs=[x for x in link if x.endswith('.so') and x.startswith('/')]
rpaths=[x for x in link if x.startswith('-Wl,-rpath,')]
atlas=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt')
cmd=[link[0],'-O2','-g','-DNDEBUG','-DXAOD_STANDALONE','-std=gnu++23',*includes,'-I'+str(atlas/'include'),'-I'+str(source),'-I'+str(root/'lookup-benchmark'),str(root/'lookup-benchmark/benchmark.cxx'),'-o',str(out/'benchmark'),*rpaths,'-Wl,-rpath,'+str(atlas/'lib'),str(atlas/'lib/libJetUncertaintiesLib.so'),*libs]
(out/'compile-command.txt').write_text(shlex.join(cmd)+'\n')
subprocess.run(cmd,check=True)
files=[root/'payloads/uncertainty-tables.json.gz',root/'payloads/uncertainty-manifest.json',root/'results/overhead-study/kernel-inputs.bin',*[(root/'src'/n) for n in ('backend.hxx','systematics.hxx','efficiency.hxx')]]
metadata=dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),hashes={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},production_modified=False,rounds=7,discarded_rounds=[0],order='rotated for lookups; alternating for shared evaluator',input='196606 nominal jets from existing full-sample native replay; systematic inputs are 7 columns of the 21-column binary')
(out/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
print('Prepared lookup benchmark',flush=True)

