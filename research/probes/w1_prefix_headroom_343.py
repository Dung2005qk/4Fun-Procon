"""Read-only feasibility from completed DEVELOPMENT evidence; never run a bot."""
import argparse
from collections import defaultdict
import run_pair_score_341 as r

ID='ATTR-W1-COMPLETE-PREFIX-HEADROOM-343'
M=r.ROOT/f'research/holdouts/{ID}.json'
S=r.ROOT/f'research/evidence/{ID}.summary.json'


def headroom(deadline, timing):
    return max(0, deadline['totalMs']-deadline['networkMs']-
               min(deadline['certificationMs'],25)-timing['totalMs']-1)


def freeze():
    old=r.load(r.M);r.verify(old)
    paths=set(old['hashes'])|{str(r.M.relative_to(r.ROOT)).replace('\\','/'), old['development'],
        'research/probes/w1_prefix_headroom_343.py','research/probes/test_w1_prefix_headroom_343.py',
        'src/decision.cpp','include/udon/decision.hpp','src/runtime.cpp','src/btc_main.cpp',
        'research/evidence/ATTR-TARGET-FOLLOWUP-PREFIX-FEASIBILITY-303.md',
        'research/evidence/ATTR-ACK-W1-SUFFIX-CONSUMPTION-333-closure.md',
        'research/evidence/ATTR-INACTIVE-RUNTIME-AA-CONTROL-342-closure.md',
        'research/evidence/ATTR-INACTIVE-RUNTIME-AA-CONTROL-342.summary.json'}
    cases=r.load(r.ROOT/old['development'])['cases'];r.require(len(cases)==24,'all development cases required')
    for c in cases:
        seed=c['spec']['seed'];r.validate_side(c,'parent')
        paths.update(str(p.relative_to(r.ROOT)).replace('\\','/') for p in (r.D/'parent').glob(str(seed)+'.*'))
        paths.add(str((r.D/(str(seed)+'.pair_complete.json')).relative_to(r.ROOT)).replace('\\','/'))
    paths.add(str((r.D/'run_complete.json').relative_to(r.ROOT)).replace('\\','/'))
    r.write_new(M,{'experiment':ID,'development':old['development'],'cases':24,'days':96,
        'validation_floor_ms':25,'rounding_guard_ms':1,'existing_pricing_cap_ms':100,
        'hashes':{p:r.digest(r.ROOT/p) for p in sorted(paths)},
        'gate':'Source-complete consumer-preserving boundary and positive nonterminal selected-complete-W1 headroom in at least2families. Feasibility only, not score or speed authority.'})
    print('manifest_sha256='+r.digest(M))


def analyze():
    m=r.load(M);r.verify(m);cases=r.load(r.ROOT/m['development'])['cases'];rows=[]
    done=r.load(r.D/'run_complete.json')
    r.require(done['pairs']==24 and done['execution_sha256']==r.digest(r.M),'development not complete')
    for c in cases:
        r.validate_side(c,'parent');p=r.D/'parent'/str(c['spec']['seed'])
        r.require(r.load(r.D/(p.name+'.pair_complete.json'))['results']['parent']==r.digest(p.with_suffix('.result.json')),'parent pair hash')
        days=r.decisions(p.with_suffix('.replay.jsonl'));r.require(len(days)==4,'complete four-day evidence')
        for d in days:
            remaining=4-d['dayNumber'];outcomes=d['profile']['outcomes']
            full=bool(outcomes) and all(o['certified'] and not o['lowerBoundOnly'] and
                len(o['futurePlans'])==remaining for o in outcomes)
            h=headroom(d['deadline'],d['timing'])
            rows.append({'seed':c['spec']['seed'],'family':c['spec']['family'],'day':d['dayNumber'],
                'terminal':remaining==0,'selected_complete_witness':full,'conservative_headroom_ms':h,
                'main_total_ms':d['timing']['totalMs'],'certification_ms':d['timing']['certificationMs'],
                'potential_full_100ms':h>=100 and remaining>0 and full})
    r.require(len(rows)==96,'wrong day count')
    groups=defaultdict(list)
    for row in rows:groups[row['family']].append(row)
    stats={family:{'days':len(items),'nonterminal':sum(not d['terminal'] for d in items),
        'complete_nonterminal':sum(not d['terminal'] and d['selected_complete_witness'] for d in items),
        'positive_eligible':sum(not d['terminal'] and d['selected_complete_witness'] and d['conservative_headroom_ms']>0 for d in items),
        'full_100ms_fit':sum(d['potential_full_100ms'] for d in items),
        'min_nonterminal_headroom_ms':min(d['conservative_headroom_ms'] for d in items if not d['terminal']),
        'max_nonterminal_headroom_ms':max(d['conservative_headroom_ms'] for d in items if not d['terminal'])}
        for family,items in groups.items()}
    report={'experiment':ID,'rows':rows,'families':stats,'manifest_sha256':r.digest(M),
        'positive_families':sum(s['positive_eligible']>0 for s in stats.values()),
        'headroom_gate':sum(s['positive_eligible']>0 for s in stats.values())>=2,
        'authority':'Read-only development attribution. Lower estimate at an earlier W1 boundary under recorded runs, not guaranteed future wall time, no pricing throughput or promotion claim. Source-preservation proof and fresh SCORE still required.'}
    r.write_new(S,report);print({'families':stats,'headroom_gate':report['headroom_gate'],'summary_sha256':r.digest(S)})


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','analyze'));a=p.parse_args()
    freeze() if a.mode=='freeze' else analyze()
