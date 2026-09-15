"""Local byte/lifecycle verification only; no optimizer or VM process started."""
import hashlib
import json
from pathlib import Path
from gain_short_window_365 import check_short_guard, tests
from takeover_resume_364 import prefix_bytes

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'artifacts/research/365/completed'
SOURCE = ROOT/'artifacts/research/362/completed/research/evidence/SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362-development'


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p): return json.loads(p.read_text())


def verify():
    assert sha(WORK/'stage365.json') == 'CC35B6AEAA356EF0ADF4AB819DE88120BE9AA7C3BB43F690BAF56526C6042D9C'
    for name, h in load(WORK/'stage365.json')['hashes'].items(): assert sha(WORK/name) == h, name
    assert sha(WORK/'gain_short_window_365.py') == sha(ROOT/'research/probes/gain_short_window_365.py')
    assert tests() == {'positive': 1, 'adverse': 4}
    m = load(WORK/'input365.json'); report = load(WORK/'complete365.json')
    assert report['complete'] and report['score_authority'] is False
    assert report['stage_sha256'] == sha(WORK/'stage365.json')
    assert report['expired_waits'] == report['safety_failures'] == 0
    assert sha(SOURCE/'run_complete.json') == '24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791'
    expected = {f"{s['seed']}-{c}" for s in m['sessions'] for c in m['controls']}
    assert {p.name for p in (WORK/'sessions').iterdir()} == expected
    rows = []
    for s in m['sessions']:
        seed, n = s['seed'], s['restored_days']
        for name, h in s['source_hashes'].items(): assert sha(SOURCE/name) == h, name
        before = prefix_bytes(SOURCE/f'A/candidate/{seed}.replay.jsonl', n)
        assert hashlib.sha256(before).hexdigest().upper() == s['prefix_sha256']
        original = load(SOURCE/f'A/candidate/{seed}.transport.json')
        for c in m['controls']:
            p = WORK/'sessions'/f'{seed}-{c}'; row = load(p/'case_complete.json'); rows.append(row)
            assert set(x.name for x in p.iterdir()) == set(row['files']) | {'case_complete.json'}
            for name, h in row['files'].items(): assert sha(p/name) == h, name
            t = load(p/'transport.json'); assert t['error'] is None and t['exit_code'] == 0
            assert t['day'] == n+2 and not t['assignment_posts'] and len(t['new_actions']) == 2
            assert t['restored'] == [a['validated'] for a in original['actions'][:n]]
            assert t['own'] == [a['road_footprint'] for a in t['restored']]+[a['validated']['road_footprint'] for a in t['new_actions']]
            agents = t['restored'][-1]['agents']
            for i, a in enumerate(t['new_actions'], n):
                assert a['wire_day'] == a['state']['day'] == i and a['state']['agents'] == agents
                assert a['validated']['ok'] and a['validated']['agrees'] and a['deadline_margin_ms'] > 0
                agents = a['validated']['agents']
            posts = [q for q in t['requests'] if q['query']['method'] == 'POST']
            assert len(posts) == 2 and [q['wire_day'] for q in posts] == [n, n+1]
            assert all(q['query']['path'].endswith('/actions') and q['response']['status'] == 200 for q in posts)
            raw = (p/'replay.jsonl').read_bytes(); assert raw.startswith(before)
            events = [json.loads(x) for x in raw[len(before):].splitlines()]; by = {}
            for e in events: by.setdefault(e['kind'], []).append(e)
            assert not set(('resource_marginal','actions_server_wait','actions_deadline_skip','actions_fallback',
                'actions_recovery_wait','virtual_parent_drop','virtual_parent_dropped','action_transport_retry')).intersection(by)
            for kind in ('day_state','decision','protected_slack','actions','action_result','checkpoint_actions'):
                assert len(by.get(kind, [])) == 2, kind
            check_short_guard(by['protected_slack'][0]['body'])
            assert by['protected_slack'][1]['body']['publicContinuationAuthorized'] is True
            for i in range(2):
                d=by['decision'][i]['body']['decision']; st=by['day_state'][i]; ack=by['action_result'][i]
                assert d['dayNumber'] == n+i+1 and not d['emergency'] and d['candidate']['simulation']['valid']
                assert 0 < d['deadline']['totalMs'] <= 5000 and d['timing']['totalMs'] == row['main_ms'][i] <= 5000
                assert ack['body']['day'] == n+i+1 and ack['body']['valid'] and ack['status'] == 200
                assert st['atUnixMs'] < st['body']['endsAt'] and ack['atUnixMs'] <= st['body']['endsAt']
                assert row['response_ms'][i] == ack['atUnixMs']-st['atUnixMs']
                assert not any(v for k,v in by['protected_slack'][i]['body'].items() if k.endswith('Failure'))
            assert (p/'stderr').stat().st_size == 0
            check = (p/'replay-check.txt').read_text()
            assert f'summary days={n+2} reconciled_transitions={n+1}' in check
            assert f'resume accepted_days={n+2} last_wire_day={n+1}' in check
    assert report['rows'] == rows and len(rows) == report['sessions'] == 4
    for key in ('new_acks','restored_days','full_days','transitions'):
        assert sum(r[key] for r in rows) == report[key] == m['expected_'+key]
    assert (WORK/'runner365.stderr').stat().st_size == 0
    return {'verified': True, 'sessions': 4, 'new_acks': 8, 'restored_days': 10,
        'full_days': 18, 'transitions': 14, 'safety_failures': 0, 'unexpected_expired_waits': 0,
        'main_ms': [r['main_ms'] for r in rows], 'response_ms': [r['response_ms'] for r in rows],
        'complete_sha256': sha(WORK/'complete365.json'), 'archive_sha256': sha(WORK.parent/'complete.tar.gz')}


if __name__ == '__main__': print(json.dumps(verify(), indent=2))
