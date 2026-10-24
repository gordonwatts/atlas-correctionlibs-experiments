from pathlib import Path
import copy,gzip,hashlib,importlib.util,json,math,shutil
import numpy as np
import correctionlib
root=Path('/home/gwatts/atlas/correctionlib_tests')
out=root/'optimization-campaign/29-uniform-binning'
spec=importlib.util.spec_from_file_location('export_nodes',root/'python/atlas_correctionlib/nodes.py')
nodes=importlib.util.module_from_spec(spec);spec.loader.exec_module(nodes)
audit=json.loads((out/'axis-audit.json').read_text())
checks=[]
rng=np.random.default_rng(2901)
for i,row in enumerate(audit['axes']):
    edges=row['edges'];uniform=nodes._uniform_axis(tuple(edges))
    assert (uniform is not None)==(row['reason']=='safe')
    if uniform is None:continue
    n,low,high=uniform
    points=[x for e in edges for x in [math.nextafter(e,-math.inf),float(e),math.nextafter(e,math.inf)]]
    points += [low-(high-low),high+(high-low),-math.inf,math.inf]
    points += list(rng.uniform(low,high,100000))
    p=np.array(points)
    for flow in ('clamp',-123.):
        def correction(name,edge_spec):
            return dict(name=name,version=1,inputs=[dict(name='x',type='real')],output=dict(name='bin',type='real'),data=dict(nodetype='binning',input='x',edges=edge_spec,content=list(map(float,range(n))),flow=flow))
        payload=dict(schema_version=2,corrections=[correction('old',edges),correction('new',dict(n=n,low=low,high=high))])
        cs=correctionlib.CorrectionSet.from_string(json.dumps(payload))
        a,b=cs['old'].evaluate(p),cs['new'].evaluate(p)
        assert np.array_equal(a,b),(edges,flow)
        checks.append(dict(axis=i,flow=flow,points=len(p),exact=True))
    # Error flow must preserve in-range indices and reject the same exterior points.
    payload=dict(schema_version=2,corrections=[correction('old',edges),correction('new',dict(n=n,low=low,high=high))])
    for c in payload['corrections']:c['data']['flow']='error'
    cs=correctionlib.CorrectionSet.from_string(json.dumps(payload))
    for x in [low,math.nextafter(high,low),high,math.nextafter(low,-math.inf)]:
        answers=[]
        for name in ('old','new'):
            try:answers.append(cs[name].evaluate(x))
            except RuntimeError:answers.append('error')
        assert answers[0]==answers[1]
assert nodes.binning('x',[0,1,2],[0,1],'wrap')['edges']==[0.,1.,2.]
assert nodes._uniform_axis((-math.inf,0,math.inf)) is None
assert nodes._uniform_axis((0.,1.,3.)) is None
assert nodes._uniform_axis((-4.4,0.,4.4)) is None
report=dict(axis_checks=checks,changes=[],payloads={})
backup=out/'original-payloads';backup.mkdir(exist_ok=False)
for name in ('nominal.json.gz','uncertainty-tables.json.gz'):
    path=root/'payloads'/name
    with gzip.open(path,'rt') as f:payload=json.load(f)
    changed=[]
    def walk(node,path):
        if isinstance(node,dict):
            if node.get('nodetype')=='binning' and isinstance(node['edges'],list):
                uniform=None if node.get('flow')=='wrap' else nodes._uniform_axis(tuple(node['edges']))
                if uniform is not None:
                    n,low,high=uniform
                    changed.append(dict(path=path,old_edges=node['edges'],new_edges=dict(n=n,low=low,high=high)))
                    node['edges']=dict(n=n,low=low,high=high)
            for k,v in node.items():walk(v,path+'/'+str(k))
        elif isinstance(node,list):
            for i,v in enumerate(node):walk(v,path+'/'+str(i))
    old_hash=hashlib.sha256(path.read_bytes()).hexdigest()
    walk(payload,'')
    if changed:
        shutil.copy2(path,backup/name)
        candidate=out/name
        with gzip.open(candidate,'wt') as f:json.dump(payload,f)
        correctionlib.CorrectionSet.from_file(str(candidate))
        shutil.copy2(candidate,path)
    report['payloads'][name]=dict(before_sha256=old_hash,after_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),converted_nodes=len(changed))
    for entry in changed:
        entry['payload']=name
        index=int(entry['path'].split('/')[2]);entry['correction']=payload['corrections'][index]['name']
    report['changes']+=changed
# The existing bundle manifest must describe the updated compressed artifacts.
path=root/'payloads/bundle-manifest.json';shutil.copy2(path,backup/path.name)
manifest=json.loads(path.read_text())
for name in report['payloads']:
    p=root/'payloads'/name
    manifest['files'][name].update(sha256=hashlib.sha256(p.read_bytes()).hexdigest(),size=p.stat().st_size)
path.write_text(json.dumps(manifest,indent=2))
(out/'conversion.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
