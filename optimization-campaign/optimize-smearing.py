from pathlib import Path
import sys
root=Path('/home/gwatts/atlas/correctionlib_tests')
p=root/'src/backend.hxx'
s=p.read_text()
if sys.argv[1]=='construction':
    old=' TRandom3 random;random.SetSeed(seed);'
    assert old in s
    s=s.replace(old,' TRandom3 random(seed);')
elif sys.argv[1]=='cache':
    old=' TRandom3 random(seed);\n double value=-1;while(value<0)value=random.Gaus(1.,width);return value;'
    assert old in s
    new=''' // The same phi seed is reused across resolution states. Cache only the
 // first standard-normal draw, preserving ROOT's stream and width-dependent
 // rejection. Direct-mapped storage bounds memory; a full seed tag handles
 // collisions and fallback seeds. Each thread owns its cache.
 struct Entry {long long seed=0;double gaussian=0;};
 static thread_local std::vector<Entry> cache(1u<<20);
 auto& entry=cache[static_cast<unsigned long long>(seed)&(cache.size()-1)];
 if(entry.seed!=seed){
  TRandom3 random(seed);entry.gaussian=random.Gaus(0.,1.);entry.seed=seed;
 }
 double value=1.+width*entry.gaussian;
 if(value>=0)return value;
 // Recreate the original stream when its first draw is rejected. Rejection
 // depends on width, so caching an accepted draw would change correlations.
 TRandom3 random(seed);
 value=-1;while(value<0)value=random.Gaus(1.,width);return value;'''
    s=s.replace(old,new)
else:
    raise ValueError(sys.argv[1])
p.write_text(s)
