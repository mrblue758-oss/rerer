import collections,sys
sys.path.insert(0,'.')
def parse_objs(txt):
    out=[]
    for o in txt.replace('\n','').split(';'):
        if not o: continue
        kv=o.split(','); out.append(dict(zip(kv[0::2],kv[1::2])))
    return out
orig=parse_objs(open('nivel_de_prueba_sin_deco.gmd.txt').read().split(';',1)[1])
gen=parse_objs(open('out1.txt').read())
cell=lambda o:(int(float(o['2'])//30),int(float(o['3'])//30))
occ=collections.defaultdict(list)
for o in orig: occ[cell(o)].append(o['1'])
# 1) overlap check
bad=collections.Counter(); selfstack=0
for o in gen:
    if o['1']=='899': continue
    c=cell(o)
    if c in occ:
        if any(g in('1338','326','327','328','329','1339') for g in occ[c]): selfstack+=1
        else: bad[(o['1'],tuple(occ[c]))]+=1
print('generated',len(gen),'| same-cell-as-gameplay (expected recipe/self stacks):',selfstack,'| UNEXPECTED overlaps with non-gameplay-recipe objs:',sum(bad.values()),dict(list(bad.items())[:5]))
# 2) duplicates in same cell+id
dup=collections.Counter((o['1'],o['2'],o['3']) for o in gen)
print('duplicate piece placements:',sum(1 for v in dup.values() if v>1))
# 3) every object has marker group & valid keys
print('all have marker 9999:',all(o.get('57')=='9999' for o in gen))
print('ids:',collections.Counter(o['1'] for o in gen).most_common())
# 4) trigger sanity
trg=[o for o in gen if o['1']=='899']
print('triggers',len(trg),'x range',min(float(o['2']) for o in trg),max(float(o['2']) for o in trg),'y range',min(float(o['3']) for o in trg),max(float(o['3']) for o in trg))
print('channels targeted',collections.Counter(o['23'] for o in trg))
# 5) channel usage in deco
print('deco channel mix',collections.Counter(o['21'] for o in gen if o['1']!='899'))
# 6) min y
print('min deco y',min(float(o['3']) for o in gen if o['1']!='899'))
