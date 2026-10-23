import correctionlib
from pathlib import Path
def test_negative_momentum_reset_does_not_clamp_positive_momentum():
 cs=correctionlib.CorrectionSet.from_file(str(Path(__file__).resolve().parents[1]/"payloads/nominal.json.gz"))
 c=cs["pileup_scale"]
 assert c.evaluate(100.,1.,90.,20.)*100==10
 assert abs(c.evaluate(100.,1.,90.,9.)*100-1)<1e-14
 assert c.evaluate(100.,1.,90.,10.)==0
 for feature in ["chargedFraction","EM3","nTrk","trackWIDTH"]:
  assert cs["gsc_"+feature].evaluate(50.,.5,0.,0.)==1
  assert cs["gsc_"+feature].evaluate(50.,.5,0.,-1.)==1
 assert cs["gsc_Tile0"].evaluate(50.,.5,0.,-1.)==1
