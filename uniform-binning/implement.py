from pathlib import Path
import shutil
root=Path('/home/gwatts/atlas/correctionlib_tests')
win=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
p=root/'python/atlas_correctionlib/nodes.py'
original=p.read_text()
old='''def binning(name, edges, content, flow="error"):
    return dict(nodetype="binning", input=name, edges=[float(e) if not isinstance(e,str) else e for e in edges], content=list(content), flow=flow)
'''
replacement='''# Cache axis checks during export; many tables share the same edges.
from functools import lru_cache
from bisect import bisect_right

@lru_cache(maxsize=256)
def _uniform_axis(edges):
    """Return (n, low, high) only when finite-double bin membership is preserved.

    Evenly spaced decimal edges alone are insufficient: arithmetic indexing
    can round a value just below an edge into the next bin. The arithmetic
    is monotone; testing each edge and its adjacent representable values
    checks the transitions against upper_bound. Keep unsafe axes explicit.
    This matches CorrectionLib's current division-then-multiplication order.
    """
    values=tuple(float(x) for x in edges)
    if len(values)<2 or not all(math.isfinite(x) for x in values):return None
    low,high=values[0],values[-1];n=len(values)-1;span=high-low
    if not math.isfinite(span) or span<=0:return None
    if any(a>=b for a,b in zip(values,values[1:])):return None
    for i,x in enumerate(values):
        expected=low+i*span/n
        if abs(x-expected)>4*max(math.ulp(x),math.ulp(expected)):return None
    for edge in values:
        for x in (math.nextafter(edge,-math.inf),edge,math.nextafter(edge,math.inf)):
            if low<=x<high and int(n*((x-low)/span))!=bisect_right(values,x)-1:return None
    return n,low,high

def binning(name, edges, content, flow="error"):
    edges=[float(e) if not isinstance(e,str) else e for e in edges]
    uniform=None if flow=="wrap" else _uniform_axis(tuple(edges))
    if uniform is not None:
        n,low,high=uniform
        edges=dict(n=n,low=low,high=high)
    return dict(nodetype="binning", input=name, edges=edges, content=list(content), flow=flow)
'''
assert original.count(old)==1
p.write_text(original.replace(old,replacement))
target=win/'columnar-source/python/atlas_correctionlib/nodes.py'
target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
print('Installed conservative uniform-axis export selection')
