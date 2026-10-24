from pathlib import Path
import re,shutil,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
p=w/'x2-optimization.md';s=p.read_text();old=(w/'c-nice-optimization.md').read_text()
rows=[line for line in old.splitlines() if re.match(r'\| \d\d-',line)]
rows=[line.replace(' | ', ' | ',1) for line in rows]
for i,line in enumerate(rows):
 if any(f'| {n:02d}-' in line for n in range(5,9)):rows[i]=line.replace(' | `',' **Historical four-worker run.** | `')
start=s.index('| Step |');end=s.index('\n\n',start)
current=s[start:end].splitlines();s=s[:start]+'\n'.join(current[:2]+rows+current[2:])+s[end:]
s=s.replace('# Exact-result single-core optimization toward 2×','# Complete C-nice optimization timing history')
s=s.replace('Three runs per checkpoint;', 'The table includes every optimization checkpoint from the original 155.86 s baseline. Steps 05–08 used four native workers; steps 09 onward use one worker pinned to one CPU. Earlier steps 00–04 used one native worker without the later affinity policy. Do not interpret the thread-policy transitions as isolated implementation gains. Historical A/B/C/D, nominal-only and no-calibration comparisons remain in [the four-path comparison](four-path-timing-comparison.md). Profiling runs 22–24 are diagnostic studies, not additional accepted benchmark checkpoints.\n\nThree runs per checkpoint;')
p.write_text(s)
p=w/'c-nice-optimization.md';s=p.read_text();a=s.index('**Current single-threaded result:');b=s.index('\n\n',a);s=s[:a]+'**Selected single-threaded result: 26.44 ± 0.42 s.** See the [complete timing history, starting at the original 155.86 s baseline](x2-optimization.md). This report retains the first campaign’s detailed evidence.'+s[b:];p.write_text(s)
for name in ('x2-optimization.md','c-nice-optimization.md'):(r/name).write_text((w/name).read_text().replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
shutil.copy2(w/'consolidate-timing-history.py',r/'optimization-campaign/consolidate-timing-history.py')
subprocess.run(['git','add','x2-optimization.md','c-nice-optimization.md','optimization-campaign/consolidate-timing-history.py'],cwd=r,check=True);subprocess.run(['git','commit','-q','-m','Consolidate timing history from original baseline and label thread-policy changes'],cwd=r,check=True)
