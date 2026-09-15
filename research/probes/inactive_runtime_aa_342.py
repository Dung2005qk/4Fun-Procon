"""Fresh same-binary negative control. No optimizer or production changes."""
import argparse
from collections import Counter
import json
from pathlib import Path
import time

from run_pair_protected_341 import validate_side
from protected_http_transport_341 import ROOT, Bridge, digest_case, run_case, operational
from run_pair_score_341 import load, digest, write_new, require, verify, memory_available, decisions, FIELDS
from summarize_http_baseline_314 import compare

ID = 'ATTR-INACTIVE-RUNTIME-AA-CONTROL-342'
M = ROOT / f'research/holdouts/{ID}.json'
D = ROOT / f'research/evidence/{ID}'
S = ROOT / f'research/evidence/{ID}.summary.json'
LABELS = {'parentA': ('A', 'parent'), 'candidateA': ('A', 'candidate'),
          'candidateB': ('B', 'candidate'), 'parentB': ('B', 'parent')}
SUFFIXES = ('.result.json', '.replay.jsonl', '.transport.json', '.replay-check.txt', '.stdout', '.stderr')


def order_for(index):
    labels = list(LABELS)
    shift = index % len(labels)
    return labels[shift:] + labels[:shift]


def coverage(cases):
    require(len(cases) == len({c['seed'] for c in cases}) == 8, 'eight unique cases required')
    require(Counter((c['side'], c['role'], c['fuel']) for c in cases) == Counter(
        (s, r, f) for s in (8, 32) for r in ('fixed-all-Patrol', 'native') for f in ('low', 'default')),
        'crossed fixture coverage')
    require(len({digest_case(c['setup']) for c in cases}) == 8, 'duplicate gameplay')
    for i, c in enumerate(cases):
        require(c['run_order'] == order_for(i), 'counterbalanced order drift')
        require(c['window_ms'] == 5000 and c['days'] == 4 and c['roadless'] is False, 'control scope')
        require(c['players'] == 8 + i % 3, 'team coverage')
        require(c['setup']['daySeconds'] == [5] * 4, 'public window drift')
        require(any(v == 1 for row in c['setup']['map']['cells'] for v in row), 'missing negative-control road')
    return {'fixtures': 8, 'results': 32, 'aa_pairs': 16, 'actions': 128, 'transitions': 96}


def prefix_for(c, label, directory):
    repeat, side = LABELS[label]
    return directory / repeat / side / str(c['seed'])


def validate_control(c, label, directory):
    repeat, side = LABELS[label]
    result = validate_side(c, side, directory / repeat)
    if side == 'candidate':
        for day in decisions(prefix_for(c, label, directory).with_suffix('.replay.jsonl')):
            for stats in [day['profile']['horizonPricing'], *[v['horizonPricing'] for v in day['audit']['candidates']]]:
                require(all(stats[k] == 0 for k in FIELDS if k != 'calls'), 'negative control entered supported pricing')
    return result


def fixture_marker(c, directory):
    return {'seed': c['seed'], 'case_sha256': digest_case(c), 'run_order': c['run_order'],
            'result_hashes': {label: digest(prefix_for(c, label, directory).with_suffix('.result.json')) for label in LABELS}}


def audit(cases, directory, validator=validate_control):
    allowed = {'A', 'B', 'run_complete.json'} | {str(c['seed']) + '.fixture_complete.json' for c in cases}
    require(all(p.name in allowed for p in directory.iterdir()), 'foreign root evidence')
    for repeat in ('A', 'B'):
        require((directory / repeat).is_dir(), 'missing repeat directory')
        require({p.name for p in (directory / repeat).iterdir()} == {'parent', 'candidate'}, 'foreign repeat evidence')
        for side in ('parent', 'candidate'):
            folder = directory / repeat / side
            require(folder.is_dir(), 'missing side directory')
            expected = {str(c['seed']) + suffix for c in cases for suffix in SUFFIXES}
            require(all(p.name in expected for p in folder.iterdir()), 'foreign side artifact')
    for c in cases:
        for label in LABELS:
            prefix = prefix_for(c, label, directory)
            if prefix.with_suffix('.result.json').exists():
                validator(c, label, directory)
            else:
                require(not list(prefix.parent.glob(prefix.name + '.*')), 'ambiguous partial side; no blind resume')
        marker = directory / (str(c['seed']) + '.fixture_complete.json')
        if marker.exists():
            require(load(marker) == fixture_marker(c, directory), 'fixture marker mismatch')


def complete(m, cases, directory=D):
    audit(cases, directory)
    counts = coverage(cases)
    marker = load(directory / 'run_complete.json')
    require(all(marker[k] == v for k, v in counts.items()), 'completion counts')
    require(marker['manifest_sha256'] == digest(M), 'completion manifest mismatch')
    markers = sorted(directory.glob('*.fixture_complete.json'))
    require(len(markers) == 8 and len(list(directory.glob('*/*/*.result.json'))) == 32, 'exact file counts')
    require(marker['fixture_hashes'] == {p.name: digest(p) for p in markers}, 'completion evidence hash mismatch')
    return marker


def run(resume=False):
    m = load(M); verify(m); cases = m['cases']; counts = coverage(cases)
    if D.exists():
        require(resume and not (D / 'run_complete.json').exists(), 'existing run; no duplication')
    else:
        D.mkdir()
        for repeat, side in LABELS.values():
            (D / repeat / side).mkdir(parents=True)
    audit(cases, D)
    bridge = Bridge(ROOT / m['bridge_binary'])
    try:
        for i, c in enumerate(cases):
            for label in c['run_order']:
                prefix = prefix_for(c, label, D)
                if prefix.with_suffix('.result.json').exists():
                    continue
                require(memory_available() >= m['resource_floor_bytes'], 'available RAM below frozen1024MiB floor')
                repeat, side = LABELS[label]
                started = time.monotonic()
                run_case(c, m[side + '_binary'], prefix.parent, bridge, ID, side)
                validate_control(c, label, D)
                print(f'side_complete fixture={i+1} label={label} elapsed_seconds={time.monotonic()-started:.3f}', flush=True)
            p = D / (str(c['seed']) + '.fixture_complete.json')
            row = fixture_marker(c, D)
            if p.exists(): require(load(p) == row, 'existing marker changed')
            else: write_new(p, row)
            print(f'fixture_complete count={i+1}', flush=True)
    finally:
        bridge.close(); bridge.process.stdout.close(); bridge.process.stderr.close()
    verify(m)
    write_new(D / 'run_complete.json', {**counts, 'manifest_sha256': digest(M),
        'fixture_hashes': {p.name: digest(p) for p in sorted(D.glob('*.fixture_complete.json'))}})
    print('run_complete results=32 aa_pairs=16', flush=True)
    summarize()


def compare_runs(a, b):
    plans = [[day['plan'] for day in t['actions']] for t in (a, b)]
    equal = {k: [x['validated'][k] for x in a['actions']] == [x['validated'][k] for x in b['actions']]
             for k in ('agents', 'ledger', 'road_footprint')}
    roles_equal = a['roles'] == b['roles']
    outcome, tier, difference = compare(b['score'], a['score'])
    return {'A': a['score'], 'B': b['score'], 'comparison_B_vs_A': outcome,
        'first_tier': tier, 'first_tier_difference': difference,
        'delta': [y-x for x, y in zip(a['score'], b['score'], strict=True)],
        'roles': {'A': a['roles'], 'B': b['roles']}, 'roles_equal': roles_equal,
        'plans_equal': plans[0] == plans[1], 'transition_equal': equal,
        'trajectory_equal': roles_equal and plans[0] == plans[1] and all(equal.values()),
        'first_divergence': 0 if not roles_equal else next((i+1 for i, (x,y) in enumerate(zip(a['actions'],b['actions'],strict=True))
            if x['plan'] != y['plan'] or any(x['validated'][k] != y['validated'][k] for k in equal)), None),
        'plan_hashes': dict(zip(('A', 'B'), (digest_case(p) for p in plans), strict=True))}


def aggregate(rows):
    return {'pairs': len(rows), 'score_differences': sum(r['A'] != r['B'] for r in rows),
        'role_differences': sum(not r['roles_equal'] for r in rows),
        'trajectory_differences': sum(not r['trajectory_equal'] for r in rows),
        'wtl_B_vs_A': dict(Counter(r['comparison_B_vs_A'] for r in rows)),
        'delta': [sum(r['delta'][i] for r in rows) for i in range(3)],
        'first_tiers': dict(Counter(str(r['first_tier']) for r in rows))}


def summarize():
    m = load(M); verify(m); cases = m['cases']; marker = complete(m, cases)
    rows = []
    for c in cases:
        for side in ('parent', 'candidate'):
            labels = [side + repeat for repeat in ('A', 'B')]
            paths = [prefix_for(c, label, D) for label in labels]
            transports = [load(p.with_suffix('.transport.json')) for p in paths]
            row = {k: c[k] for k in ('seed', 'side', 'role', 'fuel', 'family', 'players', 'days', 'window_ms', 'run_order')}
            row.update(binary=side, binary_sha256=digest(ROOT/m[side+'_binary']), **compare_runs(*transports))
            row['result_hashes'] = {label: digest(p.with_suffix('.result.json')) for label, p in zip(labels, paths, strict=True)}
            row['safety'] = {label: operational(p.with_suffix('.replay.jsonl'), c, side) for label, p in zip(labels, paths, strict=True)}
            row['candidate_pricing_calls'] = None if side == 'parent' else {
                label: sum(sum(v['horizonPricing']['calls'] for v in d['audit']['candidates'])
                           for d in decisions(p.with_suffix('.replay.jsonl'))) for label,p in zip(labels,paths,strict=True)}
            row['supported_pricing'] = None if side == 'parent' else 0
            rows.append(row)
    by_binary = {side: aggregate([r for r in rows if r['binary'] == side]) for side in ('parent', 'candidate')}
    report = {'experiment': ID, 'complete': True, **coverage(cases), 'by_binary': by_binary, 'rows': rows,
        'strata': {side: {key: {str(v): aggregate([r for r in rows if r['binary']==side and r[key]==v])
            for v in sorted({r[key] for r in rows})} for key in ('side','role','fuel','family','players')}
            for side in ('parent','candidate')}, 'manifest_sha256': digest(M),
        'completion_sha256': digest(D/'run_complete.json'), 'zero_safety_failure': True,
        'observed_nonrepeatability': any(not r['trajectory_equal'] for r in rows),
        'authority': 'A/A attribution only on fresh unsupported-domain Windows synthetic HTTP. Not BTC performance, not a quantitative variance model, not a rescue of341 or promotion evidence.'}
    write_new(S, report)
    print(json.dumps({'by_binary': by_binary, 'summary_sha256': digest(S)}), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('mode', choices=('run','summarize')); parser.add_argument('--resume', action='store_true')
    args = parser.parse_args(); run(args.resume) if args.mode == 'run' else summarize()
