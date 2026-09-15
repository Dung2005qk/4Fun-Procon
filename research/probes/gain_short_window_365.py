"""Finite real-clock lifecycle contract on retained bytes, not SCORE."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import queue
import shutil
import subprocess
import sys
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = Path('/home/LMC/udon365-0909')
OLD = Path('/home/LMC/udon364-0909')
BASE = Path('/home/LMC/udon360-0909')
PACKAGE = Path('/home/LMC/udon357-0909')
SOURCE = Path('/home/LMC/udon362-0909/research/evidence/SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362-development')
ID = 'CONTRACT-GAIN-TO-SHORT-WINDOW-365'
BINARY_SHA = 'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678'
OLD_SHA = 'F7AA82A6D67E79C882B609F68A0E4A62C00EAE8250458937235875CFB31B953C'


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p): return json.loads(p.read_text())
def write(p, obj):
    with p.open('x', encoding='utf-8', newline='\n') as f:
        json.dump(obj, f, indent=2, sort_keys=True); f.write('\n')


def check_short_guard(slack):
    assert slack['publicContinuationAuthorized'] is False
    assert 0 < slack['outerWindowMs'] <= 5000
    assert not any(v for k, v in slack.items() if k.endswith('Failure'))


def tests():
    valid = {'publicContinuationAuthorized': False, 'outerWindowMs': 4999,
             'publicContinuationFailure': False}
    check_short_guard(valid)
    for field, value in [('publicContinuationAuthorized', True), ('outerWindowMs', 5001),
                         ('outerWindowMs', 0), ('publicContinuationFailure', True)]:
        bad = dict(valid, **{field: value})
        try: check_short_guard(bad)
        except AssertionError: pass
        else: raise AssertionError('adverse short-window checker accepted')
    return {'positive': 1, 'adverse': 4}


def stage():
    assert tests() == {'positive': 1, 'adverse': 4}
    assert sha(ROOT/'artifacts/research/360/completed/btc360-plain') == BINARY_SHA
    previous = ROOT/'artifacts/research/364/completed'
    assert sha(previous/'complete364.json') == '8DDF91A0C5688855AB979383551F9ABBC7AD854FD6B397384571D21219113FB2'
    old = load(previous/'input364.json')
    folder = ROOT/'artifacts/research/365/stage'
    folder.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(Path(__file__), folder/Path(__file__).name)
    shutil.copyfile(ROOT/'research/evidence'/f'{ID}-preregistration.md', folder/'preregistration.md')
    write(folder/'input365.json', {'experiment': ID, 'sessions': old['sessions'], 'controls': ['0', '1'],
        'binary_sha256': BINARY_SHA, 'old_input_sha256': sha(previous/'input364.json'),
        'old_runner_sha256': OLD_SHA, 'short_authoritative_ms': 5000,
        'expected_sessions': 4, 'expected_new_acks': 8, 'expected_restored_days': 10,
        'expected_full_days': 18, 'expected_transitions': 14, 'no_score_authority': True})
    write(folder/'stage365.json', {'hashes': {p.name: sha(p) for p in sorted(folder.iterdir()) if p.is_file()},
        'guard_tests': tests()})
    print('stage365', sha(folder/'stage365.json'))


def dependencies():
    assert subprocess.check_output(['hostname'], text=True).strip() == 'udon-f0-240-0829'
    for p, h in load(WORK/'stage365.json')['hashes'].items(): assert sha(WORK/p) == h, p
    assert sha(OLD/'takeover_resume_364.py') == OLD_SHA
    sys.path.insert(0, str(OLD))
    import takeover_resume_364 as old
    old.verify()
    m = load(WORK/'input365.json')
    assert sha(OLD/'input364.json') == m['old_input_sha256']
    assert m['sessions'] == load(OLD/'input364.json')['sessions']
    assert sha(BASE/'btc360-plain') == BINARY_SHA
    assert tests() == {'positive': 1, 'adverse': 4}
    sys.path.insert(0, str(PACKAGE/'research/probes'))
    import run_resource_357 as frozen
    return m, old, frozen.rpc


def restored_server(old, rpc):
    class ShortAfterGain(old.server_class(rpc)):
        def get(self, endpoint):
            opening_short = endpoint == 'state' and self.day == self.expire_day and self.deadline is None
            status, body = rpc.ProtectedCase.get(self, endpoint)
            if opening_short:
                assert status == 200 and self.pending is None
                self.window_ms = 5000
                self.deadline = self.clock() + 5
                self.ends_at = int(self.wall()*1000) + 5000
                self.last_state['endsAt'] = self.ends_at
                body = self.last_state
            return status, body

        def post(self, endpoint, body):
            assert endpoint == 'actions' and self.day in (self.expire_day, self.expire_day+1)
            assert self.pending is None, 'duplicate accepted action'
            return rpc.ProtectedCase.post(self, endpoint, body)
    return ShortAfterGain


def validate(folder, s, control, old, rpc, bridge, create=False):
    n = s['restored_days']; seed = s['seed']
    t = load(folder/'transport.json')
    assert t['exit_code'] == 0 and t['error'] is None and t['day'] == n+2
    assert not t['assignment_posts'] and len(t['new_actions']) == 2
    original = load(SOURCE/f'A/candidate/{seed}.transport.json')
    rebuilt = restored_server(old, rpc)(original, bridge, n)
    assert t['restored'] == rebuilt.restored
    own, agents, ledger = list(rebuilt.own), rebuilt.agents, rebuilt.ledger
    for i, a in enumerate(t['new_actions'], n):
        state = a['state']; assert a['wire_day'] == state['day'] == i
        assert state['agents'] == agents
        assert state['traffics'] == rpc.traffic_for(rebuilt.setup, i, own, original['case']['external_road_footprints'])
        checked = bridge.request({'op': 'step', 'setup': rebuilt.setup,
            'state': dict(state, day=i+1, endsAt=state['endsAt']//1000), 'ledger': ledger, 'plan': a['plan']})
        assert checked == a['validated'] and checked['ok'] and checked['agrees']
        assert a['deadline_margin_ms'] > 0
        agents, ledger = checked['agents'], checked['ledger']; own.append(checked['road_footprint'])
    assert own == t['own']
    posts = [q for q in t['requests'] if q['query']['method'] == 'POST']
    assert len(posts) == 2 and [q['wire_day'] for q in posts] == [n, n+1]
    assert all(q['query']['path'].endswith('/actions') and q['response']['status'] == 200 for q in posts)
    before = old.prefix_bytes(SOURCE/f'A/candidate/{seed}.replay.jsonl', n)
    assert hashlib.sha256(before).hexdigest().upper() == s['prefix_sha256']
    raw = (folder/'replay.jsonl').read_bytes(); assert raw.startswith(before)
    events = [json.loads(x) for x in raw[len(before):].splitlines()]
    by = {}
    for e in events: by.setdefault(e['kind'], []).append(e)
    forbidden = ('resource_marginal', 'actions_deadline_skip', 'actions_server_wait', 'actions_fallback',
                 'actions_recovery_wait', 'virtual_parent_drop', 'virtual_parent_dropped', 'action_transport_retry')
    assert not set(forbidden).intersection(by)
    for kind in ('day_state', 'decision', 'protected_slack', 'actions', 'action_result', 'checkpoint_actions'):
        assert len(by.get(kind, [])) == 2, kind
    short = by['protected_slack'][0]['body']; check_short_guard(short)
    assert by['protected_slack'][1]['body']['publicContinuationAuthorized'] is True
    main, responses = [], []
    for i in range(2):
        d = by['decision'][i]['body']['decision']; st = by['day_state'][i]; ack = by['action_result'][i]
        assert d['dayNumber'] == n+i+1 and st['body']['day'] == n+i
        assert not d['emergency'] and d['candidate']['simulation']['valid']
        assert 0 < d['deadline']['totalMs'] <= 5000 and d['timing']['totalMs'] <= 5000
        assert ack['status'] == 200 and ack['body']['valid'] and ack['body']['day'] == n+i+1
        assert st['atUnixMs'] < st['body']['endsAt'] and ack['atUnixMs'] <= st['body']['endsAt']
        assert not any(v for k, v in by['protected_slack'][i]['body'].items() if k.endswith('Failure'))
        main.append(d['timing']['totalMs']); responses.append(ack['atUnixMs']-st['atUnixMs'])
    assert (folder/'stderr').stat().st_size == 0
    path = folder/'replay-check.txt'
    if not path.exists():
        assert create
        q = subprocess.run([str(BASE/'btc360-plain'), 'replay-check', '--replay', str(folder/'replay.jsonl')],
            capture_output=True, text=True, timeout=60)
        with path.open('x') as f: f.write(q.stdout+q.stderr)
        assert q.returncode == 0 and not q.stderr
    check = path.read_text()
    assert f'summary days={n+2} reconciled_transitions={n+1}' in check
    assert f'resume accepted_days={n+2} last_wire_day={n+1}' in check
    row = {'seed': seed, 'control': control, 'new_acks': 2, 'restored_days': n, 'full_days': n+2,
        'transitions': n+1, 'main_ms': main, 'response_ms': responses, 'short_window_ms': short['outerWindowMs'],
        'short_authorized': False, 'prefix_byte_identical': True, 'new_resource_frames': 0,
        'files': {p.name: sha(p) for p in sorted(folder.iterdir()) if p.is_file() and p.name != 'case_complete.json'}}
    if create: write(folder/'case_complete.json', row)
    else: assert load(folder/'case_complete.json') == row
    return row


def live(s, control, old, rpc, bridge):
    seed, n = s['seed'], s['restored_days']; folder = WORK/'sessions'/f'{seed}-{control}'
    folder.mkdir(parents=True, exist_ok=False)
    state = restored_server(old, rpc)(load(SOURCE/f'A/candidate/{seed}.transport.json'), bridge, n)
    before = old.prefix_bytes(SOURCE/f'A/candidate/{seed}.replay.jsonl', n)
    assert hashlib.sha256(before).hexdigest().upper() == s['prefix_sha256']
    with (folder/'replay.jsonl').open('xb') as f: f.write(before)
    env = os.environ.copy(); env['HEXUDON_TOKEN'] = 'synthetic-loopback-only-no-credential'
    env['UDON_RESOURCE_MARGINAL_357'] = control
    match = f'm-365-{seed}-{control}'
    command = [str(BASE/'btc360-plain'), 'http', '--url', 'http://127.0.0.1:352', '--match', match,
               '--response-ms', '5000', '--poll-ms', '220', '--replay', str(folder/'replay.jsonl')]
    output = queue.Queue(); requests = []; error = None; last = 0; started = time.monotonic()
    with (folder/'stdout').open('x') as out, (folder/'stderr').open('x') as err:
        p = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err, text=True, env=env)
        print('started365', seed, control, p.pid, flush=True)
        def read():
            for line in p.stdout: output.put(line)
            output.put(None)
        reader = threading.Thread(target=read, daemon=True); reader.start()
        try:
            while True:
                assert time.monotonic()-started < 120
                line = output.get(timeout=30)
                if line is None: break
                q = json.loads(line)
                if q.get('transportRPC') != 352:
                    assert q.get('synthetic') is True and state.day == n+2
                    out.write(line); out.flush(); continue
                assert q['id'] == last+1; last = q['id']
                response = rpc.dispatch(state, q, match)
                requests.append({'query': q, 'response': response, 'wire_day': state.day})
                p.stdin.write(json.dumps(response, separators=(',', ':'))+'\n'); p.stdin.flush()
            p.wait(timeout=10)
        except Exception as exc: error = repr(exc)
        finally: p.stdin.close(); reader.join(timeout=1)
    write(folder/'transport.json', {'restored': state.restored, 'new_actions': state.actions, 'requests': requests,
        'own': state.own, 'error': error, 'exit_code': p.poll(), 'day': state.day, 'assignment_posts': state.assignment_posts})
    assert error is None and p.poll() == 0, error
    p.stdout.close()
    return validate(folder, s, control, old, rpc, bridge, create=True)


def run(check=False):
    m, old, rpc = dependencies()
    if not check: assert not (WORK/'sessions').exists(), 'do not rerun completed or ambiguous sessions'
    rows = []
    with rpc.BridgeContext() as bridge:
        for s in m['sessions']:
            for control in m['controls']:
                folder = WORK/'sessions'/f"{s['seed']}-{control}"
                rows.append(validate(folder, s, control, old, rpc, bridge) if check else live(s, control, old, rpc, bridge))
                print('verified365', s['seed'], control, flush=True)
    dependencies()
    report = {'experiment': ID, 'complete': True, 'sessions': len(rows), 'rows': rows,
        'stage_sha256': sha(WORK/'stage365.json'), 'guard_tests': tests(), 'score_authority': False,
        'expired_waits': 0, 'safety_failures': 0}
    for k in ('new_acks', 'restored_days', 'full_days', 'transitions'):
        report[k] = sum(r[k] for r in rows); assert report[k] == m['expected_'+k]
    assert report['sessions'] == m['expected_sessions']
    if check: assert load(WORK/'complete365.json') == report
    else: write(WORK/'complete365.json', report)
    print('complete365', sha(WORK/'complete365.json'), flush=True)


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('mode', choices=('stage', 'test', 'run', 'check')); a = p.parse_args()
    if a.mode == 'stage': stage()
    elif a.mode == 'test': print(json.dumps(tests()))
    else: run(a.mode == 'check')
