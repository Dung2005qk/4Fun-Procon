"""Create-only fresh broad grids before any candidate source changes."""
from refinement_cost_353 import ROOT,sha,load,write,require
from run_http_baseline_314 import Bridge
from freeze_complete_pool_344 import order_for
from pair_pricing_prevalence_337 import gameplay_hash
ID='SCORE-PROTECTED-RESOURCE-MARGINAL-357'
def main():
    require(not (ROOT/'artifacts/research/357/source').exists(),'source must follow input freeze')
    seen=set();paths=['research/probes/freeze_resource_357.py','research/probes/protected_fixture_332.cpp',
        'artifacts/research/332/protected-fixture.exe',f'research/evidence/{ID}-preregistration.md',
        'research/evidence/ATTR-RESOURCE-CONDITIONED-MARGINAL-CAPABILITY-356-closure.md']
    # Hash identities only; no unopened setup/score is inspected or exported.
    for name in ('SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350','SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347','SCORE-COMPLETE-W1-POOL-PAIR-PRICING-344'):
        for phase in ('development','broad-development','holdout','protected'):
            p=ROOT/f'research/holdouts/{name}-{phase}.json'
            if p.exists():seen.update(c['gameplay_sha256']for c in load(p)['cases']);paths.append(p.relative_to(ROOT).as_posix())
    splits={};bridge=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe')
    try:
        for phase,offset in [('development',0),('holdout',100000),('protected',200000)]:
            cases=[];skips=[]
            for f in range(6):
                for fi,fuel in enumerate(('low','default','high')):
                    cells=[(s,w)for s in range(2)for w in range(3)] if phase=='protected' else ([(s,(f+fi+s)%3)for s in range(2)] if phase=='development' else [((f+fi+w)%2,w)for w in range(3)])
                    for si,wi in cells:
                        i=len(cases);cursor=0
                        while True:
                            base=202609093570000+offset;base-=base%6
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
            p=ROOT/f'research/holdouts/{ID}-{phase}.json';write(p,{'experiment':ID,'phase':phase,'cases':cases,'repetitions':2,'identity_skips':skips})
            splits[phase]={'path':p.relative_to(ROOT).as_posix(),'sha256':sha(p),'fixtures':len(cases),
                'actions':4*sum(c['days']for c in cases),'minimum_wire_seconds':4*sum(sum(c['setup']['daySeconds'])for c in cases)}
    finally:bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    p=ROOT/f'research/holdouts/{ID}.json';write(p,{'experiment':ID,'parent_commit':'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c',
        'hashes':{x:sha(ROOT/x)for x in paths},'splits':splits,'sealed_opened':False,'source_not_created':True})
    print('357 inputs frozen',sha(p),splits)
if __name__=='__main__':main()
