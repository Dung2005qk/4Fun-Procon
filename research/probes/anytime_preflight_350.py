"""Complete feasible incumbent retention under the unchanged resource budgets."""
import argparse
from collections import Counter
from pathlib import Path
import subprocess
import sys
from run_http_baseline_314 import ROOT,Bridge,digest
from terminal_quotient_work_339 import load,write_new,verify,require,validate as validate_control

ID='SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350'
M=ROOT/f'research/holdouts/{ID}.json'
E=ROOT/f'research/holdouts/{ID}-preflight-execution.json'
D=ROOT/f'research/evidence/{ID}-budget-preflight'
S=ROOT/f'research/evidence/{ID}-budget-preflight.summary.json'
INPUT=ROOT/'research/holdouts/ATTR-PAIR-TERMINAL-QUOTIENT-WORK-339.json'
CAPS=tuple(2**n for n in range(10,19))
LIMITS={'settled':32768,'created':196609,'actions':262144,'memo':8192,'transitions':262144,'queries':1024}

def freeze():
    require(digest(M)=='3ABB3D35C5FCFDD49B66680BD563FBFCD70D228BE6F0821948EBD8D5F403E754','fresh input drift')
    m=load(M);verify(m)
    require(digest(INPUT)=='5B288535C21B0907519730205EA7A03598E1B9DAD0DF806523FD54BA88DFAA90','DEV input drift')
    for s in m['splits'].values():require(digest(ROOT/s['path'])==s['sha256'],'split drift')
    commands=[[sys.executable,'research/probes/test_anytime_350.py']]
    cmake=next(l.split('=',1)[1] for l in (ROOT/'build-release/CMakeCache.txt').read_text().splitlines() if l.startswith('CMAKE_COMMAND:INTERNAL='))
    commands.append([str(Path(cmake).with_name('ctest.exe')),'--test-dir','artifacts/research/350/build','--output-on-failure'])
    records=[]
    for c in commands:
        r=subprocess.run(c,cwd=ROOT,text=True,capture_output=True)
        records.append({'command':c,'code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        require(r.returncode==0,r.stdout+r.stderr)
    src=ROOT/'artifacts/research/350/source'
    paths=set(m['hashes'])|{str(M.relative_to(ROOT)),str(INPUT.relative_to(ROOT)),
        'research/probes/anytime_preflight_350.py','research/probes/test_anytime_350.py',
        'research/probes/build_anytime_350.cmd','artifacts/research/350/probe.cpp',
        'artifacts/research/350/probe.exe','artifacts/research/350/build/udonshield_btc.exe',
        'artifacts/research/350/build/udon_shield.lib',
        'artifacts/research/347/build/udonshield_pricing_contract_344.exe',
        'research/probes/terminal_quotient_work_339.py'}
    paths|={str(p.relative_to(ROOT)) for f in ('src','include','strategies','tests') for p in (src/f).rglob('*') if p.suffix in ('.hpp','.cpp','.inc')}
    write_new(E,{'experiment':ID,'hashes':{p:digest(ROOT/p) for p in sorted(paths)},'tests':records})
    print('preflight_execution_sha256='+digest(E),flush=True)

def validate(c,a,judge,cap):
    q=c['context']['request'];require(a.get('ok') and a['originalUnchanged'] and a['failures']==0,'probe safety')
    require(a['baseline']==c['context']['baseline'] and a['supported']==1,'root identity')
    require(a['completed']+a['exhausted']==1,'search completeness')
    require(0<=a['anytimeReturns']<=a['exhausted'] and a['improvements']<=a['completed']+a['anytimeReturns'],'anytime authority')
    require(a['certifiedIncumbents']>=a['improvements'],'missing certificate')
    for k,n in LIMITS.items():require(0<=a[k]<=min(n,cap) if k=='transitions' else 0<=a[k]<=n,'cap exceeded:'+k)
    require(tuple(a['score'])>=tuple(a['baseline']),'original regression')
    if a['anytimeReturns']:require(a['improvements']==1 and tuple(a['score'])>tuple(a['baseline']),'non-strict publication')
    if not a['improvements']:require(a['plans']==q['plans'] and a['score']==a['baseline'],'unpublished mutation')
    require(len(a['plans'])==len(q['plans'])==3,'full suffix')
    fixed=[i for i in range(3) if all(p[i]==o[i] for p,o in zip(a['plans'],q['plans'],strict=True))]
    require(fixed,'physical pair identity')
    state,ledger=q['state'],q['ledger']
    for i,p in enumerate(a['plans']):
        r=judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':p})
        require(r.get('ok') and r['agrees'],'dual validation')
        require(all(r['agents'][j]==c['context']['original_days'][i]['agents'][j] for j in fixed),'fixed state drift')
        state={**state,'day':state['day']+1,'agents':r['agents']};ledger=r['ledger']
    require(r['score']==a['score'],'full-ledger identity')

def run():
    e=load(E);verify(e);cases=load(INPUT)['cases'];require(len(cases)==24,'ALL DEV roots')
    D.mkdir(exist_ok=False)
    p=Bridge(ROOT/'artifacts/research/350/probe.exe');b=Bridge(ROOT/'artifacts/research/347/build/udonshield_pricing_contract_344.exe');judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
    try:
        for c in cases:
            rows=[]
            for cap in CAPS:
                q={**c['context']['request'],'op':'price','expired':False,'corrupt':False,'limit':'transitions','value':cap}
                a=p.request(q);validate(c,a,judge,cap)
                ref=b.request(q);validate_control(c,ref,judge)
                if a['completed'] and ref['completed']:require(a['score']==ref['score'] and a['plans']==ref['plans'],'complete parity')
                rows.append({'cap':cap,'candidate':a,'control':ref})
            write_new(D/(str(c['spec']['seed'])+'.result.json'),{'spec':c['spec'],'outputs':rows})
            print('case_complete seed='+str(c['spec']['seed']),flush=True)
    finally:
        for x in (p,b,judge):x.close();x.process.stdout.close();x.process.stderr.close()
    verify(e)
    write_new(D/'run_complete.json',{'cases':24,'queries':216,'controls':216,'execution_sha256':digest(E),
        'hashes':{p.name:digest(p) for p in sorted(D.glob('*.result.json'))}})
    summarize()

def summarize():
    e=load(E);verify(e);done=load(D/'run_complete.json')
    require(done['execution_sha256']==digest(E) and done['cases']==24 and len(done['hashes'])==24,'completion identity')
    require(done['hashes']=={p.name:digest(p) for p in sorted(D.glob('*.result.json'))},'result drift')
    counts=Counter();families=Counter();roots=[];rows=[];judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
    try:
        for c in load(INPUT)['cases']:
            r=load(D/(str(c['spec']['seed'])+'.result.json'));require(r['spec']==c['spec'],'case identity')
            require([x['cap'] for x in r['outputs']]==list(CAPS),'grid drift');hits=0
            for x in r['outputs']:
                a,b=x['candidate'],x['control'];validate(c,a,judge,x['cap']);validate_control(c,b,judge)
                if a['completed'] and b['completed']:require(a['score']==b['score'] and a['plans']==b['plans'],'complete parity')
                counts.update({k:a[k] for k in ('completed','exhausted','anytimeReturns','certifiedIncumbents','improvements',*LIMITS)})
                counts.update({'queries_total':1,'control_complete':b['completed'],'control_complete_candidate_exhausted':b['completed'] and not a['completed']})
                hits+=a['anytimeReturns']
            if hits:roots.append(c['spec']);families[c['spec']['family']]+=1
            rows.append(r)
    finally:judge.close();judge.process.stdout.close();judge.process.stderr.close()
    require(counts['queries_total']==216,'query total')
    gate=len(roots)>=4 and len(families)>=2
    report={'experiment':ID,'complete':True,'gate_passed':gate,'counts':counts,'roots':roots,'families':dict(families),
        'zero_safety_failure':True,'rows':rows,'execution_sha256':digest(E),'completion_sha256':digest(D/'run_complete.json'),
        'authority':'Old DEVELOPMENT budget-retention capability only; no applied score or promotion claim'}
    write_new(S,report);print({k:v for k,v in report.items() if k not in ('rows','roots')},flush=True);print('summary_sha256='+digest(S))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','run','summarize'))
    {'freeze':freeze,'run':run,'summarize':summarize}[p.parse_args().mode]()
