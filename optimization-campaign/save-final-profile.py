from pathlib import Path
import json,shutil,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
p=r/'optimization-campaign/23-final-scoped-profile/runs/C-1'
s=json.loads((p/'results/scoped-profile.json').read_text());wall=json.loads((p/'timing.json').read_text())['wall_seconds']
for label in ('22-final-python-profile','23-final-scoped-profile'):
 source=r/'optimization-campaign'/label;dest=w/'optimization-results'/label;dest.mkdir(exist_ok=True)
 for name in ('python-stage-0.txt','functions.json','metadata.json','python-profile.log','run.log'):
  if (source/name).exists():shutil.copy2(source/name,dest/name)
 for name in ('scoped-profile.json','full-stage-timing.json','nominal-comparison.json','systematics-comparison.json','jvt-comparison.json'):
  item=source/'runs/C-1/results'/name
  if item.exists():shutil.copy2(item,dest/name)
 shutil.copy2(source/'runs/C-1/timing.json',dest/'timing.json')
 for stage in ('nominal','systematics','jvt'):assert json.loads((source/f'runs/C-1/results/{stage}-comparison.json').read_text())['passed']
shutil.copy2(p/'results/scoped-profile.json',p.parents[1]/'scoped-profile.json')
shutil.copy2(p/'timing.json',p.parents[1]/'timing.json')
sections=s['sections'];load=sum(sections[key]['seconds'] for key in ('Nominal tool load','Systematic tool load','JVT tool load'))
rows=[('Read A references for systematics and JVT',sections['A reference reads']['seconds']),('JVT evaluation, including array preparation and cache',sections['JVT evaluate including array preparation and exact cache']['seconds']),('Load nominal, systematic and JVT tools',load),('Write ROOT output objects',sections['ROOT output assignment']['seconds']),('Produce all 117 systematic variations',sections['Systematic variation production']['seconds']),('Nominal input preparation and evaluation',sections['Nominal input preparation and native evaluation']['seconds'])]
remainder=wall-sum(value for _,value in rows)
lines=['# Profile of the selected C-nice implementation','',f'A fresh profile of the selected checkpoint-20 implementation confirms that reference-data reading is now the largest individually measured section. The direct elapsed-time run took **{wall:.2f} s** including process startup. This is one diagnostic run, not a replacement for the **26.44 ± 0.42 s** benchmark. It used the same 20,000 events, 1,000-event chunks, one-CPU affinity, one native worker, all outputs and all A comparisons. Both new profile runs passed those comparisons.','','| Measured section | Elapsed seconds |','|---|---:|']
for name,value in rows:lines.append(f'| {name} | {value:.2f} |')
lines += [f'| Remaining time, not separately attributed | {remainder:.2f} |',f'| **Outer process total** | **{wall:.2f}** |','',
'These scoped timers surround separate operations. The remainder includes process/import overhead, PHYSLITE reading, nominal reference reads, saved-kinematics reading and checks, comparisons and array reductions outside the wrappers, NPZ output, and other orchestration. It has not been subdivided. Tool loading breaks down into nominal 2.30 s, systematics 0.68 s and JVT 0.76 s. The 7.53 s reference-read number includes only the grouped systematic/JVT reference reader, not every input read.', '',
'The existing uninstrumented benchmark stage means remain nominal 4.43 s, systematics 9.86 s and JVT 11.74 s. Those broader stage totals include I/O and validation and must not be added to the table above.', '',
'## What this suggests', '',
'The next substantial target is reference-data access: a more efficient reference representation or read strategy could accelerate this diagnostic benchmark while preserving every comparison. Simply omitting reference validation would change the benchmark scope and would not establish another optimization of the same workload. A production analysis normally would not compare every variation to a saved A output.', '',
'JVT is the largest measured calculation section. A native profile of NNJVT and efficiency evaluation is the next step to split its 4.42 s further. This report measures the complete JVT call, including array preparation and exact-cache checks; it does not claim that all of that time is neural-network inference. Tool construction and serialization also remain meaningful costs. No new physics approximation is justified by this report alone.', '',
'## Evidence and limits', '',
'The fresh cProfile run is retained for inspection, but some entries have inconsistent call accounting (for example, zero primitive calls and zero cumulative time despite nonzero self time). Its reported function-time total is about 40 s and includes substantial instrumentation effects. Do not treat its cumulative entries as an additive wall-time budget or scale them directly to the 26.44 s benchmark. The table above comes from independent perf_counter wrappers around the selected operations, not from those cProfile entries. No new C++ instruction profile was captured in this follow-up.', '',
'[Scoped measurements](optimization-results/23-final-scoped-profile/scoped-profile.json), [outer timing](optimization-results/23-final-scoped-profile/timing.json), [cProfile text](optimization-results/22-final-python-profile/python-stage-0.txt), and [all recorded Python functions](optimization-results/22-final-python-profile/functions.json). The raw pstats file remains in WSL under `optimization-campaign/22-final-python-profile/runs/C-1/profile-0.pstats`.', '',
'The instrumentation is confined to separate profiling scripts and directories; the selected physics implementation and benchmark configuration were not changed. Profiles, comparisons and this report are preserved with local commits. Nothing was pushed.','']
text='\n'.join(lines);(w/'final-profile-report.md').write_text(text);(r/'final-profile-report.md').write_text(text.replace('optimization-results/','optimization-campaign/'))
for base in (w,r):
 path=base/'x2-optimization.md';path.write_text(path.read_text()+'\nA fresh profile of the selected implementation is available in [the final profile report](final-profile-report.md), with direct wall-time attribution for reference reads, JVT, tool loading and output.\n')
for name in ('profile-x2-final.py','profile-x2-final.sh','scoped-profile-x2.py','run-scoped-profile-x2.py','run-scoped-profile-x2.sh','save-final-profile.py'):shutil.copy2(w/name,r/'optimization-campaign'/name)
subprocess.run(['git','add','optimization-campaign','final-profile-report.md','x2-optimization.md'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Profile selected C-nice workflow with scoped wall timers and retained cProfile diagnostics'],cwd=r,check=True)
print(json.dumps({'wall_seconds':wall,'remainder':remainder,'passed':True}))

