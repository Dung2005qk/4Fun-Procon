"""Create-only holdout execution closure. Does not parse the sealed case file."""
import json
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch
import run_pair_score_341 as dev
import run_pair_holdout_341 as held


def rel(p): return str(p.relative_to(dev.ROOT)).replace('\\', '/')


def main():
    dev.require(not held.M.exists() and not held.D.exists() and not held.S.exists(),
                'holdout already frozen/opened; no duplicate')
    dev.require(dev.digest(dev.M) == '01617B33D424B892C8103D65F51DD267CFBF5C78781F78E4C76E1B2CF9B647CA',
                'development execution drift')
    m = dev.load(dev.M); dev.verify(m)
    base = dev.load(dev.ROOT/m['initial_manifest'])
    dev.require(subprocess.check_output(['git','rev-parse','HEAD'], cwd=dev.ROOT, text=True).strip()
                == base['parent_commit'], 'HEAD drift')
    expected = dev.load(dev.S)
    dev.require(dev.digest(dev.S) == 'FA19803A9ABB1DBD35C5DA965A35D8B452749E0476D8CBDBEA53775592860A4F',
                'development summary drift')
    with patch.object(dev, 'write_new', lambda p, v: dev.require(p == dev.S and v == expected,
                                                               'development recomputation mismatch')):
        dev.summarize()
    dev.require(expected['complete'] and expected['gate_passed'] and expected['zero_safety_failure'],
                'development did not qualify')
    frozen_split = base['splits']['holdout']
    dev.require(frozen_split['pairs'] == 54 and frozen_split['sha256'] ==
                '99454502161E90FEC4412786C7A89F3DC6A05ABDA82961D394E7B961F1E7ADF0', 'wrong sealed split')
    # Hash bytes only. First gameplay parsing happens in the frozen run command.
    dev.require(dev.digest(dev.ROOT/frozen_split['path']) == frozen_split['sha256'], 'sealed input drift')
    tests = subprocess.run([sys.executable, '-m', 'unittest', 'test_pair_holdout_341',
                            'test_pair_score_runner_341', 'test_http_baseline_314'],
                           cwd=dev.ROOT/'research/probes', capture_output=True, text=True)
    dev.require(tests.returncode == 0, 'holdout execution tests failed: '+tests.stdout+tests.stderr)
    preflight = dev.ROOT/f'research/evidence/{dev.ID}-holdout-preflight.json'
    dev.write_new(preflight, {'tests': tests.stdout+tests.stderr,
        'development_summary_sha256': dev.digest(dev.S),
        'dedicated_runner_sha256': dev.digest(Path(held.__file__)),
        'original_runner_sha256': dev.digest(Path(dev.__file__)),
        'sealed_gameplay_read': False,
        'authority': 'Exact phase/count/registered gate specialization; complete development replay helper equivalence; no new score run.'})
    paths = set(m['hashes']) | {rel(dev.M), rel(dev.S), rel(preflight),
        f'research/evidence/{dev.ID}-development-closure.md',
        f'research/evidence/{dev.ID}-development.runner.stdout',
        f'research/evidence/{dev.ID}-development.runner.stderr',
        'research/probes/run_pair_holdout_341.py', 'research/probes/test_pair_holdout_341.py',
        'research/probes/freeze_pair_holdout_execution_341.py'}
    paths |= {rel(p) for p in dev.D.rglob('*') if p.is_file()}
    dev.verify(m)
    dev.write_new(held.M, {'experiment': dev.ID, 'phase':'holdout',
        'initial_manifest':m['initial_manifest'], 'development_execution':rel(dev.M),
        'holdout':frozen_split['path'], 'protected':m['protected'],
        'parent_binary':m['parent_binary'], 'candidate_binary':m['candidate_binary'], 'bridge_binary':m['bridge_binary'],
        'hashes':{p:dev.digest(dev.ROOT/p) for p in sorted(paths)},
        'expected':{'pairs':54, 'results':108, 'actions':432, 'transitions':324},
        'gate':base['holdout_gate'], 'inactive_score_differences_allowed':False,
        'resource_floor_bytes':1073741824,
        'authority':'Native Windows WinHTTP synthetic loopback, not BTC, Linux sandbox or performance authority.',
        'stage':'Frozen development-qualified one-time holdout authorized. No production modification or automatic promotion.'})
    print(json.dumps({'holdout_execution_sha256':dev.digest(held.M), 'preflight_sha256':dev.digest(preflight),
                      'runner_sha256':dev.digest(Path(held.__file__)), 'dependencies':len(paths)}, indent=2))


if __name__ == '__main__': main()
