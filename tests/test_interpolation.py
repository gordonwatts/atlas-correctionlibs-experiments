import random
import ROOT
import correctionlib
from atlas_correctionlib.nodes import correction,linear,bilinear

def evaluator(name,inputs,node):
    import json
    return correctionlib.CorrectionSet.from_string(json.dumps(dict(schema_version=2,corrections=[correction(name,inputs,node)])))[name]

def test_th1_boundaries():
    from array import array
    h=ROOT.TH1D("linear_test","",4,array("d",[0,1,3,6,10]))
    vals=[0.7,1.2,0.9,1.3]
    for i,v in enumerate(vals,1): h.SetBinContent(i,v)
    xs=[h.GetBinCenter(i) for i in range(1,5)]
    c=evaluator("test",[("x","real")],linear(xs,vals,"x"))
    for x in [-10.,0.,0.5,1.,2.,3.,4.5,6.,8.,10.,30.]:
        assert abs(c.evaluate(x)-h.Interpolate(x))<1e-12

def test_th2_centres_edges_and_random():
    from array import array
    h=ROOT.TH2D("bilinear_test","",3,array("d",[0,1,3,7]),3,array("d",[-2,0,4,10]))
    vals=[[0.8,1.1,0.9],[1.2,0.95,1.05],[1.4,0.85,1.3]]
    for i,row in enumerate(vals,1):
        for j,v in enumerate(row,1):h.SetBinContent(i,j,v)
    xs=[h.GetXaxis().GetBinCenter(i) for i in range(1,4)]
    ys=[h.GetYaxis().GetBinCenter(i) for i in range(1,4)]
    c=evaluator("test",[("x","real"),("y","real")],bilinear(xs,ys,vals,"x","y"))
    rng=random.Random(123)
    points=[(x,y) for x in [1e-6,0.5,1.,2.,3.,5.,7.-1e-6] for y in [-2.+1e-6,-1.,0.,2.,4.,7.,10.-1e-6]]
    points += [(rng.uniform(1e-6,7.-1e-6),rng.uniform(-2.+1e-6,10.-1e-6)) for _ in range(1000)]
    for x,y in points:
        assert abs(c.evaluate(x,y)-h.Interpolate(x,y))<1e-12,(x,y,c.evaluate(x,y),h.Interpolate(x,y))

def test_th2_single_bin_axis():
    h=ROOT.TH2D("single_bin","",3,0,3,1,0,2)
    vals=[[0.8],[1.2],[0.9]]
    for i,row in enumerate(vals,1):h.SetBinContent(i,1,row[0])
    c=evaluator("single",[("x","real"),("y","real")],bilinear([0.5,1.5,2.5],[1.],vals,"x","y"))
    for x in [0.001,0.5,1.,1.5,2.,2.5,2.999]:
        for y in [0.001,1.,1.999]:
            assert abs(c.evaluate(x,y)-h.Interpolate(x,y))<1e-12
