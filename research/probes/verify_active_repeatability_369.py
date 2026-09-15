"""Revalidate the completed369 copy using frozen lifecycle/certificate checks."""
import importlib.util
import json
from pathlib import Path
import sys
from explain_active_repeatability_369 import ROOT, COPY, EXEC_HASH, LABELS, SEED, load, sha


def main():
    assert sha(COPY/'execution369.json') == EXEC_HASH
    complete = load(COPY/'data/run_complete.json')
    assert sha(COPY/'summary369.json') == complete['summary_sha256']
    actual = {p.relative_to(COPY/'data').as_posix():sha(p) for p in (COPY/'data').rglob('*')
              if p.is_file() and p.name != 'run_complete.json'}
    assert actual == complete['files'] and len(actual) == 28
    assert (COPY/'runner369.stderr').stat().st_size == 0
    assert (complete['results'],complete['actions'],complete['transitions']) == (4,40,36)
    frozen = ROOT/'artifacts/research/357/completed/research/probes'
    sys.path.insert(0,str(frozen))
    import run_resource_357 as f
    pkg = ROOT/'artifacts/research/366/completed-protected-causal/package'
    checker_file = pkg/'score_resource_366_causal.py'
    assert sha(checker_file) == '4C314DF4BA09497F647A7E96DCF08C8CA116D51E9A08DAC2AC820689F8652474'
    spec = importlib.util.spec_from_file_location('frozen_p2_checker_for369',checker_file)
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    case = load(COPY/'execution369.json')['case']
    rows = {}
    counts = {'resource_frames':0, 'takeovers':0, 'deadline':0, 'failure':0}
    for label in LABELS:
        side = label[:-1]
        p = COPY/'data'/label[-1]/side/str(SEED)
        f.validate_side(case,side,COPY/'data'/label[-1])
        f.tr.operational(p.with_suffix('.replay.jsonl'),case,side)
        cert = load(p.with_suffix('.certificate.json'))
        checker.verify_certificate(cert,side,case)
        counts['resource_frames'] += len(cert['frames'])
        for field in ('takeovers','deadline','failure'):
            source_field = 'takeover' if field == 'takeovers' else field
            counts[field] += sum(x[source_field] for x in cert['frames'])
        rows[label] = load(p.with_suffix('.transport.json'))
    summary = load(COPY/'summary369.json')
    assert summary['scores'] == {label:t['score'] for label,t in rows.items()}
    assert summary['AB'] == {r:f.compare_runs(rows['parent'+r],rows['candidate'+r]) for r in ('A','B')}
    assert summary['AA'] == {s:f.compare_runs(rows[s+'A'],rows[s+'B']) for s in ('parent','candidate')}
    source_paths = ('src/graph.cpp','include/udon/graph.hpp','src/decision.cpp','src/runtime.cpp',
                    'include/udon/planner.hpp','include/udon/decision.hpp')
    source_checks = {}
    for path in source_paths:
        a,b = ROOT/path,ROOT/'artifacts/research/357/completed/source'/path
        assert a.read_text(encoding='utf-8') == b.read_text(encoding='utf-8'),path
        source_checks[path] = {'production_sha256':sha(a),'frozen_sha256':sha(b),'normalized_equal':True}
    report = {'experiment':'ATTR-ACTIVE-MAIN-REPEATABILITY-369', 'complete':True,
              'verified_files':28,'results':4,'actions':40,'transitions':36,
              'script_sha256':sha(Path(__file__)), 'counts':counts, 'source_checks':source_checks,
              'frozen_certificate_checker_sha256':sha(checker_file),
              'execution_sha256':EXEC_HASH, 'completion_sha256':sha(COPY/'data/run_complete.json'),
              'summary_sha256':sha(COPY/'summary369.json'),
              'source_audit':'resource method has no engine-router reference; post-ACK shares engine router; interrupted route results are not cached; hits precede deadline',
              'solver_rerun':False,'frozen_gate_changed':False,'promotion_authorized':False}
    out = ROOT/'research/evidence/ATTR-ACTIVE-MAIN-REPEATABILITY-369-verification.json'
    if out.exists():
        assert load(out) == report
    else:
        with out.open('x',encoding='utf-8',newline='\n') as stream:
            json.dump(report,stream,indent=2,sort_keys=True)
            stream.write('\n')
    print('verified',json.dumps(counts),'sha256',sha(out))


if __name__=='__main__':
    main()
