"""Create-only366 frozen grids and DEV-only upload; no experiment scores read."""
from pathlib import Path
import json
import shutil
import tarfile
from refinement_cost_353 import ROOT,sha,load,write,require
from run_http_baseline_314 import Bridge
from freeze_complete_pool_344 import order_for
from pair_pricing_prevalence_337 import gameplay_hash
from verify_late_control_360_copy import verify as verify360
from verify_resume_control_361_copy import verify as verify361
from verify_gain_short_window_365_copy import verify as verify365

ID='SCORE-CAUSAL-RESOURCE-QUALIFICATION-366'

def main():
    verify360();verify361();verify365()
    manifest=ROOT/f'research/holdouts/{ID}.json'
    work=ROOT/'artifacts/research/366/stage'
    require(not manifest.exists() and not work.exists(),'already frozen or staged')
    seen=set()
    for name in ('SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350','SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347',
                 'SCORE-COMPLETE-W1-POOL-PAIR-PRICING-344','SCORE-PROTECTED-RESOURCE-MARGINAL-357',
                 'SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362'):
        for phase in ('development','broad-development','holdout','protected'):
            p=ROOT/f'research/holdouts/{name}-{phase}.json'
            # Identity exclusion only: never emit or measure still-sealed prior setups.
            if p.exists():seen.update(c['gameplay_sha256'] for c in load(p)['cases'])
    exclusions=len(seen);splits={};bridge=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe')
    try:
        for phase,offset in [('development',0),('holdout',100000),('protected',200000)]:
            cases=[];skips=[]
            for family in range(6):
                for fi,fuel in enumerate(('low','default','high')):
                    grid=[(s,w) for s in range(2) for w in range(3)] if phase=='protected' else (
                        [(s,(family+fi+s)%3) for s in range(2)] if phase=='development' else
                        [((family+fi+w)%2,w) for w in range(3)])
                    for si,wi in grid:
                        i=len(cases);cursor=0
                        while True:
                            base=202609093660000+offset;base-=base%6
                            q={'seed':base+6*i+family+cursor*6000,'side':(8,32)[si],'fuel':fuel,
                               'window_ms':(5000,10000,15000)[wi],'days':(4,5,10)[(family+fi+wi)%3],
                               'players':8+(family+wi)%3,'roadless':(family+fi+si)%2==0};cursor+=1
                            require(cursor<10000,'identity exhaustion')
                            r=bridge.request(q);require(r.get('ok'),'fixture validation')
                            h=gameplay_hash(r['setup'])
                            if h not in seen:seen.add(h);break
                            skips.append(q['seed'])
                        cases.append({**q,**r,'gameplay_sha256':h,
                            'role':'fixed-all-Patrol' if (family+wi)%2==0 else 'native',
                            'order':['parent','candidate'],'run_order':order_for(i)})
            require(len(cases)=={'development':36,'holdout':54,'protected':108}[phase],'coverage')
            p=ROOT/f'research/holdouts/{ID}-{phase}.json'
            write(p,{'experiment':ID,'phase':phase,'cases':cases,'repetitions':2,'identity_skips':skips})
            splits[phase]={'path':p.relative_to(ROOT).as_posix(),'sha256':sha(p),'fixtures':len(cases),
                'actions':4*sum(c['days'] for c in cases),
                'minimum_wire_seconds':4*sum(sum(c['setup']['daySeconds']) for c in cases)}
    finally:bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    paths=['research/probes/prepare_resource_366.py','research/probes/score_resource_366.py',
           'research/probes/gate_resource_366.py','research/probes/test_gate_resource_366.py',
           'research/probes/launch_resource_366.py','research/probes/verify_late_control_360_copy.py',
           'artifacts/research/332/protected-fixture.exe',f'research/evidence/{ID}-preregistration.md']
    write(manifest,{'experiment':ID,'parent_commit':'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c',
        'candidate_sha256':'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678',
        'hashes':{p:sha(ROOT/p) for p in paths},'splits':splits,'excluded_identity_count':exclusions,
        'sealed_opened':False,'candidate_source_change':False})
    sources={'input366.json':manifest,'preregistration.md':ROOT/f'research/evidence/{ID}-preregistration.md',
             splits['development']['path']:ROOT/splits['development']['path']}
    for name in ('score_resource_366','gate_resource_366','test_gate_resource_366','launch_resource_366','verify_late_control_360_copy'):
        sources[name+'.py']=ROOT/'research/probes'/(name+'.py')
    for relative,source in sources.items():
        target=work/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target);require(sha(target)==sha(source),'copy drift')
    write(work/'stage366.json',{'hashes':{p:sha(work/p) for p in sources},'no_sealed_setup_exported':True})
    archive=work.parent/'stage.tar.gz'
    with tarfile.open(archive,'x:gz') as tar:
        for p in sorted(work.rglob('*')):
            if p.is_file():tar.add(p,arcname=p.relative_to(work).as_posix())
    print(json.dumps({'input_sha256':sha(manifest),'stage_sha256':sha(work/'stage366.json'),
                     'archive_sha256':sha(archive),'excluded':exclusions,'splits':splits},indent=2))

if __name__=='__main__':main()
