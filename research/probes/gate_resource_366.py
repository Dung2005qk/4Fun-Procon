"""Prospective366 classifier. No solver, filesystem or historical-score inputs."""
from collections import Counter
import math

STRATA=('family','fuel','side','window_ms','days','players','roadless','role')

def sign_p(positive,negative):
    n=positive+negative
    return sum(math.comb(n,k) for k in range(positive,n+1))/2**n if n else 1.0

def inactive_gate(rows):
    ids=sorted({r['seed'] for r in rows}); details=[]; tails=True; upper=True
    for seed in ids:
        rr=sorted((r for r in rows if r['seed']==seed),key=lambda r:r['repeat'])
        assert [r['repeat'] for r in rr]==['A','B']
        spread=max(abs(rr[0][side][2]-rr[1][side][2]) for side in ('A','B'))
        values=[]; limits=[]
        for r in rr:
            allowance=max(1,r['A'][2]/50)
            limit=max(allowance,spread)
            tails &= r['delta'][2]>=-limit
            upper &= r['delta'][0]>=0 and r['delta'][1]>=0
            values.append(r['delta'][2]/allowance);limits.append(limit)
        details.append({'seed':seed,'normalized_mean':sum(values)/2,
                        'same_fixture_aa_spread':spread,'tail_limits':limits})
    values=[d['normalized_mean'] for d in details]
    above=sum(x>-1 for x in values);below=sum(x<-1 for x in values)
    negative=sum(x<0 for x in values);positive=sum(x>0 for x in values)
    ni=sign_p(above,below);harm=sign_p(negative,positive)
    checks={'inactive_fixture_count':len(ids)>=12,'inactive_upper_tiers':upper,
            'inactive_same_fixture_tail':tails,'inactive_noninferiority':ni<=0.05,
            'inactive_no_detected_negative_shift':harm>0.05}
    return {'checks':checks,'noninferiority_p':ni,'negative_shift_p':harm,
            'fixtures':details,'passed':all(checks.values())}

def aggregate(rows):
    return {'delta':[sum(r['delta'][k] for r in rows) for k in range(3)],
            'wtl':Counter(r['comparison_B_vs_A'] for r in rows),
            'gross_gain':sum(max(0,r['delta'][2]) for r in rows),
            'gross_loss':sum(max(0,-r['delta'][2]) for r in rows)}

def classify(cases,rows,robust,phase):
    assert phase in ('development','holdout','protected')
    assert len(rows)==2*len(cases)
    checks={}; inactive=inactive_gate([r for r in rows if r['window_ms']==5000])
    checks.update(inactive['checks'])
    for repeat in ('A','B'):
        rr=[r for r in rows if r['repeat']==repeat]
        active=[r for r in rr if r['window_ms']>5000]; a=aggregate(active)
        checks[repeat+'_overall_components']=all(x>=0 for x in aggregate(rr)['delta'])
        checks[repeat+'_active_benefit']=(all(x>=0 for x in a['delta']) and any(x>0 for x in a['delta'])
            and a['wtl']['win']>a['wtl']['loss'] and a['gross_gain']>=2*a['gross_loss'])
        checks[repeat+'_active_downside']=all(r['delta'][0]>=0 and r['delta'][1]>=0
            and r['delta'][2]>=-max(1,r['A'][2]/50) for r in active)
        checks[repeat+'_active_strata']=all(all(sum(r['delta'][k] for r in active if r[s]==v)>=0 for k in range(3))
            for s in STRATA for v in {r[s] for r in active})
    checks['causal_inactive_operation_equivalence']=all(not r['certificate']['frames'] and r['first_takeover'] is None
        for r in rows if r['window_ms']==5000)
    consistent=[c for c in cases if c['window_ms']>5000 and all(r['comparison_B_vs_A']=='win' and r['causal']
        for r in rows if r['seed']==c['seed'])]
    checks['causal_breadth']=len(consistent)>={'development':2,'holdout':4,'protected':2}[phase] and (
        phase=='protected' or len({c['family'] for c in consistent})>=2)
    if phase=='holdout':checks['both_long_windows']={c['window_ms'] for c in consistent}>={10000,15000}
    ids={c['seed'] for c in consistent}
    wins=sum(x['window_ms']>5000 and x['outcome']=='win' and x['seed'] in ids for x in robust)
    losses=sum(x['window_ms']>5000 and x['outcome']=='loss' for x in robust)
    p=sign_p(wins,losses)
    if phase!='development':checks['robust_active_causal_sign']=p<=0.05
    return {'passed':all(checks.values()),'checks':checks,'inactive_analysis':inactive,
            'robust_sign_p':p,'robust_causal_wins':wins,'all_robust_active_losses':losses,
            'consistent_causal_seeds':sorted(ids),'protocol':'366 prospective inactive calibration;362 verdict unchanged'}
