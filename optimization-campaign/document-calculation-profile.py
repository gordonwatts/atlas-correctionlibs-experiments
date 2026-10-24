from pathlib import Path
import shutil,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
text=(w/'calculation-profile-report.md').read_text()
(r/'calculation-profile-report.md').write_text(text.replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
# Make linked timer summary available beside native profile summaries.
shutil.copy2(r/'optimization-campaign/24-calculation-profile/runs/C-1/results/detailed-timers.json',r/'optimization-campaign/24-calculation-profile/detailed-timers.json')
for base in (w,r):
 for name in ('final-profile-report.md','x2-optimization.md'):
  p=base/name;p.write_text(p.read_text()+'\nThe subsequent [native calculation study](calculation-profile-report.md) resolves NNJVT, efficiency and grouped-systematic costs, including internal allocations and exact repeated NN features.\n')
shutil.copy2(w/'document-calculation-profile.py',r/'optimization-campaign/document-calculation-profile.py')
subprocess.run(['git','add','calculation-profile-report.md','final-profile-report.md','x2-optimization.md','optimization-campaign'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Document native calculation costs and exact NN reuse opportunities'],cwd=r,check=True)
