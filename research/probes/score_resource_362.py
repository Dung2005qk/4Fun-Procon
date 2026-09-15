"""Prospective362 controller; exact357 lifecycle/certificate machinery retained."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

WORK=Path('/home/LMC/udon362-0909')
BASE=Path('/home/LMC/udon360-0909')
PACKAGE=Path('/home/LMC/udon357-0909')
ID='SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362'
EXEC=WORK/'execution362.json'
INPUT=WORK/'input362.json'

def load(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def write(p,obj):
    p.parent.mkdir(parents=True,exist_ok=True)
    with p.open('x')as stream:json.dump(obj,stream,indent=2,sort_keys=True);stream.write('\n')

sys.path.insert(0,str(PACKAGE/'research/probes'))
import run_resource_357 as f
f.ROOT=WORK;f.M=EXEC;f.INPUT=INPUT;f.ID=ID;f.rpc.ID=ID


def boundary():
    assert sha(BASE/'execution360.json')=='47488178393C4578A1D28795CA2EFDC9329F0B6BE96A75967774446F28FC68F9'
    assert sha(BASE/'result360.json')=='F61EB2B622550443556D56B1975F011C46A9385ED9F1C1003889E1C608449808'
    sys.path.insert(0,str(BASE))
    import late_control_360 as c
    c.verify_execution()
    sys.path.insert(0,'/home/LMC/udon361-0909')
    import resume_control_361 as r
    assert sha(r.WORK/'complete361.json')=='6A9478EA267C5220A69376DC4F584AB40915E67F4ABD385F48744772398A3184'
    r.verify();assert r.summary()==load(r.WORK/'complete361.json')
    sys.path.insert(0,str(WORK))
    from verify_late_control_360_copy import guarded_reachability
    text=(BASE/'btc360-plain.disasm.txt').read_text()
    guarded_reachability(text,0x3A110)
    assert sha(BASE/'btc360-plain')=='D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678'
    return True


def verify():
    assert subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829'
    m=load(EXEC)
    for p,h in m['hashes'].items():assert sha(WORK/p)==h,p
    assert m['parent_binary']==m['candidate_binary']==str(BASE/'btc360-plain')
    boundary()
    return m


def classify(cases,rows,robust,phase):
    checks={}
    for repeat in ('A','B'):
        rr=[r for r in rows if r['repeat']==repeat]
        active=[r for r in rr if r['window_ms']>5000];a=f.aggregate(active)
        checks[repeat+'_active_benefit']=all(v>=0 for v in a['delta']) and any(v>0 for v in a['delta']) and a['wtl'].get('win',0)>a['wtl'].get('loss',0) and a['gross_serving_gain']>=2*a['gross_serving_loss']
        checks[repeat+'_all_downside']=all(r['delta'][0]>=0 and r['delta'][1]>=0 and r['delta'][2]>=-max(1,r['A'][2]/50)for r in rr) and all(v>=0 for v in f.aggregate(rr)['delta'])
        checks[repeat+'_all_strata']=all(all(sum(r['delta'][k]for r in rr if r[s]==v)>=0 for k in range(3))for s in f.STRATA for v in {r[s]for r in rr})
    checks['causal_inactive_operation_equivalence']=all(not r['certificate']['frames'] and r['first_takeover']is None for r in rows if r['window_ms']==5000)
    consistent=[c for c in cases if c['window_ms']>5000 and all(r['comparison_B_vs_A']=='win' and r['causal']for r in rows if r['seed']==c['seed'])]
    checks['causal_breadth']=len(consistent)>={'development':2,'holdout':4,'protected':2}[phase] and (phase=='protected' or len({c['family']for c in consistent})>=2)
    if phase=='holdout':checks['both_long_windows']={c['window_ms']for c in consistent}>={10000,15000}
    ids={c['seed']for c in consistent}
    w=sum(x['window_ms']>5000 and x['outcome']=='win' and x['seed']in ids for x in robust)
    losses=sum(x['window_ms']>5000 and x['outcome']=='loss' for x in robust)
    n=w+losses;p=sum(math.comb(n,k)for k in range(w,n+1))/2**n if n else 1.0
    if phase!='development':checks['robust_active_causal_sign']=p<=0.05
    return {'passed':all(checks.values()),'checks':checks,'robust_sign_p':p,'robust_causal_wins':w,
        'all_robust_active_losses':losses,'consistent_causal_seeds':sorted(ids)}


def summarize(phase,check=False):
    verify();cases=load(WORK/load(INPUT)['splits'][phase]['path'])['cases']
    d=WORK/f'research/evidence/{ID}-{phase}';complete=load(d/'run_complete.json')
    assert complete['execution_sha256']==sha(EXEC)
    assert len(list(d.glob('*/*/*.result.json')))==4*len(cases) and len(list(d.glob('*.fixture_complete.json')))==len(cases)
    for p,h in complete['files'].items():assert sha(d/p)==h,p
    f.audit(cases,d);rows=[];controls=[];robust=[]
    for c in cases:
        ts={label:load(f.prefix(c,label,d).with_suffix('.transport.json'))for label in f.LABELS}
        common={k:c[k]for k in (*f.STRATA,'seed')}
        for repeat in ('A','B'):
            comp=f.compare_runs(ts['parent'+repeat],ts['candidate'+repeat])
            cs=load(f.prefix(c,'candidate'+repeat,d).with_suffix('.certificate.json'))
            first=next((x['day']for x in cs['frames']if x['takeover']),None)
            rows.append({**common,'repeat':repeat,**comp,'first_takeover':first,
                'causal':first is not None and comp['first_divergence']is not None and first<=comp['first_divergence'],
                'certificate':cs,'safety':True})
        for side in ('parent','candidate'):controls.append({**common,'side_binary':side,**f.compare_runs(ts[side+'A'],ts[side+'B'])})
        a=sorted(tuple(ts['parent'+r]['score'])for r in ('A','B'));b=sorted(tuple(ts['candidate'+r]['score'])for r in ('A','B'))
        robust.append({**common,'parent_interval':a,'candidate_interval':b,'outcome':'win'if b[0]>a[1]else'loss'if b[1]<a[0]else'exact-tie'if a[0]==a[1]==b[0]==b[1]else'unresolved'})
    report={'experiment':ID,'phase':phase,'complete':True,'execution_sha256':sha(EXEC),'completion_sha256':sha(d/'run_complete.json'),
        'gate':classify(cases,rows,robust,phase),'summary':f.aggregate(rows),
        'active_only_summary':f.aggregate([r for r in rows if r['window_ms']>5000]),
        'repeats':{r:f.aggregate([x for x in rows if x['repeat']==r])for r in ('A','B')},
        'strata':{k:{str(v):f.aggregate([r for r in rows if r[k]==v])for v in sorted({r[k]for r in rows})}for k in f.STRATA},
        'rows':rows,'same_binary_controls':controls,'robust_intervals':robust,'zero_safety_failure':True,
        'independent5000_trajectory_mismatches':sum(not r['trajectory_equal']for r in rows if r['window_ms']==5000),
        'independent5000_score_mismatches':sum(r['A']!=r['B']for r in rows if r['window_ms']==5000),
        'boundary_evidence360':'F61EB2B622550443556D56B1975F011C46A9385ED9F1C1003889E1C608449808',
        'boundary_evidence361':'6A9478EA267C5220A69376DC4F584AB40915E67F4ABD385F48744772398A3184',
        'authority':'prospective362 causal operation equivalence; every independent difference retained; no357 reclassification or automatic promotion'}
    report=json.loads(json.dumps(report));p=WORK/f'research/evidence/{ID}-{phase}.summary.json'
    if check:assert load(p)==report,'summary drift'
    else:write(p,report)
    print('summary_complete',sha(p),json.dumps(report['gate']),flush=True)


def freeze():
    assert not EXEC.exists()
    stage=load(WORK/'stage362.json')
    for p,h in stage['hashes'].items():assert sha(WORK/p)==h,p
    boundary()
    write(EXEC,{'experiment':ID,'parent_binary':str(BASE/'btc360-plain'),
        'candidate_binary':str(BASE/'btc360-plain'),'hashes':stage['hashes'],
        'frozen360_execution':sha(BASE/'execution360.json'),
        'frozen361_complete':sha(Path('/home/LMC/udon361-0909/complete361.json'))})
    print('execution362_frozen',sha(EXEC),flush=True)


f.verify=verify;f.summarize=summarize  # prospective controller, never mutates frozen357 code/data.
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['freeze','run','summarize']);p.add_argument('--phase',choices=['development','holdout','protected'],default='development');p.add_argument('--resume',action='store_true');p.add_argument('--check',action='store_true');args=p.parse_args()
    if args.mode=='freeze':freeze()
    elif args.mode=='run':f.run(args.phase,args.resume)
    else:summarize(args.phase,args.check)
