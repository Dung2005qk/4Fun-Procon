"""Generate/hash unplayed splits before any341candidate edit. No solver calls."""
import json
from pathlib import Path
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, close_bridge
from run_http_baseline_314 import Bridge
from pair_pricing_prevalence_337 import gameplay_hash, FAMILIES

ID = 'SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341'


def main():
    require(not (ROOT/'artifacts/research/341/source').exists(), 'candidate already exists')
    s337 = ROOT/'research/evidence/ATTR-PAIR-STOCK-RELAXATION-BOUND-340.summary.json'
    require(digest(s337) == '96552297929C1B486A4225483CD03090E1E9BC74F7F57514CF8EB871FD699B15', '340summary drift')
    require(load(s337)['gate_passed'], '340 failed')
    old = load(ROOT/'research/holdouts/SCORE-W1-HORIZON-RESOURCE-PRICING-332.json')
    for p,h in old['parent_hashes'].items(): require(digest(ROOT/p)==h, 'parent drift:'+p)
    consumed = []
    for name in ('ATTR-THREE-PATROL-HTTP-BASELINE-314','ATTR-W1-HORIZON-PRICING-PREVALENCE-331','ATTR-W1-PAIR-PRICING-PREVALENCE-337'):
        consumed += load(ROOT/f'research/holdouts/{name}.json')['setups']
    consumed += [c['setup'] for c in load(ROOT/'research/holdouts/SCORE-W1-HORIZON-RESOURCE-PRICING-332-development.json')['cases']]
    consumed += [c['setup'] for c in load(ROOT/'research/holdouts/SCORE-W1-BOUNDED-PAIR-PRICING-338-development.json')['cases']]
    seen = {gameplay_hash(s) for s in consumed}
    splits = {}
    bridge = Bridge(ROOT/old['bridge_binary'])
    try:
        for phase,offset,count in (('development',0,4),('holdout',100000,9)):
            cases=[]; skipped=[]
            for f,family in enumerate(FAMILIES):
                r=0; cursor=0
                while r<count:
                    require(cursor<10000,'fresh pool exhausted')
                    spec={'seed':20260907341000+offset+10000*f+cursor,'family':family,'players':8+(f+r)%3}
                    cursor+=1
                    q=bridge.request({'op':'fixture',**spec}); require(q.get('ok'),'fixture rejected')
                    h=gameplay_hash(q['setup'])
                    if h in seen:
                        skipped.append(spec['seed']); continue
                    seen.add(h)
                    cases.append({'spec':spec,'setup':q['setup'],'gameplay_sha256':h,
                        'order':['parent','candidate'] if (f+r)%2==0 else ['candidate','parent']})
                    r+=1
            path=ROOT/f'research/holdouts/{ID}-{phase}.json'
            write_new(path,{'experiment':ID,'phase':phase,'cases':cases,'unplayed_identity_skips':skipped,
                'authority':'Actual HTTP synthetic roadless fixed3Patrol causal screen, not official BTC or traffic authority.'})
            splits[phase]={'path':str(path.relative_to(ROOT)).replace('\\','/'),'sha256':digest(path),'pairs':len(cases)}
    finally: close_bridge(bridge)
    p=Bridge(ROOT/'artifacts/research/332/protected-fixture.exe'); protected=[]
    try:
        for f in range(6):
            for fi,fuel in enumerate(('low','default','high')):
                for si,side in enumerate((8,32)):
                    for wi,window in enumerate((5000,10000,15000)):
                        n=len(protected); base=202609073410000
                        q={'seed':base-base%6+6*n+f,'side':side,'fuel':fuel,'window_ms':window,
                            'days':(4,5,10)[(f+fi+wi)%3],'players':8+(f+wi)%3,'roadless':(f+fi+si)%2==0}
                        r=p.request(q); require(r.get('ok'),'protected fixture rejected')
                        protected.append({**q,**r,'role':'fixed-all-Patrol' if (f+wi)%2==0 else 'native',
                            'order':['parent','candidate'] if n%2==0 else ['candidate','parent']})
    finally: close_bridge(p)
    require(len(protected)==108,'protected count')
    pp=ROOT/f'research/holdouts/{ID}-protected.json'
    write_new(pp,{'experiment':ID,'pairs':108,'cases':protected,
        'traffic':'Exact own plus frozen aggregate external footprints over two prior days, divided by players; day1smooth.',
        'gate':old['protected_gates'],
        'execution_contract':'Actual MatchSession/HTTP lifecycle, fixed/native assignment and authoritative windows; dedicated runner must pass contracts before measurement.'})
    splits['protected']={'path':str(pp.relative_to(ROOT)).replace('\\','/'),'sha256':digest(pp),'pairs':108}
    paths=set(old['parent_hashes'])|{'research/probes/freeze_pair_score_341.py','research/probes/pair_pricing_prevalence_337.py',
        'research/probes/protected_fixture_332.cpp','artifacts/research/332/protected-fixture.exe',
        str(s337.relative_to(ROOT)).replace('\\','/'),
        'research/evidence/ATTR-PAIR-STOCK-RELAXATION-BOUND-340-closure.md',
        'research/evidence/ATTR-PAIR-STOCK-RELAXATION-BOUND-340-crosscheck.json',
        'artifacts/research/340/horizon_pricing.cpp'}
    m={'experiment':ID,'parent_commit':old['parent_commit'],'splits':splits,
        'parent_btc_binary':old['parent_btc_binary'],'bridge_binary':old['bridge_binary'],
        'parent_hashes':{p:digest(ROOT/p) for p in sorted(paths)},'limits_per_witness':old['limits_per_witness'],
        'development_gate':old['development_gate'],'holdout_gate':old['holdout_gate'],'protected_gates':old['protected_gates'],
        'mechanism':'Every unordered pair from original full W1; canonical resource frontiers; joint stock-capped reward; physical pair cell/fuel/lifetime DP; all other whole trajectories fixed. Sound independent residual-stock upper bound and terminal stable mask quotient; no individually losing route pruning.',
        'pair_order':'Physical index lexicographic; shared original-root day cache and all invocation counters; memo cleared per pair; any incomplete pair discards entire replacement.',
        'budget_provenance':'Unchanged332limits; not sized from336337observed work. Full-team dual validation precharged, action/memo/query/label allocation bounds and deadline checks.',
        'resource_floor_bytes':1073741824,'stage':'All inputs frozen before source; execution not authorized until candidate and tests frozen',
        'production_change':False,'holdout_opened':False}
    path=ROOT/f'research/holdouts/{ID}.json'; write_new(path,m)
    print(json.dumps({'manifest_sha256':digest(path),'splits':splits},indent=2))


if __name__=='__main__': main()
