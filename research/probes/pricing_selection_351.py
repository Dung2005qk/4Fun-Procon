"""Frozen complete-development attribution; never invokes a solver or submits plans."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import anytime_score_350 as source

ROOT=source.ROOT
ID='ATTR-PRICING-SELECTION-LEVERAGE-351'
M=ROOT/f'research/evidence/{ID}.manifest.json'
S=ROOT/f'research/evidence/{ID}.summary.json'
load,digest,require,write_new,verify=(source.core.load,source.core.digest,
    source.core.require,source.core.write_new,source.core.verify)
KEYS=('lifetimeDistinct','totalDailyDistinct','totalServings')
PINNED={'development':'0A42A53907E5971D9C0A41F970430519612A04679D6776677F5E75B74F981C8C',
    'broad-development':'FC6605640E6CEE092943B6EA1D7EDAFA16EF73915074D6F76609EA65718A9199'}

def score(s):return tuple(s[k] for k in KEYS)
def rel(p):return Path(p).relative_to(ROOT).as_posix()

def dominates(lower,upper,weights):
    """Exact source survival comparison, not a weighted score sum."""
    require(len(lower)==len(upper)==len(weights)>0,'support sizes')
    require(all(type(w)is int and w>=0 for w in weights) and sum(weights)>0,'weights')
    strict=False
    for threshold in sorted(set(lower+upper)):
        a=sum(w for x,w in zip(lower,weights,strict=True) if x>=threshold)
        b=sum(w for x,w in zip(upper,weights,strict=True) if x>=threshold)
        if a<b:return False
        strict |= a>b
    return strict

def witness_bytes(w):
    result=f"{int(w['certified'])}:{int(w['lowerBoundOnly'])}:"
    for plan in w['futurePlans']:
        p=json.dumps(plan,separators=(',',':'))
        result+=str(len(p))+':'+p
    return result

def selected_identity(record,w):
    return (record['afterWitness']==witness_bytes(w) and
        record['afterScore']==w['score']==w['witnessScore'])

def inspect_day(day,next_day=None):
    audit=source.core.check_causal(day)
    selected=day['candidate']['stableId']; profile=day['profile']
    row={'day':day['dayNumber'],'entered':audit['entered'],
        'current':score(day['candidate']['scoreAfterToday']),
        'selected_lower':score(profile['certifiedLowerBound']),
        'selected_upper':score(profile['validUpperBound']),
        'upper_valid':profile['hasValidUpperBound'],
        'base_selected':audit['baseSelectedId'],'final_selected':audit['finalSelectedId'],
        'selection_changed':audit['entered'] and audit['baseSelectedId']!=audit['finalSelectedId'],
        'upgrades':[],'selected_witness_identity_failures':0,
        'ignored_strict_dominance':0,'all_candidate_work':dict(sum(
            (Counter(c['horizonPricing']) for c in day['audit']['candidates']),Counter()))}
    if not audit['entered']:return row
    candidates={c['stableId']:c for c in day['audit']['candidates']}
    grouped=defaultdict(list)
    for outcome in audit['outcomes']:grouped[outcome['candidateId']].append(outcome)
    weights=profile['scenarioWeights']; uppers=list(map(score,profile['scenarioValidUpperBounds']))
    require(weights==[s['weight'] for s in day['manifest']['scenarios']],'scenario weights')
    for cid,outcomes in sorted(grouped.items()):
        if not any(x['replaced'] for x in outcomes):continue
        outcomes=sorted(outcomes,key=lambda x:x['scenarioIndex'])
        require([x['scenarioIndex'] for x in outcomes]==list(range(len(weights))),'outcome coverage')
        ca=candidates[cid]
        after=list(map(lambda x:score(x['afterScore']),outcomes))
        complete=all(x['afterWitness'].startswith('1:0:') for x in outcomes)
        # This exactly follows the source predicate on the common support.
        strict=(complete and profile['hasValidUpperBound'] and dominates(after,uppers,weights))
        identities=True
        if cid==selected:
            identities=all(selected_identity(x,profile['outcomes'][x['scenarioIndex']]) for x in outcomes)
            row['selected_witness_identity_failures']+=not identities
        ignored=cid!=selected and strict
        row['ignored_strict_dominance']+=ignored
        lower=score(ca['finalCertifiedLowerBound'])
        row['upgrades'].append({'candidate_id':cid,'selected':cid==selected,
            'disposition':ca['disposition'],'current':score(ca['scoreAfterToday']),
            'current_below_selected':score(ca['scoreAfterToday'])<row['current'],
            'final_lower':lower,'lower_above_selected':lower>row['selected_lower'],
            'valid_upper':score(ca['validUpperBound']),
            'strictly_dominates_selected_upper':strict,'complete_witnesses':complete,
            'selected_witness_identity':identities,'work':ca['horizonPricing'],
            'outcomes':outcomes,
            'first_suffix_matches_next_decision':None if cid!=selected or next_day is None else
                any(w['futurePlans'] and w['futurePlans'][0]==next_day['candidate']['plan']
                    for w in profile['outcomes'])})
    return row

def freeze():
    require(not M.exists(),'already frozen')
    source.configure(); execution=load(source.M);verify(execution)
    require(digest(source.M)=='A60CC5C0E4438D50BC7ABD50444E7EC9BF5F7AEFD095E3B1B87192B029A53B7B','350 execution')
    paths={source.M,Path(__file__),ROOT/'research/probes/test_pricing_selection_351.py',
        ROOT/f'research/evidence/{ID}-preregistration.md',
        ROOT/'research/evidence/SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350-broad-development-closure.md'}
    references=[]
    for phase,h in PINNED.items():
        p=ROOT/f'research/evidence/{source.ID}-{phase}.summary.json'
        require(digest(p)==h,'350 summary drift');report=load(p)
        d=ROOT/f'research/evidence/{source.ID}-{phase}';cases=load(ROOT/execution['splits'][phase]['path'])['cases']
        source.core.audit(cases,d,phase)
        require(report['complete'] and digest(d/'run_complete.json')==report['completion_sha256'],'complete phase')
        paths|={p,d/'run_complete.json'}
        rows={(x['seed'],x['repeat']):x for x in report['rows']}
        for c in cases:
            paths.add(d/f'{source.core.seed(c)}.fixture_complete.json')
            for label in source.core.LABELS:
                loc=source.core.location(c,label,d)
                paths.update(loc.with_suffix(s) for s in source.core.SUFFIXES)
            for repeat in ('A','B'):
                r=rows[source.core.seed(c),repeat]
                loc=source.core.location(c,'candidate'+repeat,d)
                references.append({'phase':phase,'repeat':repeat,
                    **{k:r[k] for k in ('seed','family','fuel','side','role','days','players','window_ms','roadless','agent_count','brand_count','stock_vector','step_vector')},
                    'replay':rel(loc.with_suffix('.replay.jsonl'))})
    require(len(references)==120 and sum(r['days'] for r in references)==648,'expected coverage')
    hashes={**execution['hashes'],**{rel(p):digest(p) for p in paths}}
    write_new(M,{'experiment':ID,'references':references,'hashes':hashes,'matches':120,'decision_days':648})
    print({'manifest_sha256':digest(M),'dependencies':len(hashes)})

def analyze(check=False):
    source.configure();m=load(M);verify(m);rows=[]
    for ref in m['references']:
        days=source.core.decisions(ROOT/ref['replay'])
        require(len(days)==ref['days'],'day coverage')
        for i,day in enumerate(days):
            rows.append({**ref,**inspect_day(day,days[i+1] if i+1<len(days) else None)})
    require(len(rows)==m['decision_days'],'exact decision coverage')
    def totals(rr):
        upgrades=[u for r in rr for u in r['upgrades']]
        return {'days':len(rr),'entered':sum(r['entered'] for r in rr),
            'selection_changes':sum(r['selection_changed'] for r in rr),
            'upgraded_candidates':len(upgrades),'selected_upgrades':sum(u['selected'] for u in upgrades),
            'unselected_better_lower':sum(not u['selected'] and u['lower_above_selected'] for u in upgrades),
            'unselected_better_lower_current_conflict':sum(not u['selected'] and u['lower_above_selected'] and u['current_below_selected'] for u in upgrades),
            'selected_witness_identity_failures':sum(r['selected_witness_identity_failures'] for r in rr),
            'ignored_strict_dominance':sum(r['ignored_strict_dominance'] for r in rr),
            'work':dict(sum((Counter(r['all_candidate_work']) for r in rr),Counter()))}
    phases={p:totals([r for r in rows if r['phase']==p]) for p in PINNED}
    violations=[r for r in rows if r['selected_witness_identity_failures'] or r['ignored_strict_dominance']]
    report={'experiment':ID,'complete':True,'manifest_sha256':digest(M),'matches':120,
        'decision_days':len(rows),'totals':totals(rows),'phases':phases,
        'consumer_defect_found':bool(violations),'violation_roots':sorted({r['seed'] for r in violations}),
        'strata':{k:{str(v):totals([r for r in rows if r[k]==v]) for v in sorted({r[k] for r in rows})}
            for k in ('phase','family','fuel','side','role','days','players','window_ms','roadless')},
        'rows':rows,'authority':'Completed DEVELOPMENT read-only consumer attribution, not promotion or performance.'}
    verify(m);report=json.loads(json.dumps(report))
    if check:require(load(S)==report,'recomputation drift')
    else:write_new(S,report)
    print({'summary_sha256':digest(S),'consumer_defect_found':report['consumer_defect_found'],'phases':phases})

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','analyze'));p.add_argument('--check',action='store_true');a=p.parse_args()
    freeze() if a.mode=='freeze' else analyze(a.check)
