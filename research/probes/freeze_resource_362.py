"""Create-only fresh362 grids for the ALREADY frozen360 plain candidate."""
from pathlib import Path
from refinement_cost_353 import ROOT,sha,load,write,require
from run_http_baseline_314 import Bridge
from freeze_complete_pool_344 import order_for
from pair_pricing_prevalence_337 import gameplay_hash
from verify_late_control_360_copy import verify as verify360
from verify_resume_control_361_copy import verify as verify361

ID='SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362'

def main():
    verify360();verify361()
    require(not (ROOT/f'research/holdouts/{ID}.json').exists(),'never replace frozen inputs')
    seen=set()
    for name in ('SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350','SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347',
                 'SCORE-COMPLETE-W1-POOL-PAIR-PRICING-344','SCORE-PROTECTED-RESOURCE-MARGINAL-357'):
        for phase in ('development','broad-development','holdout','protected'):
            p=ROOT/f'research/holdouts/{name}-{phase}.json'
            # Identity extraction only; never emit/measure existing sealed setups.
            if p.exists():seen.update(c['gameplay_sha256']for c in load(p)['cases'])
    exclusions=len(seen);splits={}
    bridge=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe')
    try:
        for phase,offset in [('development',0),('holdout',100000),('protected',200000)]:
            cases=[];skips=[]
            for f in range(6):
                for fi,fuel in enumerate(('low','default','high')):
                    cells=[(s,w)for s in range(2)for w in range(3)] if phase=='protected' else ([(s,(f+fi+s)%3)for s in range(2)] if phase=='development' else [((f+fi+w)%2,w)for w in range(3)])
                    for si,wi in cells:
                        i=len(cases);cursor=0
                        while True:
                            base=202609093620000+offset;base-=base%6
                            q={'seed':base+6*i+f+cursor*6000,'side':(8,32)[si],'fuel':fuel,
                               'window_ms':(5000,10000,15000)[wi],'days':(4,5,10)[(f+fi+wi)%3],
                               'players':8+(f+wi)%3,'roadless':(f+fi+si)%2==0};cursor+=1
                            require(cursor<10000,'identity exhaustion');r=bridge.request(q);require(r.get('ok'),'fixture validation')
                            h=gameplay_hash(r['setup'])
                            if h not in seen:seen.add(h);break
                            skips.append(q['seed'])
                        cases.append({**q,**r,'gameplay_sha256':h,'role':'fixed-all-Patrol'if(f+wi)%2==0 else'native',
                            'order':['parent','candidate'],'run_order':order_for(i)})
            require(len(cases)=={'development':36,'holdout':54,'protected':108}[phase],'coverage')
            p=ROOT/f'research/holdouts/{ID}-{phase}.json'
            write(p,{'experiment':ID,'phase':phase,'cases':cases,'repetitions':2,'identity_skips':skips})
            splits[phase]={'path':p.relative_to(ROOT).as_posix(),'sha256':sha(p),'fixtures':len(cases),
                'actions':4*sum(c['days']for c in cases),'minimum_wire_seconds':4*sum(sum(c['setup']['daySeconds'])for c in cases)}
    finally:bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    paths=['research/probes/freeze_resource_362.py','research/probes/score_resource_362.py',
        'research/probes/verify_late_control_360_copy.py','research/probes/verify_resume_control_361_copy.py',
        'artifacts/research/332/protected-fixture.exe',f'research/evidence/{ID}-preregistration.md']
    manifest={'experiment':ID,'parent_commit':'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c',
        'candidate_sha256':'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678',
        'hashes':{p:sha(ROOT/p)for p in paths},'splits':splits,'excluded_identity_count':exclusions,
        'sealed_opened':False,'candidate_source_change':False,'authority':'prospective362 protocol; never reclassify357'}
    write(ROOT/f'research/holdouts/{ID}.json',manifest)
    print('input362_frozen',sha(ROOT/f'research/holdouts/{ID}.json'),splits)

if __name__=='__main__':main()
