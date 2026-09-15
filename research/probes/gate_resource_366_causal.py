"""P2: active causal decision; retain the original P1 diagnostics unmodified."""
from gate_resource_366_single import classify as original_classify


def classify(cases, rows):
    original = original_classify(cases, rows)  # preserves schema/identity checks
    active = [r for r in rows if r['window_ms'] > 5000]
    keys = ('safety','active_benefit','active_strata','inactive_no_frames_or_control',
            'inactive_fixture_count','causal_breadth','single_pass_causal_sign')
    checks = {k:original['checks'][k] for k in keys}
    checks['registered_windows'] = all(r['window_ms'] in (5000,10000,15000) for r in rows)
    checks['active_no_upper_tier_loss'] = all(r['delta'][0] >= 0 and r['delta'][1] >= 0 for r in active)
    checks['active_serving_tail'] = all(r['delta'][2] >= -max(1,r['A'][2]/50) for r in active)
    return {'passed':all(checks.values()),'checks':checks,
        'original_p1_diagnostics':original,
        'causal_winner_seeds':original['causal_winner_seeds'],
        'all_active_losses':original['all_active_losses'],
        'single_pass_causal_sign_p':original['single_pass_causal_sign_p'],
        'same_fixture_aa':None,'replicated_intervals':None,
        'inactive_score_authority':'diagnostic only, both wins and losses; compiled/lifecycle proof remains mandatory',
        'protocol':'366-P2 explicit post-holdout user decision amendment; prospective one-pass protected'}
