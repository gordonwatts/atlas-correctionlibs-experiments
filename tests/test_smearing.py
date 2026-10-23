import sys
from pathlib import Path
import numpy as np,ROOT
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"install/python"))
from _atlas_corrections import smear_batch
def test_reference_trandom3_seed_sign_and_rejection():
 for data in (False,True):
  phi=[0.,1e-8,-.125,2.9,1.37]
  sigma=[.1,.2,.3,.4,2.]
  variation=[0.,-.1,.1,-.3,10.]
  expected=[]
  for p,s,v in zip(phi,sigma,variation):
   if v==0 or (data and v>0) or (not data and v<0):
    expected.append(1.);continue
   seed=int((123000 if data else 200000)*abs(p)) or (34545654 if data else 45583453)
   random=ROOT.TRandom3(seed);width=np.sqrt((s+abs(v))**2-s*s)
   draw=-1.
   while draw<0:draw=random.Gaus(1.,width)
   expected.append(draw)
  assert np.array_equal(smear_batch(phi,sigma,variation,data),expected)
  assert np.array_equal(smear_batch(phi[::-1],sigma[::-1],variation[::-1],data)[::-1],expected)
