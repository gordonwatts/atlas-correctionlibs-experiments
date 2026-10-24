from pathlib import Path
import shutil,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests')
w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
for name in ('x2-optimization.md','correctionlib-interface-handoff.md','c-nice-optimization.md','single-thread-profiling.md','four-path-timing-comparison.md','c-nice/README.md'):
 (r/name).write_text((w/name).read_text().replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
for name in ('x2-final-docs.py','x2-sync-report-links.py'):shutil.copy2(w/name,r/'optimization-campaign'/name)
subprocess.run(['git','add','*.md','c-nice/README.md','optimization-campaign/x2-final-docs.py','optimization-campaign/x2-sync-report-links.py'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Resolve report links to authoritative source and result paths in WSL'],cwd=r,check=True)
