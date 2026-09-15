"""Replicated, case-atomic actual WinHTTP score screen. Complete phases only."""
import argparse
from collections import Counter, defaultdict
import math
from pathlib import Path
import time
from run_pair_score_341 import (ROOT, load, digest, write_new, require, verify,
    decisions, pricing_safety, measured_day, validate_side as small_validate)
from run_http_baseline_314 import Bridge, run_case as small_run
from run_pair_protected_341 import validate_side as protected_validate, STRATA
from protected_http_transport_341 import run_case as protected_run, operational, digest_case
from summarize_http_baseline_314 import compare, safety
from inactive_runtime_aa_342 import LABELS, SUFFIXES, compare_runs
from freeze_complete_pool_344 import order_for
from anytime_diagnostics_350 import install
install()
ID='SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350'

M=ROOT/f'research/holdouts/{ID}-execution.json'


def seed(c):return c.get('seed',c.get('spec',{}).get('seed'))


def location(c,label,directory):
    repeat,side=LABELS[label]
    return directory/repeat/side/str(seed(c))


def check_causal(day):
    a=day['audit']['completePoolPricing']
    require(all(type(a[k]) is bool and a[k] for k in
        ('currentPlansUnchanged','noGainPoolUnchanged','noGainSelectionUnchanged')),'same-invocation mutation')
    require(type(a['entered']) is bool and type(a['replacements']) is int and a['replacements']>=0,'causal audit schema')
    if not a['entered']:
        require(a['replacements']==0 and a['outcomes']==[] and a['originalPoolSize']==0,'unexpected bypass evidence')
        return a
    require(a['originalPoolSize']>0 and a['baseSelectedId'] and
        a['finalSelectedId']==day['candidate']['stableId'],'same-invocation selection identity')
    require(a['replacements']==sum(o['replaced'] for o in a['outcomes']),'replacement count')
    if not a['replacements']:require(a['baseSelectedId']==a['finalSelectedId'],'no-gain selection drift')
    keys=set()
    for o in a['outcomes']:
        key=(o['candidateId'],o['scenarioIndex']);require(key not in keys,'duplicate outcome audit');keys.add(key)
        before=score_tuple(o['beforeScore']);after=score_tuple(o['afterScore'])
        if o['replaced']:
            require(after>before and o['beforeWitness']!=o['afterWitness'],'uncertified/non-strict replacement')
        else:require(before==after and o['beforeWitness']==o['afterWitness'],'no-gain witness mutation')
    require(len({k[0] for k in keys})==a['originalPoolSize'],'missing original candidate outcome')
    return a


def score_tuple(s):
    return tuple(s[k] for k in ('lifetimeDistinct','totalDailyDistinct','totalServings'))


def validate(c,label,directory,phase):
    repeat,side=LABELS[label];base=directory/repeat;p=location(c,label,directory)
    row=(protected_validate if phase in ('protected','broad-development') else small_validate)(c,side,base)
    if side=='candidate':
        pricing_safety(p.with_suffix('.replay.jsonl'))
        for d in decisions(p.with_suffix('.replay.jsonl')):check_causal(d)
    return row


def marker(c,directory):
    return {'seed':seed(c),'case_sha256':digest_case(c),'order':c['run_order'],
        'results':{label:digest(location(c,label,directory).with_suffix('.result.json')) for label in LABELS}}


def audit(cases,directory,phase):
    allowed={'A','B','run_complete.json'}|{str(seed(c))+'.fixture_complete.json' for c in cases}
    require(all(p.name in allowed for p in directory.iterdir()),'foreign root evidence')
    for r in ('A','B'):
        require({p.name for p in (directory/r).iterdir()}=={'parent','candidate'},'foreign repeat evidence')
        for side in ('parent','candidate'):
            expected={str(seed(c))+s for c in cases for s in SUFFIXES}
            require(all(p.name in expected for p in (directory/r/side).iterdir()),'foreign side evidence')
    for c in cases:
        for label in LABELS:
            p=location(c,label,directory)
            if p.with_suffix('.result.json').exists():validate(c,label,directory,phase)
            else:require(not list(p.parent.glob(p.name+'.*')),'ambiguous partial side; no blind resume')
        p=directory/(str(seed(c))+'.fixture_complete.json')
        if p.exists():require(load(p)==marker(c,directory),'fixture marker drift')


def coverage(cases,phase):
    n={'development':24,'broad-development':36,'holdout':54,'protected':108}[phase]
    require(len(cases)==len({seed(c) for c in cases})==n,'fixture count/identity')
    require(all(c['run_order']==order_for(i) for i,c in enumerate(cases)),'run order drift')
    return {'fixtures':n,'results':4*n,'paired_comparisons':2*n,
        'actions':4*sum(len(c['setup']['daySteps']) for c in cases),
        'transitions':4*sum(len(c['setup']['daySteps'])-1 for c in cases)}


def authorize(m,phase):
    if phase!='development':
        dev=load(ROOT/f'research/evidence/{ID}-development.summary.json')
        require(dev['execution_sha256']==digest(M) and dev['gate']['passed'],'development did not qualify')
    if phase in ('holdout','protected'):
        broad=load(ROOT/f'research/evidence/{ID}-broad-development.summary.json')
        require(broad['execution_sha256']==digest(M) and broad['gate']['passed'],'broad development did not qualify')
    if phase=='protected':
        held=load(ROOT/f'research/evidence/{ID}-holdout.summary.json')
        require(held['execution_sha256']==digest(M) and held['gate']['passed'],'sealed phase did not qualify')


def run(phase,resume=False):
    m=load(M);verify(m);authorize(m,phase)
    cases=load(ROOT/m['splits'][phase]['path'])['cases'];counts=coverage(cases,phase)
    directory=ROOT/f'research/evidence/{ID}-{phase}'
    if directory.exists():require(resume and not (directory/'run_complete.json').exists(),'existing/completed run')
    else:
        directory.mkdir()
        for r,side in LABELS.values():(directory/r/side).mkdir(parents=True)
    audit(cases,directory,phase);bridge=Bridge(ROOT/m['bridge_binary'])
    try:
        for i,c in enumerate(cases):
            for label in c['run_order']:
                p=location(c,label,directory)
                if p.with_suffix('.result.json').exists():continue
                repeat,side=LABELS[label];start=time.monotonic()
                if phase in ('protected','broad-development'):protected_run(c,m[side+'_binary'],p.parent,bridge,ID,side)
                else:small_run(c['spec'],c['setup'],{'experiment':ID,'btc_binary':m[side+'_binary']},p.parent,bridge)
                validate(c,label,directory,phase)
                print(f'side_complete fixture={i+1} label={label} elapsed_seconds={time.monotonic()-start:.3f}',flush=True)
            p=directory/(str(seed(c))+'.fixture_complete.json')
            if p.exists():require(load(p)==marker(c,directory),'marker mismatch')
            else:write_new(p,marker(c,directory))
            print(f'fixture_complete count={i+1}',flush=True)
    finally:
        bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    verify(m)
    write_new(directory/'run_complete.json',{**counts,'execution_sha256':digest(M),
        'fixture_hashes':{p.name:digest(p) for p in sorted(directory.glob('*.fixture_complete.json'))}})
    print(f'run_complete phase={phase} results={counts["results"]}',flush=True)
    summarize(phase)


def aggregate(rows):
    return {'pairs':len(rows),'wtl':{k:sum(r['comparison']==k for r in rows) for k in ('win','tie','loss')},
        'tier_delta':[sum(r['delta'][i] for r in rows) for i in range(3)],
        'first_tiers':dict(Counter(str(r['first_tier']) for r in rows)),
        'servings_gain':sum(max(0,r['delta'][2]) for r in rows),
        'servings_loss':sum(max(0,-r['delta'][2]) for r in rows),
        'worst_servings_loss':max([0]+[-r['delta'][2] for r in rows])}


def narrow_gate(rows,phase):
    checks={};consistent=[]
    for repeat in ('A','B'):
        rr=[r for r in rows if r['repeat']==repeat];a=aggregate(rr)
        checks[repeat]=bool(a['tier_delta'][2]>0 and a['wtl']['win']>a['wtl']['loss'] and
            a['servings_gain']>=2*a['servings_loss'] and
            all(r['delta'][0]>=0 and r['delta'][1]>=0 and r['delta'][2]>=-1 for r in rr) and
            all(sum(r['delta'][2] for r in rr if r['family']==f)>=0 for f in {r['family'] for r in rr}))
    for s in {r['seed'] for r in rows}:
        rr=[r for r in rows if r['seed']==s]
        if len(rr)==2 and all(r['comparison']=='win' and r['activated_before_divergence'] for r in rr):consistent.append(rr[0])
    checks['repeatable_causal_breadth']=len(consistent)>={'development':4,'holdout':8}[phase] and len({r['family'] for r in consistent})>={'development':2,'holdout':3}[phase]
    return {'passed':all(checks.values()),'checks':checks,'repeatable_causal_wins':[r['seed'] for r in consistent]}


def protected_gate(rows,robust):
    checks={}
    for repeat in ('A','B'):
        rr=[r for r in rows if r['repeat']==repeat];a=aggregate(rr)
        checks[repeat]=bool(all(v>=0 for v in a['tier_delta']) and any(v>0 for v in a['tier_delta']) and
            a['wtl']['win']>a['wtl']['loss'] and a['servings_gain']>=2*a['servings_loss'] and
            all(r['delta'][0]>=0 and r['delta'][1]>=0 and
                r['delta'][2]>=-max(1,r['parent'][2]//50) for r in rr))
        checks[repeat+'_strata']=all(all(sum(r['delta'][i] for r in rr if r[k]==v)>=0 for i in range(3))
            for k in STRATA for v in {r[k] for r in rr})
    wins=sum(r['comparison']=='win' for r in robust);losses=sum(r['comparison']=='loss' for r in robust)
    n=wins+losses;p=sum(math.comb(n,k) for k in range(wins,n+1))/(2**n) if n else 1.0
    checks['robust_sign']=p<=0.05
    causal={r['seed'] for r in rows if r['comparison']=='win' and r['activated_before_divergence']}
    checks['fresh_causal_breadth']=sum(all(r['comparison']=='win' and r['activated_before_divergence']
        for r in rows if r['seed']==s) for s in causal)>=2
    return {'passed':all(checks.values()),'checks':checks,'robust_wins':wins,'robust_losses':losses,'one_sided_sign_p':p}


def summarize(phase):
    m=load(M);verify(m);cases=load(ROOT/m['splits'][phase]['path'])['cases'];counts=coverage(cases,phase)
    directory=ROOT/f'research/evidence/{ID}-{phase}';audit(cases,directory,phase)
    complete=load(directory/'run_complete.json')
    require(all(complete[k]==v for k,v in counts.items()) and complete['execution_sha256']==digest(M),'completion identity')
    markers=sorted(directory.glob('*.fixture_complete.json'))
    require(len(markers)==len(cases) and len(list(directory.glob('*/*/*.result.json')))==4*len(cases),'exact completion counts')
    require(complete['fixture_hashes']=={p.name:digest(p) for p in markers},'completion artifact drift')
    rows=[];controls=[];robust=[];cross=[]
    for c in cases:
        transports={label:load(location(c,label,directory).with_suffix('.transport.json')) for label in LABELS}
        for t in transports.values():t.setdefault('roles',[0]*len(c['setup']['agents']))
        fields={**c.get('spec',{}),'seed':seed(c),'family':c.get('family',c.get('spec',{}).get('family')),
            'fuel':c.get('fuel',str(c['setup']['fuelLimits'])),'side':c.get('side',c['setup']['map']['width']),
            'role':c.get('role','fixed-all-Patrol'),'days':len(c['setup']['daySteps']),
            'players':c['setup']['players'],'window_ms':c.get('window_ms',5000),'roadless':c.get('roadless',True),
            'agent_count':len(c['setup']['agents']),'brand_count':len({s['brand'] for s in c['setup']['spots']}),
            'stock_vector':str([s['stocks'] for s in c['setup']['spots']]),'step_vector':str(c['setup']['daySteps'])}
        for repeat in ('A','B'):
            ta,tb=transports['parent'+repeat],transports['candidate'+repeat]
            comparison=compare_runs(ta,tb);a,b=tb['score'],ta['score'];outcome,tier,difference=compare(a,b)
            replay=location(c,'candidate'+repeat,directory).with_suffix('.replay.jsonl');dd=decisions(replay)
            telem=[measured_day(d,tb['actions'][i+1]['plan'] if i+1<len(dd) else None) for i,d in enumerate(dd)]
            causal=[check_causal(d) for d in dd]
            activation=next((i+1 for i,x in enumerate(causal) if x['replacements']),None)
            divergence=comparison['first_divergence']
            row={**fields,'repeat':repeat,'parent':b,'candidate':a,'comparison':outcome,'first_tier':tier,
                'first_tier_difference':difference,'delta':[x-y for x,y in zip(a,b,strict=True)],
                'trajectory':comparison,'first_certificate_gain_day':activation,
                'activated_before_divergence':activation is not None and divergence is not None and activation<=divergence,
                'pricing_days':telem,'same_invocation_causal':causal,
                'safety':{side:(operational(location(c,side+repeat,directory).with_suffix('.replay.jsonl'),c,side)
                    if phase in ('protected','broad-development') else safety(location(c,side+repeat,directory).with_suffix('.replay.jsonl')))
                    for side in ('parent','candidate')},
                'result_hashes':{side:digest(location(c,side+repeat,directory).with_suffix('.result.json')) for side in ('parent','candidate')}}
            rows.append(row)
        for side in ('parent','candidate'):
            controls.append({**fields,'binary':side,**compare_runs(transports[side+'A'],transports[side+'B'])})
        for cr in ('A','B'):
            for pr in ('A','B'):
                o,t,d=compare(transports['candidate'+cr]['score'],transports['parent'+pr]['score'])
                cross.append({'seed':seed(c),'candidate_repeat':cr,'parent_repeat':pr,'comparison':o,'first_tier':t,'difference':d})
        ca=sorted(tuple(transports['candidate'+r]['score']) for r in ('A','B'))
        pa=sorted(tuple(transports['parent'+r]['score']) for r in ('A','B'))
        robust.append({**fields,'candidate_interval':ca,'parent_interval':pa,
            'comparison':'win' if ca[0]>pa[1] else 'loss' if ca[1]<pa[0] else 'exact-tie' if ca[0]==ca[1]==pa[0]==pa[1] else 'unresolved'})
    gate=protected_gate(rows,robust) if phase in ('protected','broad-development') else narrow_gate(rows,phase)
    if phase=='broad-development':
        gate['diagnostic_sign_and_causal']={k:v for k,v in gate['checks'].items() if k not in ('A','B','A_strata','B_strata')}
        gate['checks']={k:v for k,v in gate['checks'].items() if k in ('A','B','A_strata','B_strata')}
        gate['passed']=all(gate['checks'].values())
    report={'experiment':ID,'phase':phase,'complete':True,**counts,'gate':gate,'summary':aggregate(rows),
        'repeat_summaries':{r:aggregate([x for x in rows if x['repeat']==r]) for r in ('A','B')},
        'strata':{k:{str(v):aggregate([r for r in rows if r[k]==v]) for v in sorted({r[k] for r in rows})} for k in STRATA},
        'rows':rows,'same_binary_controls':controls,'robust_intervals':robust,'cross_repeat_comparisons':cross,
        'execution_sha256':digest(M),'completion_sha256':digest(directory/'run_complete.json'),
        'authority':'Synthetic actual Windows HTTP; not official BTC, performance or promotion. No noise subtraction.'}
    p=ROOT/f'research/evidence/{ID}-{phase}.summary.json';write_new(p,report)
    print({'phase':phase,'summary':report['summary'],'gate':gate,'summary_sha256':digest(p)},flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('mode',choices=('run','summarize'))
    parser.add_argument('--phase',choices=('development','broad-development','holdout','protected'),default='development')
    parser.add_argument('--resume',action='store_true');a=parser.parse_args()
    run(a.phase,a.resume) if a.mode=='run' else summarize(a.phase)
