"""User-amended protected-only one-pass classifier; no outcomes or I/O inputs."""
from gate_resource_366 import STRATA, aggregate, sign_p


def classify(cases, rows):
    assert len(cases) == len(rows) and cases
    assert len({c['seed'] for c in cases}) == len(cases)
    assert {r['seed'] for r in rows} == {c['seed'] for c in cases}
    by_seed = {c['seed']: c for c in cases}
    for r in rows:
        assert r['repeat'] == 'A'
        assert all(r[k] == by_seed[r['seed']][k] for k in STRATA)
        assert len(r['A']) == len(r['B']) == len(r['delta']) == 3
        assert r['delta'] == [b-a for a,b in zip(r['A'],r['B'])]
        outcome = 'win' if r['B'] > r['A'] else 'loss' if r['B'] < r['A'] else 'tie'
        assert r['comparison_B_vs_A'] == outcome
    active = [r for r in rows if r['window_ms'] > 5000]
    inactive = [r for r in rows if r['window_ms'] == 5000]
    a = aggregate(active)
    checks = {
        'safety': all(r['safety'] is True for r in rows),
        'overall_components': all(v >= 0 for v in aggregate(rows)['delta']),
        'active_benefit': (all(v >= 0 for v in a['delta']) and any(v > 0 for v in a['delta'])
            and a['wtl']['win'] > a['wtl']['loss'] and a['gross_gain'] >= 2*a['gross_loss']),
        'no_individual_upper_tier_loss': all(r['delta'][0] >= 0 and r['delta'][1] >= 0 for r in rows),
        'all_lane_serving_tail_no_aa_relief': all(r['delta'][2] >= -max(1,r['A'][2]/50) for r in rows),
        'active_strata': all(all(sum(r['delta'][k] for r in active if r[s] == v) >= 0 for k in range(3))
            for s in STRATA for v in {r[s] for r in active}),
        'inactive_no_frames_or_control': all(not r['certificate']['frames'] and r['first_takeover'] is None
            for r in inactive),
        'inactive_fixture_count': len(inactive) >= 12,
    }
    details = [{'seed':r['seed'],'normalized_delta':r['delta'][2]/max(1,r['A'][2]/50),
                'tail_limit':max(1,r['A'][2]/50)} for r in inactive]
    values = [r['normalized_delta'] for r in details]
    ni = sign_p(sum(x > -1 for x in values),sum(x < -1 for x in values))
    shift = sign_p(sum(x < 0 for x in values),sum(x > 0 for x in values))
    checks['inactive_noninferiority'] = ni <= .05
    checks['inactive_no_detected_negative_shift'] = shift > .05
    causal = [r for r in active if r['comparison_B_vs_A'] == 'win' and r['causal'] is True
              and r['first_takeover'] is not None and r['first_divergence'] is not None
              and r['first_takeover'] <= r['first_divergence']]
    losses = sum(r['comparison_B_vs_A'] == 'loss' for r in active)
    p = sign_p(len(causal),losses)
    checks['causal_breadth'] = len(causal) >= 2
    checks['single_pass_causal_sign'] = p <= .05
    return {'passed':all(checks.values()),'checks':checks,
        'inactive_analysis':{'noninferiority_p':ni,'negative_shift_p':shift,'fixtures':details},
        'causal_winner_seeds':sorted(r['seed'] for r in causal),'all_active_losses':losses,
        'single_pass_causal_sign_p':p,'same_fixture_aa':None,'replicated_intervals':None,
        'protocol':'366-P1 user-authorized single-pass protected; not original repeated protected pass'}
