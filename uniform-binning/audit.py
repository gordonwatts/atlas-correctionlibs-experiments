"""Audit actual exported axes before changing their representation."""
from pathlib import Path
import bisect, collections, gzip, json, math
root=Path('/home/gwatts/atlas/correctionlib_tests')
out=root/'optimization-campaign/29-uniform-binning'
out.mkdir(exist_ok=False)
axes=collections.Counter()
def walk(node):
    if isinstance(node,dict):
        if node.get('nodetype')=='binning' and isinstance(node['edges'],list):
            axes[tuple(node['edges'])]+=1
        for value in node.values():walk(value)
    elif isinstance(node,list):
        for value in node:walk(value)
for filename in ['nominal.json.gz','uncertainty-tables.json.gz']:
    with gzip.open(root/'payloads'/filename,'rt') as f:walk(json.load(f))
rows=[]
for axis,count in axes.items():
    edges=[float(x) for x in axis];n=len(edges)-1
    reason='nonfinite'
    if all(math.isfinite(x) for x in edges):
        low,high=edges[0],edges[-1];width=high-low
        if not (n>0 and math.isfinite(width) and width>0):reason='invalid'
        elif not all(abs(x-(low+i*width/n))<=4*max(math.ulp(x),math.ulp(low+i*width/n)) for i,x in enumerate(edges)):reason='nonuniform'
        else:
            reason='safe'
            for edge in edges:
                for x in [math.nextafter(edge,-math.inf),edge,math.nextafter(edge,math.inf)]:
                    if x<low or x>=high:continue
                    if int(n*((x-low)/width))!=bisect.bisect_right(edges,x)-1:
                        reason='boundary-rounding';break
                if reason!='safe':break
    rows.append(dict(edges=axis,occurrences=count,reason=reason))
summary={reason:dict(axes=sum(r['reason']==reason for r in rows),nodes=sum(r['occurrences'] for r in rows if r['reason']==reason)) for reason in sorted({r['reason'] for r in rows})}
(out/'axis-audit.json').write_text(json.dumps(dict(summary=summary,axes=rows),indent=2)+'\n')
print(json.dumps(summary,indent=2),flush=True)
