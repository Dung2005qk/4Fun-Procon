"""Freeze fresh roaded A/A controls without calling either planner."""
from inactive_runtime_aa_342 import ROOT, ID, M, Bridge, load, digest, digest_case, write_new, require, verify, order_for, coverage


def main():
    require(not M.exists(), 'manifest already frozen')
    old_path = ROOT/'research/holdouts/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-protected-execution.json'
    old = load(old_path); verify(old)
    paths = set(old['hashes']) | {str(old_path.relative_to(ROOT)).replace('\\','/'),
        'research/probes/inactive_runtime_aa_342.py', 'research/probes/freeze_inactive_runtime_342.py',
        'research/probes/test_inactive_runtime_342.py',
        f'research/evidence/{ID}-preflight.json',
        'research/evidence/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-protected-closure.md'}
    seen = set()
    for phase in ('development','holdout','protected'):
        p = ROOT/f'research/holdouts/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-{phase}.json'
        paths.add(str(p.relative_to(ROOT)).replace('\\','/'))
        for c in load(p)['cases']:
            setup = dict(c['setup']); setup.pop('startsAt', None)
            seen.add(digest_case(setup))
    fixture_binary = 'artifacts/research/332/protected-fixture.exe'
    require(fixture_binary in old['hashes'], 'fixture provenance missing')
    bridge = Bridge(ROOT/fixture_binary); cases=[]
    try:
        for size in (8,32):
            for role in ('fixed-all-Patrol','native'):
                for fuel in ('low','default'):
                    i=len(cases); base=202609073420000
                    q={'seed':base-base%6+6*i+i%6,'side':size,'fuel':fuel,'window_ms':5000,
                       'days':4,'players':8+i%3,'roadless':False}
                    r=bridge.request(q); require(r.get('ok'),'fixture generation failure')
                    identity=dict(r['setup']); identity.pop('startsAt',None); h=digest_case(identity)
                    require(h not in seen,'nonfresh fixture; stop before measurement, no result-conditioned replacement')
                    seen.add(h)
                    cases.append({**q,**r,'role':role,'order':['parent','candidate'],
                                  'run_order':order_for(i),'gameplay_sha256':h})
    finally:
        bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    counts=coverage(cases)
    preflight=load(ROOT/f'research/evidence/{ID}-preflight.json')
    require(preflight['passed'] and preflight['tests']>=8,'contract preflight failed')
    m={'experiment':ID,'parent_commit':'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c',**counts,'cases':cases,
       'parent_binary':old['parent_binary'],'candidate_binary':old['candidate_binary'],
       'bridge_binary':old['bridge_binary'],'resource_floor_bytes':1073741824,
       'hashes':{p:digest(ROOT/p) for p in sorted(paths)},
       'authority':'No holdout or promotion authority; fresh same-binary repeatability control; never-supported pricing only.',
       'gate':'Any within-binary role/plan/state/ledger/road difference falsifies exact repeatability on sampled runtime. Zero observed is not proof of absence. No change to341 rejection.',
       'forbidden':'No partial comparison, load blame, cap/source change, parallel compute, consumed-case retune, or ambiguous-side replay.'}
    write_new(M,m)
    print({'manifest_sha256':digest(M),'dependencies':len(m['hashes']),**counts})


if __name__=='__main__': main()
