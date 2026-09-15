"""Create-only fresh inputs; sealed historical inputs contribute identity hashes only."""
from pathlib import Path
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, close_bridge
from run_http_baseline_314 import Bridge
from pair_pricing_prevalence_337 import gameplay_hash, FAMILIES
from freeze_complete_pool_344 import order_for

ID='SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350'

def main():
    require(not (ROOT/'artifacts/research/350/source').exists(),'source already exists')
    base=load(ROOT/'research/holdouts/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341.json')
    for p,h in base['parent_hashes'].items():require(digest(ROOT/p)==h,'baseline drift:'+p)
    seen=set();paths=set(base['parent_hashes'])
    names=['ATTR-THREE-PATROL-HTTP-BASELINE-314','ATTR-W1-HORIZON-PRICING-PREVALENCE-331',
        'ATTR-W1-PAIR-PRICING-PREVALENCE-337','SCORE-W1-HORIZON-RESOURCE-PRICING-332-development',
        'SCORE-W1-BOUNDED-PAIR-PRICING-338-development','ATTR-INACTIVE-RUNTIME-AA-CONTROL-342']
    names += ['SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-'+p for p in ('development','holdout','protected')]
    for name in names:
        p=f'research/holdouts/{name}.json';o=load(ROOT/p);paths.add(p)
        setups=o['setups'] if 'setups' in o else [c['setup'] for c in o['cases']]
        seen.update(gameplay_hash(s) for s in setups)
    # Only precomputed identity fields, never the unopened344 setup/score fields.
    for phase in ('development','holdout','protected'):
        p347=f'research/holdouts/SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347-{phase}.json'
        seen.update(c['gameplay_sha256'] for c in load(ROOT/p347)['cases']);paths.add(p347)
        p=f'research/holdouts/SCORE-COMPLETE-W1-POOL-PAIR-PRICING-344-{phase}.json'
        seen.update(c['gameplay_sha256'] for c in load(ROOT/p)['cases']);paths.add(p)
    paths.update({'research/probes/freeze_anytime_350.py','research/probes/freeze_complete_pool_344.py',
        f'research/evidence/{ID}-preregistration.md','research/evidence/ATTR-ROOT-INCUMBENT-CERTIFICATE-349-closure.md',
        'artifacts/research/332/protected-fixture.exe'})
    paths.add('artifacts/research/341/protected-bridge.exe')
    splits={};bridge=Bridge(ROOT/base['bridge_binary'])
    try:
        for phase,offset,count in (('development',0,4),('holdout',100000,9)):
            cases=[];skips=[]
            for f,family in enumerate(FAMILIES):
                r=0;cursor=0
                while r<count:
                    require(cursor<10000,'identity pool exhausted')
                    spec={'seed':20260908350000+offset+10000*f+cursor,'family':family,'players':8+(f+r)%3};cursor+=1
                    q=bridge.request({'op':'fixture',**spec});require(q.get('ok'),'invalid fixture')
                    h=gameplay_hash(q['setup'])
                    if h in seen:skips.append(spec['seed']);continue
                    seen.add(h);cases.append({'spec':spec,'setup':q['setup'],'gameplay_sha256':h,
                        'order':['parent','candidate'],'run_order':order_for(len(cases))});r+=1
            p=ROOT/f'research/holdouts/{ID}-{phase}.json'
            write_new(p,{'experiment':ID,'phase':phase,'cases':cases,'identity_skips':skips,'repetitions':2,
                'authority':'Synthetic actual WinHTTP, not official BTC.'})
            splits[phase]={'path':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':digest(p),'fixtures':len(cases)}
    finally:close_bridge(bridge)
    bridge=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe');cases=[];skips=[]
    try:
        for f in range(6):
            for fi,fuel in enumerate(('low','default','high')):
                for si,side in enumerate((8,32)):
                    n=len(cases);cursor=0
                    while True:
                        require(cursor<10000,'broad development identities exhausted')
                        base_seed=2026090835090000
                        q={'seed':base_seed-base_seed%6+6*n+f+cursor*6000,'side':side,'fuel':fuel,
                            'window_ms':(5000,10000,15000)[(f+fi+si)%3],
                            'days':(4,5,10)[(f+fi+si)%3],'players':8+(f+fi+si)%3,
                            'roadless':(f+fi+si)%2==0};cursor+=1
                        result=bridge.request(q);require(result.get('ok'),'invalid broad development')
                        h=gameplay_hash(result['setup'])
                        if h in seen:skips.append(q['seed']);continue
                        seen.add(h);break
                    cases.append({**q,**result,'gameplay_sha256':h,
                        'role':'fixed-all-Patrol' if (f+si)%2==0 else 'native',
                        'order':['parent','candidate'],'run_order':order_for(n)})
    finally:close_bridge(bridge)
    p=ROOT/f'research/holdouts/{ID}-broad-development.json'
    write_new(p,{'experiment':ID,'phase':'broad-development','cases':cases,'repetitions':2,'identity_skips':skips})
    splits['broad-development']={'path':str(p.relative_to(ROOT)).replace('\\\\','/'),'sha256':digest(p),'fixtures':len(cases)}
    bridge=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe');cases=[];skips=[]
    try:
        for f in range(6):
            for fi,fuel in enumerate(('low','default','high')):
                for si,side in enumerate((8,32)):
                    for wi,window in enumerate((5000,10000,15000)):
                        n=len(cases);cursor=0
                        while True:
                            require(cursor<10000,'protected identities exhausted');base_seed=202609083500000
                            q={'seed':base_seed-base_seed%6+6*n+f+cursor*6000,'side':side,'fuel':fuel,'window_ms':window,
                                'days':(4,5,10)[(f+fi+wi)%3],'players':8+(f+wi)%3,'roadless':(f+fi+si)%2==0};cursor+=1
                            result=bridge.request(q);require(result.get('ok'),'invalid protected')
                            h=gameplay_hash(result['setup'])
                            if h in seen:skips.append(q['seed']);continue
                            seen.add(h);break
                        cases.append({**q,**result,'gameplay_sha256':h,'role':'fixed-all-Patrol' if (f+wi)%2==0 else 'native',
                            'order':['parent','candidate'],'run_order':order_for(n)})
    finally:close_bridge(bridge)
    require(len(cases)==108 and sum(c['days'] for c in cases)==684,'protected coverage')
    p=ROOT/f'research/holdouts/{ID}-protected.json'
    write_new(p,{'experiment':ID,'phase':'protected','cases':cases,'repetitions':2,'identity_skips':skips})
    splits['protected']={'path':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':digest(p),'fixtures':len(cases)}
    m={'experiment':ID,'parent_commit':'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c','parent_btc_binary':base['parent_btc_binary'],
        'bridge_binary':'artifacts/research/341/protected-bridge.exe','splits':splits,'limits_per_witness':base['limits_per_witness'],
        'policy':f'research/evidence/{ID}-preregistration.md','hashes':{p:digest(ROOT/p) for p in sorted(paths)},
        'stage':'Inputs frozen before source; no timing until execution/tests frozen','sealed_opened':False,'production_change':False}
    p=ROOT/f'research/holdouts/{ID}.json';write_new(p,m);print({'manifest_sha256':digest(p),'splits':splits})

if __name__=='__main__':main()
