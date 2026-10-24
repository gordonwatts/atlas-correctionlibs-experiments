from pathlib import Path
import shlex,subprocess,sys,os
root=Path('/home/gwatts/atlas/correctionlib_tests');os.chdir(root)
base=root/'optimization-campaign/checks';base.mkdir(exist_ok=True)
label=sys.argv[1]
with (base/f'{label}-build.log').open('w') as log:
    for command in (['cmake','--build','build','-j','4'],['cmake','--install','build'],['cmake','--build','build-overlay','-j','3']):
        subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT)
flags=(root/'build/CMakeFiles/_atlas_corrections.dir/flags.make').read_text()
includes=shlex.split(next(x.split('=',1)[1] for x in flags.splitlines() if x.startswith('CXX_INCLUDES =')))
link=shlex.split((root/'build/CMakeFiles/_atlas_corrections.dir/link.txt').read_text())
libraries=link[next(i for i,x in enumerate(link) if x.startswith('-Wl,-rpath,')):]
cpp='/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/check-smearing-exact.cxx'
exe=base/f'{label}-exact'
subprocess.run([link[0],'-O2','-g','-std=c++23','-I'+str(root/'src'),*includes,cpp,'-o',str(exe),*libraries],check=True)
with (base/f'{label}-tests.log').open('w') as log:
    subprocess.run([str(exe)],check=True,stdout=log,stderr=subprocess.STDOUT)
    subprocess.run([sys.executable,'-m','pytest','-q','tests'],check=True,stdout=log,stderr=subprocess.STDOUT)
print((base/f'{label}-tests.log').read_text(),flush=True)
