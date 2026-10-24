from pathlib import Path
import numpy as np,json,re
r=Path('/home/gwatts/atlas/correctionlib_tests');base=r/'optimization-campaign/24-calculation-profile'
def read(name):
 batches=[]
 with (base/f'runs/C-1/results/{name}.bin').open('rb') as f:
  while True:
   h=np.fromfile(f,dtype=np.int64,count=3)
   if not len(h):break
   n,width,sigma=h;batches.append((np.fromfile(f,dtype=np.float64,count=n*width).reshape(n,width),int(sigma)))
 return batches
batches=read('nn');x=np.stack([b[0] for b in batches]);cuts=json.loads((r/'payloads/nnjvt-cuts.json').read_text())
pte=np.asarray(cuts['ptbin_edges'],dtype=np.float32)*np.float32(1000);etae=np.asarray(cuts['etabin_edges'],dtype=np.float32)
ip=np.searchsorted(pte,x[:,:,0].astype(np.float32),side='left');ip=np.where(ip==0,1,ip);ie=np.searchsorted(etae,x[:,:,1].astype(np.float32),side='left')
active=(x[:,:,5]!=0)&(x[:,:,0]<=60000)&(ip<len(pte))&(ie>0)&(ie<len(etae))
rpt=np.divide(x[:,:,3],x[:,:,0],out=np.zeros_like(x[:,:,0]),where=x[:,:,0]!=0).astype(np.float32)
features=np.stack([rpt,x[:,:,4].astype(np.float32),(ip-1).astype(np.float32),(ie-1).astype(np.float32)],axis=2).view(np.uint32)
unique=sum(len(np.unique(features[:,j][active[:,j]],axis=0)) for j in range(x.shape[1]))
nominal_matches=int(np.sum(active[1:]&active[0]&np.all(features[1:]==features[0],axis=2)))
previous_matches=int(np.sum(active[1:]&active[:-1]&np.all(features[1:]==features[:-1],axis=2)))
report={'nn_batches':len(batches),'jets_per_batch':x.shape[1],'nn_row_evaluations':int(active.size),'network_inferences':int(active.sum()),'distinct_features_per_jet_summed':unique,'repeated_features_within_jet':int(active.sum())-unique,'active_matches_first_batch':nominal_matches,'active_matches_previous_batch':previous_matches,'feature_key':'bitwise float32 Rpt, JVFCorr, pt-bin, eta-bin; only active inference rows','sample_only':True}
for mode in ('nn','efficiency','systematics'):
 text=(base/f'{mode}-no.txt').read_text();total=int(re.search(r'([\d,]+) \(100.0%\)  PROGRAM TOTALS',text).group(1).replace(',',''));values=[]
 for line in text.splitlines():
  m=re.match(r'\s*([\d,]+)\s+\([^)]*\)\s+(.*)',line)
  if m:values.append((int(m.group(1).replace(',','')),m.group(2)))
 alloc=sum(n for n,name in values if any('???:'+fn+' [' in name for fn in ('malloc','free','_int_free')))
 matvec=sum(n for n,name in values if 'general_matrix_vector_product<' in name)
 report[mode]={'total_instructions':total,'malloc_free_int_free_exclusive':alloc,'allocation_percent':100*alloc/total,'matrix_vector_kernel_exclusive':matvec,'matrix_vector_percent':100*matvec/total}
network=json.loads((r/'payloads/nnjvt-network.json').read_text())
report['network_layers']=[{'architecture':layer.get('architecture'),'activation':layer.get('activation'),'weights':len(layer.get('weights',[])),'bias':len(layer.get('bias',[]))} for layer in network.get('layers',[])]
(base/'analysis.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
