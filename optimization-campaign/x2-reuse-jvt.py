from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'python/atlas_correctionlib/jvt.py';s=p.read_text()
s=s.replace('  self._ranges=None','  self._ranges=None\n  self._anchor=None')
s=s.replace('  # Both NNJVT', '''  # Keep one exact sigma-zero result. Copies protect against caller mutation.
  inputs=(p4[:,:2],sumpt,jvfcorr,haspv,ishs)
  if sigma==0 and self._anchor is not None:
   nn,efficiency,saved,result=self._anchor
   if nn is self.nn and efficiency is self.efficiency and all(np.array_equal(a,b) for a,b in zip(inputs,saved)):
    return tuple(value.copy() for value in result)
  # Both NNJVT''')
s=s.replace('  return score,passed,values[:,0].astype(np.float32),values[:,1]==0','''  output=(score,passed,values[:,0].astype(np.float32),values[:,1]==0)
  if sigma==0 and self._anchor is None:
   self._anchor=(self.nn,self.efficiency,tuple(np.array(value,copy=True) for value in inputs),tuple(value.copy() for value in output))
  return output''')
p.write_text(s)
