"""Read-only within-day attribution of the two complete protected active losses."""
import json
from pathlib import Path
from collections import Counter
from audit_candidate_availability_363 import identity, semantic, inventory, membership
from verify_resource_366_protected_causal_copy import ROOT, COPY, DATA, sha, load, verify_files, SUMMARY_HASH

ID = 'ATTR-ACTIVE-PROTECTED-DIVERGENCE-368'


def first_difference(values):
    return next((i + 1 for i, value in enumerate(values) if not value), None)


def analyze(row):
    transports, documents, refs, events = {}, {}, {}, {}
    for side in ('parent', 'candidate'):
        prefix = DATA / 'A' / side / str(row['seed'])
        transports[side] = load(prefix.with_suffix('.transport.json'))
        events[side] = [json.loads(line) for line in prefix.with_suffix('.replay.jsonl').read_text().splitlines()]
        documents[side] = [e['body'] for e in events[side] if e['kind'] == 'decision']
        refs[side] = [e['body'] for e in events[side] if e['kind'] == 'protected_slack']
    assert all(len(documents[s]) == len(transports[s]['actions']) == row['days'] for s in transports)
    frames = {f['day']: f for f in row['certificate']['frames']}
    days = []
    previous_scores = {'parent': [0, 0, 0], 'candidate': [0, 0, 0]}
    for i, (a, b) in enumerate(zip(documents['parent'], documents['candidate'], strict=True)):
        f = frames.get(i + 1)
        day = {'day': i + 1,
               'main_state_equal': semantic(a['state']) == semantic(b['state']),
               'main_ledger_equal': a['ledger'] == b['ledger'],
               'main_plan_equal': a['decision']['candidate']['plan'] == b['decision']['candidate']['plan'],
               'manifest_equal': a['decision']['manifest'] == b['decision']['manifest'],
               'main_plan_hashes': [identity(d['decision']['candidate']['plan']) for d in (a, b)],
               'main_scores': [d['decision']['candidate']['scoreAfterToday'] for d in (a, b)],
               'frame': {k: v for k, v in f.items() if k not in ('parentPlan', 'outputPlan')} if f else None,
               'same_day_pre_resource_equals_other_parent_plan':
                    f['parentPlan'] == transports['parent']['actions'][i]['plan'] if f else None,
               'sides': {}}
        for side, d in (('parent', a), ('candidate', b)):
            action = transports[side]['actions'][i]
            validated = action['validated']
            score = validated['score']
            ref = refs[side][i]
            day['sides'][side] = {
                'score': score, 'daily_score_increment': [x-y for x, y in zip(score, previous_scores[side])],
                'main_state': d['state'], 'main_ledger': d['ledger'],
                'main_candidate': d['decision']['candidate'],
                'master': d['decision']['masterDiagnostics'],
                'timing': d['decision']['timing'], 'deadline': d['decision']['deadline'],
                'profile': d['decision']['profile'],
                'audit_without_candidates': {k: v for k, v in d['decision']['audit'].items() if k != 'candidates'},
                'protected': ref, 'validated': validated,
                'submitted_plan_hash': identity(action['plan']),
            }
            previous_scores[side] = score
        day['daily_delta'] = [b-a for a,b in zip(day['sides']['parent']['daily_score_increment'],
                                               day['sides']['candidate']['daily_score_increment'])]
        days.append(day)
    first_main = first_difference([d['main_plan_equal'] for d in days])
    detail = None
    if first_main is not None:
        a, b = (documents[s][first_main - 1] for s in ('parent', 'candidate'))
        detail = {'day': first_main, 'left_selected_in_right': membership(a, b),
                  'right_selected_in_left': membership(b, a),
                  'inventories': {'parent': inventory(a), 'candidate': inventory(b)}}
    return {**{k: v for k, v in row.items() if k != 'certificate'},
            'event_kinds': {s: dict(Counter(e['kind'] for e in events[s])) for s in events},
            'first_main_plan_difference': first_main,
            'first_main_state_difference': first_difference([d['main_state_equal'] for d in days]),
            'first_main_ledger_difference': first_difference([d['main_ledger_equal'] for d in days]),
            'first_main_detail': detail, 'days_detail': days}


def main():
    verify_files()
    summary = DATA.with_suffix('.summary.json')
    assert sha(summary) == SUMMARY_HASH
    report = load(summary)
    losses = [r for r in report['rows'] if r['window_ms'] > 5000 and r['comparison_B_vs_A'] == 'loss']
    assert len(losses) == 2
    result = {'experiment': ID, 'summary_sha256': SUMMARY_HASH,
              'script_sha256': sha(Path(__file__)), 'solver_executed': False,
              'gate_changed': False, 'losses': [analyze(row) for row in losses]}
    output = ROOT / 'research/evidence' / (ID + '.json')
    if output.exists():
        assert load(output) == result
    else:
        with output.open('x', encoding='utf-8', newline='\n') as out:
            json.dump(result, out, indent=2, sort_keys=True)
            out.write('\n')
    for loss in result['losses']:
        print(json.dumps({k: loss[k] for k in ('seed', 'delta', 'roles_equal', 'first_divergence', 'first_takeover',
                                              'first_main_plan_difference', 'first_main_state_difference',
                                              'first_main_ledger_difference', 'event_kinds')}))
        for d in loss['days_detail']:
            print(json.dumps({k: d[k] for k in ('day', 'main_state_equal', 'main_ledger_equal', 'main_plan_equal',
                                               'manifest_equal', 'same_day_pre_resource_equals_other_parent_plan', 'daily_delta')}))
        if loss['first_main_detail']:
            print('membership', {k: loss['first_main_detail'][k]['boundary'] for k in
                                 ('left_selected_in_right', 'right_selected_in_left')})
    print('evidence_sha256', sha(output))


if __name__ == '__main__':
    main()
