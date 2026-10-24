from pathlib import Path
import shlex, subprocess
root=Path('/home/gwatts/atlas/correctionlib_tests')
out=root/'optimization-campaign/28-lookup-comparison-v2'
cmd=shlex.split((out/'compile-command.txt').read_text())
cmd.append(next(x.replace('libCore.so','libHist.so') for x in cmd if x.endswith('/libCore.so')))
cmd[cmd.index(str(root/'lookup-benchmark/benchmark.cxx'))]=str(root/'lookup-benchmark/jvt.cxx')
cmd[cmd.index(str(out/'benchmark'))]=str(out/'jvt-benchmark')
assert not (out/'jvt.json').exists()
(out/'jvt-compile-command.txt').write_text(shlex.join(cmd)+'\n')
subprocess.run(cmd,check=True)
with (out/'jvt.log').open('w') as log:
    subprocess.run([out/'jvt-benchmark',root,out],stdout=log,stderr=subprocess.STDOUT,check=True)
