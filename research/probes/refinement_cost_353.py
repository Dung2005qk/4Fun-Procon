"""Bounded same-input diagnostic; no production/library edits or full matches."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ID='ATTR-SAME-INPUT-REFINEMENT-COST-353'
ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'artifacts/research/353/validated'
D=ROOT/f'research/evidence/{ID}-validated'
ORDER=('parentA','candidateA','candidateB','parentB')
def load(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def write(p,x):
    p.parent.mkdir(parents=True,exist_ok=True)
    with p.open('x',encoding='utf-8',newline='\n')as f:json.dump(x,f,indent=2,sort_keys=True);f.write('\n')
def require(v,s):
    if not v:raise RuntimeError(s)
def snapshot(doc):
    return {'config':doc['config'],'state':doc['state'],'ledger':doc['ledger'],
            'plan':doc['decision']['candidate']['plan']}
def comparable(q):
    q=json.loads(json.dumps(q));q['state'].pop('endsAt',None);return q
def prepare():
    sources={};snaps=[]
    case_input=ROOT/'artifacts/research/352/package/input352.json'
    require(sha(case_input)=='F87841F53A693CAD141DC0B434201F8145CAF6180A0A897954C61A7E7F6D15C4','352 fixture identity')
    case=next(c for c in load(case_input)['cases'] if c['seed']==2026090835090164)
    sources[case_input.relative_to(ROOT).as_posix()]=sha(case_input)
    for label in ORDER:
        p=ROOT/f'research/evidence/ATTR-PORTABLE-RUNTIME-LOSS-REPEAT-352/{label[-1]}/{label[:-1]}/2026090835090164.replay.jsonl'
        sources[p.relative_to(ROOT).as_posix()]=sha(p)
        doc=next(json.loads(line)['body']for line in p.read_text().splitlines()if json.loads(line)['kind']=='decision')
        require(doc['config']=={'agentCount':len(case['setup']['agents']),
                'brandCount':6,'cellCount':1024,'dayCount':4},'audit/fixture counts')
        q=snapshot(doc);q['config']=case['setup'];snaps.append(q)
    require(all(comparable(q)==comparable(snaps[0])for q in snaps),'day1 semantic inputs/plans differ')
    # Preserve the exact chosen snapshot. Only differing wall timestamps are
    # excluded from equality; the refiner uses the supplied steady deadline.
    write(WORK/'snapshot.json',snaps[0])
    files=['research/probes/refinement_cost_353.cpp','research/probes/refinement_cost_353.py',
           f'research/evidence/{ID}-preregistration.md','artifacts/research/353/validated/snapshot.json']
    write(WORK/'input.json',{'experiment':ID,'sources':sources,'hashes':{p:sha(ROOT/p)for p in files},
        'budgets':[1593,1654,10000],'order':ORDER,'expected':12,
        'input_equality':'all four config/state/ledger/main-plan equal except wall endsAt; chosen state unchanged',
        'execution352_sha256':'C0FEEAF04B5082C7BC4A95ACD6B88F53B97EBC555A4620ED2AAB28DB2924EB4F'})
    print('prepared353',sha(WORK/'input.json'))
def verify(m):
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'frozen drift '+p)
def vm():
    require(subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829','authorized VM only')
    require(not D.exists() and not (WORK/'execution.json').exists(),'existing run, no blind duplication')
    m=load(WORK/'input.json');verify(m)
    execution352=ROOT/'execution352.json'
    require(sha(execution352)==m['execution352_sha256'],'352 execution identity')
    frozen=load(execution352)['hashes'];hashes=dict(m['hashes'])
    for side in ('parent','candidate'):
        lib=f'build-{side}/libudon_shield.a';require(sha(ROOT/lib)==frozen[lib],'frozen library drift')
        # Verify all original headers and source, not only the archive.
        for p,h in frozen.items():
            if p.startswith(side+'/'):require(sha(ROOT/p)==h,'original source/header drift '+p)
        target=WORK/f'probe-{side}'
        cmd=['g++','-std=c++20','-O3','-DNDEBUG','-pthread','-I',str(ROOT/side/'include'),
             str(ROOT/'research/probes/refinement_cost_353.cpp'),str(ROOT/lib),'-o',str(target)]
        subprocess.run(cmd,check=True)
        hashes[lib]=sha(ROOT/lib);hashes[target.relative_to(ROOT).as_posix()]=sha(target)
    hashes['artifacts/research/353/validated/input.json']=sha(WORK/'input.json')
    write(WORK/'execution.json',{'experiment':ID,'hashes':hashes,
        'compiler':subprocess.check_output(['g++','--version'],text=True),
        'source_execution352':sha(execution352),'budgets':m['budgets'],'order':m['order']})
    e=load(WORK/'execution.json');verify(e);D.mkdir()
    for budget in m['budgets']:
        for label in m['order']:
            q=load(WORK/'snapshot.json');q['budget_ms']=budget
            prefix=D/f'{budget}-{label}'
            p=subprocess.run([str(WORK/f'probe-{label[:-1]}')],input=json.dumps(q),capture_output=True,text=True,timeout=25)
            with prefix.with_suffix('.stdout').open('x')as f:f.write(p.stdout)
            with prefix.with_suffix('.stderr').open('x')as f:f.write(p.stderr)
            require(p.returncode==0 and not p.stderr,'probe failed; raw evidence preserved')
            row=json.loads(p.stdout);require(row.pop('input_echo')==q,'input mutation/echo')
            require(row['ok'] and row['zero_safety_failure'],'safety')
            row.update(label=label,execution_sha256=sha(WORK/'execution.json'),
                       stdout_sha256=sha(prefix.with_suffix('.stdout')),stderr_sha256=sha(prefix.with_suffix('.stderr')))
            write(prefix.with_suffix('.result.json'),row)
            print('atomic_complete',budget,label,flush=True)
    verify(e)
    write(D/'run_complete.json',{'execution_sha256':sha(WORK/'execution.json'),
        'results':{p.name:sha(p)for p in sorted(D.glob('*.result.json'))},'count':12})
    summarize(False)
def summarize(check):
    c=load(D/'run_complete.json');e=load(WORK/'execution.json');m=load(WORK/'input.json')
    require(c['execution_sha256']==sha(WORK/'execution.json'),'execution identity')
    require(c['count']==len(c['results'])==len(list(D.glob('*.result.json')))==12,'counts')
    rows=[]
    for p,h in c['results'].items():
        require(sha(D/p)==h,'atomic hash');r=load(D/p);prefix=D/p.replace('.result.json','')
        require(sha(prefix.with_suffix('.stdout'))==r['stdout_sha256'] and sha(prefix.with_suffix('.stderr'))==r['stderr_sha256'],'companions')
        raw=load(prefix.with_suffix('.stdout'));echo=raw.pop('input_echo');q=load(WORK/'snapshot.json');q['budget_ms']=r['budget_ms']
        require(echo==q and all(r[k]==v for k,v in raw.items()),'raw/atomic consistency')
        require(r['ok'] and r['zero_safety_failure'] and r['base_score']==[6,6,55],'contract')
        require(r['execution_sha256']==c['execution_sha256'],'row execution')
        rows.append(r)
    expected={(b,l)for b in m['budgets']for l in m['order']}
    require({(r['budget_ms'],r['label'])for r in rows}==expected,'registered order coverage')
    report={'experiment':ID,'complete':True,'rows':rows,'zero_safety_failure':True,
            'execution_sha256':c['execution_sha256'],'completion_sha256':sha(D/'run_complete.json'),
            'authority':'same-input phase attribution only; no promotion;350 rejected'}
    out=D.with_suffix('.summary.json')
    if check:require(load(out)==report,'summary drift')
    else:write(out,report)
    print('summary_check'if check else 'summary_complete',sha(out))
    for r in rows:print(r['budget_ms'],r['label'],r['score'],round(r['wait_ms'],2),round(r['midday_ms'],2),r['routes'],r['target_acceptances'],r['deadline'])
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('prepare','vm','check'));a=p.parse_args()
    if a.mode=='prepare':prepare()
    elif a.mode=='vm':vm()
    else:summarize(True)
