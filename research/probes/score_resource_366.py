"""Frozen366 qualification; original357 lifecycle and certificates unchanged."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from gate_resource_366 import classify

WORK=Path('/home/LMC/udon366-0909')
BASE=Path('/home/LMC/udon360-0909')
PACKAGE=Path('/home/LMC/udon357-0909')
ID='SCORE-CAUSAL-RESOURCE-QUALIFICATION-366'
EXEC=WORK/'execution366.json';INPUT=WORK/'input366.json'
sys.path.insert(0,str(PACKAGE/'research/probes'))
import run_resource_357 as f
load=f.load;sha=f.sha;write=f.write
f.ROOT=WORK;f.M=EXEC;f.INPUT=INPUT;f.ID=ID;f.rpc.ID=ID

def boundary():
    sys.path.insert(0,str(BASE))
    import late_control_360 as c
    c.verify_execution()
    from verify_late_control_360_copy import guarded_reachability
    guarded_reachability((BASE/'btc360-plain.disasm.txt').read_text(),0x3A110)
    assert sha(BASE/'btc360-plain')=='D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678'
    for root,name,expected in (
        ('udon361-0909','complete361.json','6A9478EA267C5220A69376DC4F584AB40915E67F4ABD385F48744772398A3184'),
        ('udon364-0909','complete364.json','8DDF91A0C5688855AB979383551F9ABBC7AD854FD6B397384571D21219113FB2'),
        ('udon365-0909','complete365.json','CBB77A9B789B87F1A0D5AA06F9B66BF3EEA1E62FAB9C03F7FFEAE58D4336771D')):
        assert sha(Path('/home/LMC')/root/name)==expected,root
    return True

def verify():
    assert subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829'
    m=load(EXEC)
    for p,h in m['hashes'].items():assert sha(WORK/p)==h,p
    assert m['parent_binary']==m['candidate_binary']==str(BASE/'btc360-plain')
    boundary();return m

def summarize(phase,check=False):
    verify();cases=load(WORK/load(INPUT)['splits'][phase]['path'])['cases']
    d=WORK/f'research/evidence/{ID}-{phase}';complete=load(d/'run_complete.json')
    assert complete['execution_sha256']==sha(EXEC)
    assert len(list(d.glob('*/*/*.result.json')))==4*len(cases)
    assert len(list(d.glob('*.fixture_complete.json')))==len(cases)
    for p,h in complete['files'].items():assert sha(d/p)==h,p
    f.audit(cases,d);rows=[];controls=[];robust=[]
    for c in cases:
        ts={label:load(f.prefix(c,label,d).with_suffix('.transport.json')) for label in f.LABELS}
        common={k:c[k] for k in (*f.STRATA,'seed')}
        for repeat in ('A','B'):
            comp=f.compare_runs(ts['parent'+repeat],ts['candidate'+repeat])
            cs=load(f.prefix(c,'candidate'+repeat,d).with_suffix('.certificate.json'))
            first=next((x['day'] for x in cs['frames'] if x['takeover']),None)
            rows.append({**common,'repeat':repeat,**comp,'first_takeover':first,
                'causal':first is not None and comp['first_divergence'] is not None and first<=comp['first_divergence'],
                'certificate':cs,'safety':True})
        for side in ('parent','candidate'):
            controls.append({**common,'side_binary':side,**f.compare_runs(ts[side+'A'],ts[side+'B'])})
        a=sorted(tuple(ts['parent'+r]['score']) for r in ('A','B'))
        b=sorted(tuple(ts['candidate'+r]['score']) for r in ('A','B'))
        robust.append({**common,'parent_interval':a,'candidate_interval':b,
            'outcome':'win' if b[0]>a[1] else 'loss' if b[1]<a[0] else 'exact-tie' if a[0]==a[1]==b[0]==b[1] else 'unresolved'})
    report={'experiment':ID,'phase':phase,'complete':True,'execution_sha256':sha(EXEC),
        'completion_sha256':sha(d/'run_complete.json'),'gate':classify(cases,rows,robust,phase),
        'summary':f.aggregate(rows),'active_only_summary':f.aggregate([r for r in rows if r['window_ms']>5000]),
        'repeats':{r:f.aggregate([x for x in rows if x['repeat']==r]) for r in ('A','B')},
        'strata':{k:{str(v):f.aggregate([r for r in rows if r[k]==v]) for v in sorted({r[k] for r in rows})} for k in f.STRATA},
        'rows':rows,'same_binary_controls':controls,'robust_intervals':robust,'zero_safety_failure':True,
        'independent5000_trajectory_mismatches':sum(not r['trajectory_equal'] for r in rows if r['window_ms']==5000),
        'independent5000_score_mismatches':sum(r['A']!=r['B'] for r in rows if r['window_ms']==5000),
        'authority':'366 fresh prospective qualification of unchanged362; every difference retained; no retrospective pass or two-agent research'}
    report=json.loads(json.dumps(report));p=WORK/f'research/evidence/{ID}-{phase}.summary.json'
    if check:assert load(p)==report,'summary drift'
    else:write(p,report)
    print('summary_complete',sha(p),json.dumps(report['gate']),flush=True)

def freeze():
    assert not EXEC.exists();stage=load(WORK/'stage366.json')
    for p,h in stage['hashes'].items():assert sha(WORK/p)==h,p
    boundary()
    write(EXEC,{'experiment':ID,'parent_binary':str(BASE/'btc360-plain'),
        'candidate_binary':str(BASE/'btc360-plain'),'hashes':stage['hashes']})
    print('execution366_frozen',sha(EXEC),flush=True)

f.verify=verify;f.summarize=summarize
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['freeze','run','summarize'])
    p.add_argument('--phase',choices=['development','holdout','protected'],default='development')
    p.add_argument('--resume',action='store_true');p.add_argument('--check',action='store_true');a=p.parse_args()
    if a.mode=='freeze':freeze()
    elif a.mode=='run':f.run(a.phase,a.resume)
    else:summarize(a.phase,a.check)
