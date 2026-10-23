"""Calibration constants must survive JSON loading bit-for-bit."""
import json,random,struct
import correctionlib
def test_decimal_double_roundtrip():
 rng=random.Random(1942)
 values=[rng.uniform(.01,3.) for _ in range(500)]
 payload={"schema_version":2,"corrections":[dict(name=f"v{i}",version=1,inputs=[],output={"name":"value","type":"real"},data=v) for i,v in enumerate(values)]}
 cs=correctionlib.CorrectionSet.from_string(json.dumps(payload))
 mismatches=[(i,v,cs[f"v{i}"].evaluate()) for i,v in enumerate(values) if struct.pack("d",v)!=struct.pack("d",cs[f"v{i}"].evaluate())]
 assert not mismatches,repr(mismatches[:5])
