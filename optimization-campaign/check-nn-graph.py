from pathlib import Path
import shlex,subprocess,sys
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
flags=(r/'build/CMakeFiles/_atlas_corrections.dir/flags.make').read_text();includes=shlex.split(next(x.split('=',1)[1] for x in flags.splitlines() if x.startswith('CXX_INCLUDES =')))
link=shlex.split((r/'build/CMakeFiles/_atlas_corrections.dir/link.txt').read_text());libraries=link[next(i for i,x in enumerate(link) if x.startswith('-Wl,-rpath,')):]
exe=r/'optimization-campaign/nn-checks'/('check-'+sys.argv[1]+'.bin')
subprocess.run([link[0],'-O2','-std=c++23','-pthread',*(['-DEIGEN_RUNTIME_NO_MALLOC','-UNDEBUG'] if sys.argv[1].startswith('26') else []),'-I'+str(r/'src'),*includes,str(w/'check-nn-graph.cxx'),'-o',str(exe),*libraries],cwd=r,check=True)
with (exe.with_suffix('.log')).open('w') as log:subprocess.run([str(exe)],cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
print(exe.with_suffix('.log').read_text())

