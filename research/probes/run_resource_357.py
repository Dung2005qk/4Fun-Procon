"""Frozen357 full daily-policy A/B. Uses352 I/O, replaces only obsolete pricing audit."""
import argparse
from collections import Counter
import json
import math
import os
import subprocess
import unittest
from pathlib import Path
import process_runtime_352 as rpc
import protected_http_transport_341 as tr
from refinement_cost_353 import ROOT,sha,load,write,require
from inactive_runtime_aa_342 import compare_runs,LABELS,SUFFIXES
from run_pair_protected_341 import validate_side
from run_pair_score_341 import decisions
ID='SCORE-PROTECTED-RESOURCE-MARGINAL-357'
M=ROOT/'execution357.json'
INPUT=ROOT/f'research/holdouts/{ID}.json'
STRATA=('family','fuel','side','window_ms','role','days','players','roadless')

def records(path):return [json.loads(line)for line in path.read_text().splitlines()]
def canonical_day(day):require('horizonPricing' not in day['profile'],'old pricing candidate accidentally included')
def mechanism_safety(replay):
    for day in decisions(replay):canonical_day(day)
    frames=[e['body']for e in records(replay)if e['kind']=='resource_marginal']
    require(len({x['day']for x in frames})==len(frames),'duplicate suffix frame')
    for x in frames:
        require(not x['failure'],'resource certificate/runtime failure')
        require(0<=x['queries']<=8 and 0<=x['settled']<=1250000*x['queries'],'work cap')
        require(0<=x['certified']<=x['valid']<=x['evaluated']<=x['routes']<=32*x['queries'],'evaluation cap')
        if x['takeover']:require(x['entered'] and not x['deadline'] and x['certified']>0 and x['outputScore']>x['parentScore'],'uncertified takeover')
        else:require(x['outputPlan']==x['parentPlan'] and x['outputScore']==x['parentScore'],'no-takeover mutation')
    return frames

# The generic lifecycle safety remains unchanged.350's pricing-specific schema is
# not a357 function: replace that assertion with357's complete certified-suffix audit.
tr.pricing_safety=mechanism_safety
rpc.check_causal=canonical_day
rpc.ID=ID

def verify():
    m=load(M)
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'frozen drift:'+p)
    require(m['parent_binary']==m['candidate_binary'],'same binary required')
    return m
def prefix(c,label,d):
    repeat,side=LABELS[label];return d/repeat/side/str(c['seed'])
def certificates(c,p,bridge):
    t=load(p.with_suffix('.transport.json'));r=p.with_suffix('.replay.jsonl');frames=mechanism_safety(r)
    ledger=tr.EMPTY_LEDGER if hasattr(tr,'EMPTY_LEDGER')else {'brands':[],'totalDailyDistinct':0,'totalServings':0}
    by={x['day']:x for x in frames};out=[]
    for i,a in enumerate(t['actions']):
        x=by.get(i+1)
        if x is not None:
            require(c['setup']['daySeconds'][i]>5 and i+1<c['days'],'illegal activation lane')
            if x['entered']:
                q={'op':'step','setup':c['setup'],'state':dict(a['state'],day=i+1,endsAt=a['state']['endsAt']//1000),'ledger':ledger}
                before=bridge.request({**q,'plan':x['parentPlan']});after=bridge.request({**q,'plan':x['outputPlan']})
                require(before.get('ok') and before.get('agrees') and after.get('ok') and after.get('agrees'),'independent suffix validity')
                require(before['score']==x['parentScore'] and after['score']==x['outputScore'] and x['outputPlan']==a['plan'],'causal plan/score ownership')
                require(before['road_footprint']==after['road_footprint'],'road certificate')
                require(all(b['kind']==z['kind'] and b['pos']==z['pos'] and b['fuel']<=z['fuel'] for b,z in zip(before['agents'],after['agents'],strict=True)),'agent certificate')
                require(set(before['ledger']['brands'])<=set(after['ledger']['brands']) and all(before['score'][k]<=after['score'][k]for k in range(3)),'ledger certificate')
                out.append({'day':i+1,'takeover':x['takeover'],'before':before,'after':after,'work':x})
        ledger=a['validated']['ledger']
    return {'independently_checked':True,'frames':frames,'certificates':out}

def configure_side(side):os.environ['UDON_RESOURCE_MARGINAL_357']='1' if side=='candidate'else'0'
def marker(c,d):return {'seed':c['seed'],'results':{l:sha(prefix(c,l,d).with_suffix('.result.json'))for l in LABELS},
    'certificates':{l:sha(prefix(c,l,d).with_suffix('.certificate.json'))for l in LABELS}}
def audit(cases,d):
    for c in cases:
        for label in LABELS:
            p=prefix(c,label,d);repeat,side=LABELS[label]
            if p.with_suffix('.result.json').exists():
                validate_side(c,side,d/repeat)
                require(p.with_suffix('.certificate.json').exists(),'missing atomic certificate audit; do not rerun match')
            else:require(not list(p.parent.glob(p.name+'.*')),'ambiguous partial side; never duplicate accepted days')
        mark=d/f"{c['seed']}.fixture_complete.json"
        if mark.exists():require(load(mark)==marker(c,d),'fixture marker drift')

def run(phase,resume=False):
    m=verify();manifest=load(INPUT)
    previous={'holdout':'development','protected':'holdout'}.get(phase)
    if previous:require(load(ROOT/f'research/evidence/{ID}-{previous}.summary.json')['gate']['passed'],'previous gate failed')
    split=manifest['splits'][phase];require(sha(ROOT/split['path'])==split['sha256'],'split drift')
    cases=load(ROOT/split['path'])['cases'];d=ROOT/f'research/evidence/{ID}-{phase}'
    require(len(cases)==split['fixtures'],'count')
    if d.exists():require(resume and not(d/'run_complete.json').exists(),'existing run')
    else:
        for r,s in LABELS.values():(d/r/s).mkdir(parents=True,exist_ok=True)
    audit(cases,d)
    with rpc.BridgeContext()as bridge:
        for i,c in enumerate(cases):
            for label in c['run_order']:
                p=prefix(c,label,d);repeat,side=LABELS[label]
                if p.with_suffix('.result.json').exists():continue
                configure_side(side)
                rpc.run_one(c,ROOT/m[side+'_binary'],p,bridge,side)
                checked=certificates(c,p,bridge)
                if side=='parent':require(not checked['frames'],'parent suffix activated')
                write(p.with_suffix('.certificate.json'),checked)
                print('side_complete fixture='+str(i+1)+' label='+label,flush=True)
            mark=d/f"{c['seed']}.fixture_complete.json"
            if not mark.exists():write(mark,marker(c,d))
            print('fixture_complete '+str(i+1),flush=True)
    audit(cases,d);verify()
    write(d/'run_complete.json',{'execution_sha256':sha(M),'fixtures':len(cases),'results':4*len(cases),'actions':split['actions'],
        'files':{p.relative_to(d).as_posix():sha(p)for p in d.rglob('*')if p.is_file()}})
    print('run_complete '+phase,flush=True);summarize(phase)

def aggregate(rows):
    return {'pairs':len(rows),'wtl':dict(Counter(r['comparison_B_vs_A']for r in rows)),
        'delta':[sum(r['delta'][k]for r in rows)for k in range(3)],
        'gross_serving_gain':sum(max(0,r['delta'][2])for r in rows),'gross_serving_loss':sum(max(0,-r['delta'][2])for r in rows),
        'worst_serving_loss':max([0]+[-r['delta'][2]for r in rows]),'first_tiers':dict(Counter(str(r['first_tier'])for r in rows))}
def summarize(phase,check=False):
    m=verify();cases=load(ROOT/load(INPUT)['splits'][phase]['path'])['cases'];d=ROOT/f'research/evidence/{ID}-{phase}'
    complete=load(d/'run_complete.json');require(complete['execution_sha256']==sha(M),'completion identity')
    require(len(list(d.glob('*/*/*.result.json')))==4*len(cases) and len(list(d.glob('*.fixture_complete.json')))==len(cases),'complete counts')
    for p,h in complete['files'].items():require(sha(d/p)==h,'evidence drift')
    audit(cases,d);rows=[];controls=[];robust=[]
    for c in cases:
        ts={l:load(prefix(c,l,d).with_suffix('.transport.json'))for l in LABELS}
        common={k:c[k]for k in (*STRATA,'seed')}
        for repeat in ('A','B'):
            comp=compare_runs(ts['parent'+repeat],ts['candidate'+repeat]);cs=load(prefix(c,'candidate'+repeat,d).with_suffix('.certificate.json'))
            first=next((x['day']for x in cs['frames']if x['takeover']),None)
            rows.append({**common,'repeat':repeat,**comp,'first_takeover':first,
                'causal':first is not None and comp['first_divergence']is not None and first<=comp['first_divergence'],
                'certificate':cs,'safety':True})
        for side in ('parent','candidate'):controls.append({**common,'side_binary':side,**compare_runs(ts[side+'A'],ts[side+'B'])})
        a=sorted(tuple(ts['parent'+r]['score'])for r in ('A','B'));b=sorted(tuple(ts['candidate'+r]['score'])for r in ('A','B'))
        robust.append({**common,'parent_interval':a,'candidate_interval':b,'outcome':'win'if b[0]>a[1]else'loss'if b[1]<a[0]else'exact-tie'if a[0]==a[1]==b[0]==b[1]else'unresolved'})
    checks={}
    for repeat in ('A','B'):
        rr=[r for r in rows if r['repeat']==repeat];a=aggregate(rr)
        checks[repeat]=all(v>=0 for v in a['delta']) and any(v>0 for v in a['delta']) and a['wtl'].get('win',0)>a['wtl'].get('loss',0) and a['gross_serving_gain']>=2*a['gross_serving_loss'] and all(r['delta'][0]>=0 and r['delta'][1]>=0 and r['delta'][2]>=-max(1,r['A'][2]/50)for r in rr)
        checks[repeat+'_strata']=all(all(sum(r['delta'][k]for r in rr if r[s]==v)>=0 for k in range(3))for s in STRATA for v in {r[s]for r in rr})
    checks['exact5000']=all(r['trajectory_equal'] and r['A']==r['B'] and r['first_takeover']is None for r in rows if r['window_ms']==5000)
    consistent=[c for c in cases if all(r['comparison_B_vs_A']=='win' and r['causal']for r in rows if r['seed']==c['seed'])]
    checks['causal_breadth']=len(consistent)>={'development':2,'holdout':4,'protected':2}[phase] and (phase=='protected' or len({c['family']for c in consistent})>=2)
    if phase=='holdout':checks['both_long_windows']={c['window_ms']for c in consistent}>={10000,15000}
    w=sum(x['outcome']=='win'for x in robust);l=sum(x['outcome']=='loss'for x in robust);n=w+l
    p=sum(math.comb(n,k)for k in range(w,n+1))/2**n if n else 1.0
    if phase!='development':checks['robust_sign']=p<=0.05
    report={'experiment':ID,'phase':phase,'complete':True,'execution_sha256':sha(M),'completion_sha256':sha(d/'run_complete.json'),
        'gate':{'passed':all(checks.values()),'checks':checks,'robust_sign_p':p,'consistent_causal_seeds':[c['seed']for c in consistent]},
        'summary':aggregate(rows),'repeats':{r:aggregate([x for x in rows if x['repeat']==r])for r in ('A','B')},
        'strata':{k:{str(v):aggregate([r for r in rows if r[k]==v])for v in sorted({r[k]for r in rows})}for k in STRATA},
        'rows':rows,'same_binary_controls':controls,'robust_intervals':robust,'zero_safety_failure':True,
        'authority':'VM synthetic full policy; not official BTC; no automatic promotion'}
    s=ROOT/f'research/evidence/{ID}-{phase}.summary.json'
    report=json.loads(json.dumps(report))
    if check:require(load(s)==report,'summary recomputation drift')
    else:write(s,report)
    print('summary_complete '+sha(s)+' '+json.dumps(report['gate']),flush=True)

def contracts():
    import test_protected_http_341 as t
    names=['test_fixed_roles_from_day_one','test_native_real_assignment_before_state','test_invalid_roles_do_not_mutate',
        'test_native_assignment_idempotent_before_start_only','test_variable_windows_and_ten_day_completion',
        'test_late_or_changed_action_never_commits','test_unanswered_day_and_invalid_plan_fail_closed',
        'test_source_cost_and_own_road_footprint_are_exact','test_two_day_traffic_thresholds_and_expiration',
        'test_team_normalization_and_exact_threshold_boundary','test_state_traffic_frozen_for_open_day']
    t.ProtectedContract.setUpClass=classmethod(lambda cls:setattr(cls,'bridge',tr.Bridge(ROOT/'bridge352')))
    r=unittest.TextTestRunner().run(unittest.TestSuite(t.ProtectedContract(n)for n in names));require(r.wasSuccessful(),'traffic/ACK contracts')
    with rpc.BridgeContext()as bridge:
        for side in ('parent','candidate'):
            c=t.contract_case('native'if side=='candidate'else'fixed-all-Patrol');folder=ROOT/'contracts357'/side;folder.mkdir(parents=True)
            configure_side(side);rpc.run_one(c,ROOT/'build/udonshield_btc',folder/str(c['seed']),bridge,side)
            write((folder/str(c['seed'])).with_suffix('.certificate.json'),certificates(c,folder/str(c['seed']),bridge))
    write(ROOT/'contracts357.json',{'passed':True,'traffic_contracts':11,'policy_matches':2,
        'files':{p.relative_to(ROOT).as_posix():sha(p)for p in (ROOT/'contracts357').rglob('*')if p.is_file()}})

def freeze():
    require(load(ROOT/'contracts357.json')['passed']and load(ROOT/'unit357.json')['passed'],'contracts before measurement')
    package=load(ROOT/'package357.manifest.json')
    for p,h in package['hashes'].items():require(sha(ROOT/p)==h,'package drift')
    paths=['bridge352','build/udonshield_btc','build/libudon_shield.a','build/CMakeCache.txt','contracts357.json','unit357.json','source357.manifest.json','package357.manifest.json']
    write(M,{'experiment':ID,'parent_binary':'build/udonshield_btc','candidate_binary':'build/udonshield_btc',
        'hashes':{**package['hashes'],**{p:sha(ROOT/p)for p in paths}}})
    print('357 execution frozen '+sha(M),flush=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['contracts','freeze','run','summarize']);p.add_argument('--phase',choices=['development','holdout','protected'],default='development');p.add_argument('--resume',action='store_true');p.add_argument('--check',action='store_true');a=p.parse_args()
    if a.mode=='contracts':contracts()
    elif a.mode=='freeze':freeze()
    elif a.mode=='run':run(a.phase,a.resume)
    else:summarize(a.phase,a.check)
