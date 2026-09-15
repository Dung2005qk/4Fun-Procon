"""Development-only certificate lifetime attribution. Never sends actions."""
import argparse
import copy
import json
from collections import Counter
from pathlib import Path
from run_http_baseline_314 import ROOT, Bridge, digest
from terminal_quotient_work_339 import load, require, write_new, verify, validate

ID = 'ATTR-COMPLETED-PAIR-CERTIFICATE-RETENTION-348'
M = ROOT / f'research/holdouts/{ID}.json'
E = ROOT / f'research/holdouts/{ID}-execution.json'
D = ROOT / f'research/evidence/{ID}'
S = ROOT / f'research/evidence/{ID}.summary.json'
CAPS = (262144, 65536, 131072, 196608)


def freeze_inputs():
    p = ROOT / 'research/holdouts/ATTR-PAIR-TERMINAL-QUOTIENT-WORK-339.json'
    require(digest(p) == '5B288535C21B0907519730205EA7A03598E1B9DAD0DF806523FD54BA88DFAA90', 'input drift')
    m = load(p); verify(m)
    require(len(m['cases']) == 24, 'all roots required')
    paths = set(m['hashes']) | {str(p.relative_to(ROOT)),
        'artifacts/research/347/source/src/horizon_pricing.cpp',
        'artifacts/research/347/build/udon_shield.lib',
        'artifacts/research/340/probe.exe', 'artifacts/research/314/bridge.exe',
        'research/probes/bounded_pair_probe_340.cpp',
        'research/probes/completed_pair_retention_348.py',
        f'research/evidence/{ID}-preregistration.md'}
    paths.update(str(x.relative_to(ROOT)) for x in (ROOT/'artifacts/research/347/source/include').rglob('*.hpp'))
    write_new(M, {'experiment': ID, 'cases': m['cases'], 'transition_caps': CAPS,
        'hashes': {p:digest(ROOT/p) for p in sorted(paths)},
        'authority': 'ALL24 old DEVELOPMENT roots; no holdout or played-score authority'})
    print('input_sha256='+digest(M), flush=True)


def freeze_execution():
    m=load(M); verify(m)
    paths=set(m['hashes'])|{str(M.relative_to(ROOT)),
        'artifacts/research/348/horizon_pricing.cpp','artifacts/research/348/probe.cpp',
        'artifacts/research/348/probe.exe','research/probes/build_retention_348.cmd',
        'research/probes/test_completed_pair_retention_348.py'}
    write_new(E, {'experiment':ID, 'input':str(M.relative_to(ROOT)),
        'hashes':{p:digest(ROOT/p) for p in sorted(paths)}})
    print('execution_sha256='+digest(E), flush=True)


def validate_observed(c, out, judge):
    validate(c,out,judge)
    obs=out['observation']
    require(0<=obs['completed_pairs']<=3, 'pair count')
    if obs['score'] is None:
        require(not obs['plans'], 'unscored plans')
        return False
    require(out['exhausted']==1 and out['completed']==0 and out['failures']==0, 'wrong observation authority')
    require(obs['completed_pairs']>0 and len(set(obs['pair']))==2, 'incomplete pair')
    require(tuple(obs['score'])>tuple(out['baseline']), 'not strict')
    check=copy.deepcopy(out)
    check.update(completed=1,exhausted=0,score=obs['score'],plans=obs['plans'])
    validate(c,check,judge)
    q=c['context']['request']; fixed=set(range(3))-set(obs['pair'])
    require(all(obs['plans'][d][a]==q['plans'][d][a] for d in range(3) for a in fixed), 'fixed physical identity')
    require(obs['checkpoint_transitions']<=out['transitions'], 'checkpoint beyond actual work')
    return True


def run():
    e=load(E);verify(e);m=load(M);verify(m)
    require(not D.exists(), 'do not duplicate completed attribution')
    D.mkdir()
    probe=Bridge(ROOT/'artifacts/research/348/probe.exe')
    judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
    control=Bridge(ROOT/'artifacts/research/340/probe.exe')
    try:
        for c in m['cases']:
            outputs=[]
            q={**c['context']['request'],'op':'price','expired':False,'corrupt':False}
            for cap in CAPS:
                request={**q,'limit':'transitions','value':cap}
                out=probe.request(request)
                discarded=validate_observed(c,out,judge)
                ref=control.request(request);validate(c,ref,judge)
                if out['completed'] and ref['completed']:
                    require(out['score']==ref['score'] and out['plans']==ref['plans'], 'complete-return drift')
                outputs.append({'cap':cap,'output':out,'control':ref,'discarded_strict':discarded})
            write_new(D/(str(c['spec']['seed'])+'.result.json'),{'spec':c['spec'],'outputs':outputs})
            print('case_complete seed='+str(c['spec']['seed']),flush=True)
    finally:
        for p in (probe,judge,control):p.close();p.process.stdout.close();p.process.stderr.close()
    verify(e)
    write_new(D/'run_complete.json',{'cases':24,'queries':96,'control_queries':96,
        'execution_sha256':digest(E),'hashes':{p.name:digest(p) for p in sorted(D.glob('*.result.json'))}})
    summarize()


def summarize():
    e=load(E);verify(e);m=load(M);done=load(D/'run_complete.json')
    require(done['execution_sha256']==digest(E) and done['cases']==24, 'completion identity')
    require(done['hashes']=={p.name:digest(p) for p in sorted(D.glob('*.result.json'))} and len(done['hashes'])==24,'result hashes')
    rows=[]; roots=[]; families=Counter();counts=Counter();judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
    try:
        for c in m['cases']:
            r=load(D/(str(c['spec']['seed'])+'.result.json'));require(r['spec']==c['spec'],'identity')
            require([x['cap'] for x in r['outputs']]==list(CAPS),'budget grid')
            hits=0
            for x in r['outputs']:
                lost=validate_observed(c,x['output'],judge); require(lost==x['discarded_strict'],'verdict drift')
                counts.update({'queries':1,'discarded_strict':lost,'completed':x['output']['completed'],'exhausted':x['output']['exhausted']})
                hits+=lost
            if hits:roots.append(c['spec']);families[c['spec']['family']]+=1
            rows.append(r)
    finally:judge.close();judge.process.stdout.close();judge.process.stderr.close()
    require(counts['queries']==96,'total queries')
    passed=len(roots)>=4 and len(families)>=2
    report={'experiment':ID,'complete':True,'gate_passed':passed,'counts':counts,'roots':roots,
        'families':dict(families),'all_rows':rows,'zero_safety_failure':True,
        'execution_sha256':digest(E),'completion_sha256':digest(D/'run_complete.json'),
        'authority':'Development capability attribution only, no applied plan or promotion'}
    write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k!='all_rows'},indent=2));print('summary_sha256='+digest(S))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze-inputs','freeze-execution','run','summarize'))
    {'freeze-inputs':freeze_inputs,'freeze-execution':freeze_execution,'run':run,'summarize':summarize}[p.parse_args().mode]()
