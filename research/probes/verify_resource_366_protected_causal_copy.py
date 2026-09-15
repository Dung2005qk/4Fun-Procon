"""Reverify complete P2 evidence with the frozen runtime and classifier; no solver."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[2]
COPY = ROOT / 'artifacts/research/366/completed-protected-causal'
PACKAGE = COPY / 'package'
OLD = ROOT / 'artifacts/research/357/completed'
ID = 'SCORE-CAUSAL-RESOURCE-QUALIFICATION-366'
DATA = COPY / 'research/evidence' / (ID + '-protected-causal-single')
ARCHIVE_HASH = '7FBA3D84075973544C156397F141188521291701C78ADFB375543518FB1E8F96'
SUMMARY_HASH = 'B401C7698036FFFF557B0520055D016AB328D3C7C5F83B57BE4CBD8233D97CB4'
COMPLETE_HASH = 'B3939DC611F695D86168BD1D6EA274592DF851A4BE9537DA66FB4E98B08A6C85'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def extract_once():
    archive = COPY.parent / 'complete-protected-causal.tar.gz'
    assert sha(archive) == ARCHIVE_HASH
    if COPY.exists():
        return
    with tarfile.open(archive) as tf:
        members = tf.getmembers()
        assert len(members) == len({m.name for m in members}) == 1647
        for m in members:
            p = (COPY / m.name).resolve()
            assert m.isfile() and p.is_relative_to(COPY.resolve()) and not p.exists(), m.name
        COPY.mkdir()
        tf.extractall(COPY, members=members, filter='data')


def verify_files():
    expected = {
        'execution366.json': '94CF0B0FC04DE3277BD0584C2E54898D510F0B076923E8EECC9FBB7F02D648AC',
        'package/execution366-causal.json': 'C968D42AE14F881D49DC9A8DC631E42960EF34BE67CF9433565E5E563803C92D',
        'package/runner366-p2.json': '89C6AD1C9999D802D8597A4EBDD9023C40C587A189A0F5AB832B9A4923194F05',
        f'research/holdouts/{ID}-protected.json': '02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7',
        f'research/evidence/{ID}-protected-causal-single.summary.json': SUMMARY_HASH,
        f'research/evidence/{ID}-protected-causal-single/run_complete.json': COMPLETE_HASH,
    }
    for p, h in expected.items():
        assert sha(COPY / p) == h, p
    for base, name in ((COPY, 'execution366.json'), (PACKAGE, 'execution366-causal.json')):
        for p, h in load(base / name)['hashes'].items():
            assert (base / p).resolve().is_relative_to(base.resolve())
            assert sha(base / p) == h, p
    complete = load(DATA / 'run_complete.json')
    actual = {p.relative_to(DATA).as_posix(): sha(p) for p in DATA.rglob('*')
              if p.is_file() and p.name != 'run_complete.json'}
    assert actual == complete['files'] and len(actual) == 1620
    assert (PACKAGE / 'runner366-p2.stderr').stat().st_size == 0
    return complete


def main():
    extract_once()
    complete = verify_files()
    # Completed prior verifier covers immutable compiled/lifecycle dependencies.
    import verify_resource_366_holdout_copy as previous
    previous.verify_copy()
    sys.path.insert(0, str(OLD / 'research/probes'))
    sys.path.insert(0, str(COPY))
    sys.path.insert(0, str(PACKAGE))
    spec = importlib.util.spec_from_file_location('frozen366_local', COPY / 'score_resource_366.py')
    original = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(original)
    original.WORK = COPY
    original.EXEC = COPY / 'execution366.json'
    original.INPUT = COPY / 'input366.json'
    import score_resource_366_causal as p2
    p2.WORK = COPY
    p2.PACKAGE = PACKAGE
    p2.EXEC = PACKAGE / 'execution366-causal.json'
    p2.DATA = DATA
    p2.SUMMARY = DATA.with_suffix('.summary.json')

    def prior_adapter(unused_original, check=False):
        report = load(previous.DATA.with_suffix('.summary.json'))
        marker = load(previous.DATA / 'run_complete.json')
        p2.require_previous(report, marker)
        assert sha(previous.DATA.with_suffix('.summary.json')) == report_hash
        return report_hash

    report_hash = '88E7FF81955C7AFAC838D430508809B8B21C2AC00C9906E7BA6E3994D9EF00B3'
    p2.verify_package = lambda: original  # Only host/path verification adapter.
    p2.previous_gate = prior_adapter
    with contextlib.redirect_stdout(io.StringIO()):
        p2.summarize(check=True)  # Frozen full audit, certificates, rows and metric.
    report = load(p2.SUMMARY)
    rows = report['rows']
    frames = [f for row in rows for f in row['certificate']['frames']]
    audit = {
        'verified': True, 'solver_executed': False,
        'archive_sha256': ARCHIVE_HASH, 'summary_sha256': SUMMARY_HASH,
        'completion_sha256': COMPLETE_HASH, 'files': len(complete['files']),
        **{k: report[k] for k in ('fixtures', 'results', 'certificates', 'actions', 'transitions',
                                 'summary', 'active_only_summary', 'strata', 'zero_safety_failure',
                                 'independent5000_score_mismatches', 'independent5000_trajectory_mismatches')},
        'gate': report['gate'],
        'inactive': original.f.aggregate([r for r in rows if r['window_ms'] == 5000]),
        'active_strata': {k: {str(v): original.f.aggregate([r for r in rows if r['window_ms'] > 5000 and r[k] == v])
                             for v in sorted({r[k] for r in rows if r['window_ms'] > 5000})}
                          for k in original.f.STRATA},
        'losses': [{k: v for k, v in r.items() if k != 'certificate'} for r in rows if r['comparison_B_vs_A'] == 'loss'],
        'work': {k: sum(f[k] for f in frames) for k in
                 ('entered', 'takeover', 'deadline', 'failure', 'queries', 'settled', 'routes', 'evaluated', 'valid', 'certified')},
    }
    output = ROOT / 'research/evidence' / (ID + '-protected-causal-audit.json')
    if output.exists():
        assert load(output) == audit
    else:
        with output.open('x', encoding='utf-8', newline='\n') as out:
            json.dump(audit, out, indent=2, sort_keys=True)
            out.write('\n')
    print(json.dumps({k: audit[k] for k in ('verified', 'files', 'fixtures', 'results', 'actions', 'transitions',
                                          'active_only_summary', 'inactive', 'work')}))
    print('gate_checks', json.dumps(report['gate']['checks']))
    print('audit_sha256', sha(output))


if __name__ == '__main__':
    main()
