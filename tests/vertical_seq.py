import re,sys
from collections import defaultdict
L=open(sys.argv[1]).read().split('\n')
frames=[]; cur=None; desc=-1
for l in L:
    if l.startswith('[vseq] descida'): desc+=1
    m=re.match(r'\[cartao\] --- q(\d+)',l)
    if m: cur={'d':desc,'cards':[]}; frames.append(cur); continue
    m=re.match(r'\[cartao\] ([\d.-]+) ([\d.-]+) ([\d.-]+) ([\d.-]+) foco=(\d)',l)
    if m and cur is not None: cur['cards'].append(tuple(float(x) for x in m.groups()))
# per descent: track focused card y and the y of each card row (group by approx y band) across frames
for d in sorted(set(f['d'] for f in frames)):
    fs=[f for f in frames if f['d']==d]
    ys=[]; 
    for f in fs:
        fo=[c for c in f['cards'] if c[4]==1]
        ys.append(fo[0][1] if fo else None)
    # top row y (min y among cards) per frame
    tops=[min((c[1] for c in f['cards']),default=None) for f in fs]
    def steps(v):
        v=[x for x in v if x is not None]
        return [round(b-a,1) for a,b in zip(v,v[1:])]
    s=steps(tops)
    big=[x for x in s if abs(x)>60]
    # detect reversal (non-monotonic) beyond 2px
    sign=[1 if x>2 else -1 if x<-2 else 0 for x in s]
    rev=sum(1 for a,b in zip(sign,sign[1:]) if a*b<0)
    print('descida',d,'quadros',len(fs),'passos topo:',s[:30])
    print('   maior passo %.1f, inversoes %d, foco y: %s'%(max((abs(x) for x in s),default=0),rev,[round(y,1) if y is not None else None for y in ys[:12]]))
