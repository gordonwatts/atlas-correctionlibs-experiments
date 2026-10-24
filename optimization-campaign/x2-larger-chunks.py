from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'c-nice/timing.py';s=p.read_text();assert '"--chunk-size","1000"' in s
p.write_text(s.replace('"--chunk-size","1000"','"--chunk-size","5000"'))
p=r/'scripts/run-columnar-full.py';s=p.read_text();assert '"--chunk-size",type=int,default=1000' in s
p.write_text(s.replace('"--chunk-size",type=int,default=1000','"--chunk-size",type=int,default=5000'))
