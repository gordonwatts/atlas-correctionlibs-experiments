from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'python/atlas_correctionlib/reference.py'
p.write_text('''"""Bounded, on-demand grouped reads of the same validation reference branches."""
class ReferenceReader:
    def __init__(self,tree,names,events,block_size=16):
        self.tree=tree
        self.names=list(names)
        self.index={name:i for i,name in enumerate(self.names)}
        if len(self.index)!=len(self.names):raise ValueError("Duplicate reference branch")
        self.events=events
        self.block_size=block_size
        self.block=None
        self.values={}

    def read(self,name):
        block=self.index[name]//self.block_size
        if block!=self.block:
            start=block*self.block_size
            self.values=self.tree.arrays(self.names[start:start+self.block_size],entry_stop=self.events,how=dict)
            self.block=block
        return self.values[name]
''')
p=r/'scripts/run-columnar-systematics.py';s=p.read_text()
s=s.replace('from atlas_correctionlib.systematics import Systematics','from atlas_correctionlib.systematics import Systematics\nfrom atlas_correctionlib.reference import ReferenceReader')
s=s.replace('engine=Systematics(r/"payloads");checks={};seen=[]','engine=Systematics(r/"payloads");checks={};seen=[]\nreference=ReferenceReader(ref,["check_"+coord+"_"+name for name in engine.engine.names() for coord in ("pt","eta","phi","m")],a.events)')
s=s.replace('ref["check_"+coord+"_"+name].array(entry_stop=a.events)','reference.read("check_"+coord+"_"+name)')
p.write_text(s)
p=r/'scripts/run-columnar-jvt.py';s=p.read_text()
s=s.replace('from atlas_correctionlib.jvt import Jvt','from atlas_correctionlib.jvt import Jvt\nfrom atlas_correctionlib.reference import ReferenceReader')
needle='jvt=Jvt(r/"payloads",None);checks={}'
assert needle in s
s=s.replace(needle,needle+'''
reference_names=[prefix+name for name in engine.engine.names() for prefix in ("check_NNJvt_","jet_select_jvt_","jet_jvtEfficiency_FixedEffPt_","weight_jvt_effSF_FixedEffPt_")]
reference_names += [prefix+name for name in ("JET_NNJvtEfficiency__1up","JET_NNJvtEfficiency__1down") for prefix in ("jet_jvtEfficiency_FixedEffPt_","weight_jvt_effSF_FixedEffPt_")]
reference=ReferenceReader(ref,reference_names,a.events)''')
s=s.replace('truth=ref[name].array(entry_stop=a.events)','truth=reference.read(name)')
p.write_text(s)
