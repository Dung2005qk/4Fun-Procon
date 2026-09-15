"""Complete366 holdout provenance and exact frozen-summary rederivation only."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tarfile

ROOT=Path(__file__).resolve().parents[2]
COPY=ROOT/'artifacts/research/366/completed-holdout'
OLD=ROOT/'artifacts/research/357/completed'
ID='SCORE-CAUSAL-RESOURCE-QUALIFICATION-366'
DATA=COPY/'research/evidence'/(ID+'-holdout')

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p):return json.loads(p.read_text(encoding='utf-8'))

def extract_once():
    archive=COPY.parent/'complete-holdout.tar.gz'
    assert sha(archive)=='EF2D81FE9EB155E099D49C5DD3A0181E84E638B1807E3D245C10A0E02A6591CA'
    if COPY.exists():return
    with tarfile.open(archive) as tf:
        members=tf.getmembers()
        assert len(members)==len({m.name for m in members})==1582
        for m in members:
            p=(COPY/m.name).resolve()
            assert m.isfile() and p.is_relative_to(COPY.resolve()) and not p.exists(),m.name
        COPY.mkdir()
        tf.extractall(COPY,members=members,filter='data')

def verify_copy():
    expected={'input366.json':'5447CE02A11D6803B0EB9878A84B6B343A4E2EBA8E2D1EFAD6F9A6C6615A5B8F',
        'stage366.json':'EF76810EEA3BBCDCB82E66CC15D0929F5BF9408975514A6113AE8E43AE37EE89',
        'execution366.json':'94CF0B0FC04DE3277BD0584C2E54898D510F0B076923E8EECC9FBB7F02D648AC',
        'score_resource_366.py':'6FB21016FD4A6C65D70BF402253BDE35781F7663E95C54AF912D99A7F0459215',
        'gate_resource_366.py':'4ABDD8E700D1256F98EDD4FFED83513AC920C123D76AD452C663F08F536B42F9',
        'runner366-holdout.json':'217B9F0F55592DD5B2A62D48B73A873FD7A0BBAB441714DD15A455104C797E8B',
        f'research/holdouts/{ID}-holdout.json':'30E006228278FA030FE443EE2AE2CCDDAC21BE5529EA14DAED46B07AD41F7778',
        f'research/evidence/{ID}-holdout.summary.json':'88E7FF81955C7AFAC838D430508809B8B21C2AC00C9906E7BA6E3994D9EF00B3',
        f'research/evidence/{ID}-holdout/run_complete.json':'EE02542F975343DC8E202C343EFD1F6EC57C2685F714994A076CEB71F65291A4'}
    for p,h in expected.items():assert sha(COPY/p)==h,p
    m=load(COPY/'execution366.json')
    for p,h in m['hashes'].items():assert sha(COPY/p)==h,p
    from verify_late_control_360_copy import verify as boundary360
    from verify_resume_control_361_copy import verify as boundary361
    from verify_takeover_resume_364_copy import verify as boundary364
    from verify_gain_short_window_365_copy import verify as boundary365
    assert boundary360()['verified'] and boundary361()['verified']
    assert boundary364()['verified'] and boundary365()['verified']
    complete=load(DATA/'run_complete.json')
    actual={p.relative_to(DATA).as_posix() for p in DATA.rglob('*') if p.is_file() and p.name!='run_complete.json'}
    assert actual==set(complete['files']) and len(actual)==1566
    for p,h in complete['files'].items():assert sha(DATA/p)==h,p
    results=list(DATA.glob('*/*/*.result.json'))
    assert len(results)==len(list(DATA.glob('*/*/*.certificate.json')))==216
    assert len(list(DATA.glob('*.fixture_complete.json')))==54
    rr=[load(p) for p in results]
    assert sum(r['actions'] for r in rr)==1368 and sum(r['transitions'] for r in rr)==1152
    assert all(r['failure'] is None for r in rr)
    assert all(p.stat().st_size==0 for p in DATA.rglob('*.stderr'))
    assert (COPY/'runner366-holdout.stderr').stat().st_size==0
    return m

def main():
    extract_once();verify_copy()
    sys.path.insert(0,str(OLD/'research/probes'));sys.path.insert(0,str(COPY))
    spec=importlib.util.spec_from_file_location('frozen366_holdout_copy',COPY/'score_resource_366.py')
    s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
    s.WORK=COPY;s.EXEC=COPY/'execution366.json';s.INPUT=COPY/'input366.json';s.verify=verify_copy
    s.summarize('holdout',check=True)
    report=load(COPY/'research/evidence'/(ID+'-holdout.summary.json'))
    rows=report['rows'];inactive=[r for r in rows if r['window_ms']==5000]
    frames=[x for r in rows for x in r['certificate']['frames']]
    audit={'verified':True,'files':1566,'results':216,'fixtures':54,'acks':1368,'transitions':1152,
        'summary_sha256':sha(COPY/'research/evidence'/(ID+'-holdout.summary.json')),
        'completion_sha256':sha(DATA/'run_complete.json'),'archive_sha256':sha(COPY.parent/'complete-holdout.tar.gz'),
        'gate':report['gate'],'summary':report['summary'],'active':report['active_only_summary'],
        'inactive':s.f.aggregate(inactive),'repeats':report['repeats'],'strata':report['strata'],
        'per_repeat_strata':{repeat:{k:{str(v):s.f.aggregate([r for r in rows if r['repeat']==repeat and r[k]==v])
            for v in sorted({r[k] for r in rows})} for k in s.f.STRATA} for repeat in ('A','B')},
        'component_loss_rows':[{k:v for k,v in r.items() if k!='certificate'} for r in rows if any(x<0 for x in r['delta'])],
        'first_tier_losses':[{k:v for k,v in r.items() if k!='certificate'} for r in rows if r['comparison_B_vs_A']=='loss'],
        'gains':[{k:v for k,v in r.items() if k!='certificate'} for r in rows if r['comparison_B_vs_A']=='win'],
        'same_binary_controls':report['same_binary_controls'],'robust_intervals':report['robust_intervals'],
        'inactive_trajectory_mismatches':report['independent5000_trajectory_mismatches'],
        'inactive_score_mismatches':report['independent5000_score_mismatches'],
        'work':{k:sum(x[k] for x in frames) for k in ('entered','takeover','deadline','failure','queries','settled','routes','evaluated','valid','certified')}}
    p=ROOT/'research/evidence'/(ID+'-holdout-audit.json')
    if p.exists():assert load(p)==audit
    else:
        with p.open('x',encoding='utf-8',newline='\n') as out:json.dump(audit,out,sort_keys=True,indent=2);out.write('\n')
    print('verified366holdout',json.dumps({k:audit[k] for k in ('files','results','fixtures','acks','transitions','active','inactive','work','first_tier_losses')}))
    print('audit_sha256',sha(p))

if __name__=='__main__':main()
