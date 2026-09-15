"""Research HTTP lifecycle, variable public windows, native/fixed roles, own traffic.

External footprints are frozen aggregate traffic scenarios, not actual opponent
policies. Never describe this loopback server as official BTC or timing authority.
"""
from collections import defaultdict
import copy
import json
import os
from pathlib import Path
import subprocess
import threading
import time
from http.server import HTTPServer
from run_http_baseline_314 import ROOT, EMPTY_LEDGER, Bridge, digest, handler_for
from run_pair_score_341 import require, load, write_new, pricing_safety


def traffic_for(setup, day, own, external):
    """Zero-based day; mass comparison equals dividing by teams, without rounding."""
    cells = [v for row in setup['map']['cells'] for v in row]
    require(len(own) == day, 'own traffic history must be committed exactly once per prior day')
    require(len(external) == len(setup['daySteps']), 'external traffic day count')
    out = []
    for cell, terrain in enumerate(cells):
        if terrain != 1: continue
        mass = sum(own[d][cell]+external[d][cell] for d in range(max(0,day-2),day))
        status = 2 if mass >= setup['players']*setup['jammedThreshold'] else (
            1 if mass >= setup['players']*setup['busyThreshold'] else 0)
        out.append({'pos':cell,'status':status})
    return out


class ProtectedCase:
    def __init__(self, case, bridge, clock=time.monotonic, wall=time.time):
        self.case, self.setup, self.bridge = case, case['setup'], bridge
        self.clock, self.wall = clock, wall
        require(bridge.request({'op':'config','setup':self.setup}).get('ok'), 'invalid protected setup')
        size = self.setup['map']['width']*self.setup['map']['height']
        external = case['external_road_footprints']
        require(len(external) == len(self.setup['daySteps']) and all(len(d)==size for d in external), 'traffic shape')
        require(all(type(n) is int and n>=0 for d in external for n in d), 'invalid external traffic')
        require(all(type(t) is int and t>0 for t in self.setup['daySeconds']), 'invalid day window')
        self.day = 0; self.ledger = copy.deepcopy(EMPTY_LEDGER); self.score = [0,0,0]
        self.roles = None; self.agents = None; self.assignment_posts = []
        self.deadline = self.ends_at = self.window_ms = self.pending = self.plan = self.last_state = None
        self.actions = []; self.requests = []; self.own = []; self.failure = None
        if case['role'] == 'fixed-all-Patrol': self.assign([0]*len(self.setup['agents']))
        else: require(case['role']=='native','unknown role mode')

    def assign(self, roles):
        require(self.roles is None and self.day==0 and self.deadline is None,'assignment already frozen')
        require(self.bridge.request({'op':'roles','setup':self.setup,'roles':roles}).get('ok'),'invalid roles')
        self.roles = list(roles)
        self.agents = [{'kind':k,'pos':p,'fuel':self.setup['fuelLimits']} for k,p in zip(roles,self.setup['agents'],strict=True)]

    def advance(self):
        if self.deadline is None or self.clock() < self.deadline: return
        require(self.pending is not None,'day expired without accepted dual-valid action')
        self.agents, self.ledger, self.score = self.pending['agents'],self.pending['ledger'],self.pending['score']
        self.own.append(self.pending['road_footprint'])
        self.day += 1
        self.deadline = self.ends_at = self.window_ms = self.pending = self.plan = None

    def get(self, endpoint):
        self.advance()
        if endpoint == 'setup': return 200,self.setup
        if endpoint == 'start': return (200,{'started':True,'synthetic':True}) if self.roles is not None else (425,{})
        if endpoint == 'result':
            return (200,{'synthetic':True,'score':self.score,'standings':[]}) if self.day==len(self.setup['daySteps']) else (425,{})
        if endpoint != 'state': return 404,{}
        if self.day==len(self.setup['daySteps']): return 404,{'reason':'synthetic-complete'}
        if self.roles is None or self.pending is not None: return 425,{'reason':'synthetic-await-state'}
        if self.deadline is None:
            self.window_ms = self.setup['daySeconds'][self.day]*1000
            self.deadline = self.clock()+self.window_ms/1000
            self.ends_at = int(self.wall()*1000)+self.window_ms
            self.last_state = {'day':self.day,'endsAt':self.ends_at,'agents':copy.deepcopy(self.agents),
                'others':[], 'traffics':traffic_for(self.setup,self.day,self.own,self.case['external_road_footprints'])}
        return 200,self.last_state

    def post(self, endpoint, body):
        if endpoint == 'assignment':
            require(self.case['role']=='native','fixed roles must not be overwritten')
            if self.roles is None: self.assign(body)
            else: require(body==self.roles and self.day==0 and self.deadline is None,'assignment retry changed')
            self.assignment_posts.append(copy.deepcopy(body))
            return 200,{'valid':True,'synthetic':True}
        require(endpoint=='actions' and self.deadline is not None,'action without delivered state')
        require(self.clock()<self.deadline,'action after authoritative deadline')
        if self.pending is not None:
            require(body==self.plan,'non-idempotent action retry')
            return 200,{'valid':True,'day':self.day+1,'synthetic':True}
        checked=self.bridge.request({'op':'step','setup':self.setup,
            'state':dict(self.last_state,day=self.day+1,endsAt=self.ends_at//1000),
            'ledger':self.ledger,'plan':body})
        require(checked.get('ok') and checked.get('agrees'),'action dual validation failed')
        require(len(checked['road_footprint'])==self.setup['map']['width']*self.setup['map']['height'],'footprint shape')
        self.pending,self.plan=checked,copy.deepcopy(body)
        self.actions.append({'wire_day':self.day,'state':copy.deepcopy(self.last_state),'plan':copy.deepcopy(body),
            'validated':checked,'arrival_ms':round((self.clock()-(self.deadline-self.window_ms/1000))*1000,3),
            'deadline_margin_ms':round((self.deadline-self.clock())*1000,3)})
        return 200,{'valid':True,'day':self.day+1,'synthetic':True}


def operational(replay, case, side):
    require(side in ('parent','candidate'),'unknown binary side')
    by=defaultdict(list)
    for line in Path(replay).read_text().splitlines():
        e=json.loads(line);by[e['kind']].append(e)
    count=len(case['setup']['daySteps'])
    for k in ('day_state','decision','protected_slack','actions','action_result'):
        require(len(by[k])==count,'missing/duplicated '+k)
    forbidden={'actions_fallback','actions_recovery_wait','actions_server_wait','actions_deadline_skip',
               'actions_transport_retry','virtual_parent_dropped'}
    require(not forbidden.intersection(by),'fallback/retry/state-boundary failure')
    require(len(by['assignment'])==len(by['assignment_result'])==1,'assignment identity')
    require(by['assignment_result'][0]['body'].get('valid') is True,'assignment rejected')
    records=[]
    for i in range(count):
        state=by['day_state'][i];day=by['decision'][i]['body']['decision'];ack=by['action_result'][i]
        slack=by['protected_slack'][i]['body']
        require(state['body']['day']==i and day['dayNumber']==i+1,'day identity')
        require(ack['status']==200 and ack['body'].get('valid') is True and ack['body']['day']==i+1,'invalid/stale ACK')
        require(not day['emergency'] and day['candidate']['simulation']['valid'],'invalid/emergency')
        require(day['audit']['selectionReason']=='certified-undominated-current-floor','changed selection policy')
        require(day['deadline']['networkMs']>=1600 and day['deadline']['totalMs']<=5000,'main/transport budget changed')
        require(day['timing']['totalMs']<=5000,'main exceeded5000ms')
        require(not any(v for k,v in slack.items() if k.endswith('Failure')),'checkpoint/continuation failure')
        require(ack['atUnixMs']<=state['body']['endsAt'],'late ACK')
        require(bool(slack.get('publicContinuationAuthorized',False))==(case['setup']['daySeconds'][i]>5),
                'public-window authority mismatch')
        records.append({'day':i+1,'main_ms':day['timing']['totalMs'],'main_budget_ms':day['deadline']['totalMs'],
            'response_ms':ack['atUnixMs']-state['atUnixMs'],'slack':slack,
            'reconciled_authoritative_state':day['reconciledAuthoritativeState'],'cache_repair':day['cacheRepair']})
    # The frozen canonical parent predates horizonPricing telemetry. Do not
    # fabricate zero counters, and never excuse missing telemetry in candidate.
    if side == 'candidate': pricing_safety(replay)
    else:
        require(all('horizonPricing' not in e['body']['decision']['profile']
                    for e in by['decision']),'parent telemetry identity changed')
    return {'safety_pass':True,'days':records,'roles':by['assignment'][0]['body'],
            'session_checkpoints':len(by['session_checkpoint'])}


def validate_side(case, side, directory):
    prefix=directory/side/str(case['seed']);row=load(prefix.with_suffix('.result.json'))
    require(row['kind']=='case_complete' and row['seed']==case['seed'] and row['failure'] is None,'bad atomic row')
    require(row['case_sha256']==digest_case(case),'case provenance mismatch')
    count=case['days']
    require(row['actions']==count and row['transitions']==count-1,'incomplete side')
    for suffix,key in (('.replay.jsonl','replay_sha256'),('.transport.json','transport_sha256'),('.replay-check.txt','replay_check_sha256')):
        require(digest(prefix.with_suffix(suffix))==row[key],'side artifact hash mismatch')
    require(prefix.with_suffix('.stderr').stat().st_size==0,'child stderr')
    operational(prefix.with_suffix('.replay.jsonl'),case,side)
    return row


def digest_case(case):
    import hashlib
    return hashlib.sha256(json.dumps(case,sort_keys=True,separators=(',',':')).encode()).hexdigest().upper()


def run_case(case, binary, directory, bridge, experiment, side):
    prefix=directory/str(case['seed']);replay=prefix.with_suffix('.replay.jsonl')
    state=ProtectedCase(case,bridge)
    events=[('synthetic_fixture',{'experiment':experiment,'seed':case['seed']})]
    if state.roles is not None: events += [('assignment',state.roles),('assignment_result',{'valid':True,'synthetic':True})]
    with replay.open('x',encoding='utf-8',newline='\n') as f:
        for kind,body in events:f.write(json.dumps({'kind':kind,'body':body,'status':200,'atUnixMs':int(time.time()*1000)})+'\n')
    match=f"m-{case['seed']}"
    server=HTTPServer(('127.0.0.1',0),handler_for(state,match))
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    env=os.environ.copy();env['HEXUDON_TOKEN']='synthetic-loopback-only-no-credential'
    command=[str(ROOT/binary),'http','--url',f'http://127.0.0.1:{server.server_port}',
             '--match',match,'--response-ms','5000','--poll-ms','220','--replay',str(replay)]
    try:
        with prefix.with_suffix('.stdout').open('x') as out,prefix.with_suffix('.stderr').open('x') as err:
            process=subprocess.Popen(command,stdout=out,stderr=err,env=env,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
            rc=process.wait(timeout=sum(case['setup']['daySeconds'])+90)
    finally:server.shutdown();server.server_close();thread.join(timeout=5)
    transport={'synthetic':True,'case':case,'actions':state.actions,'requests':state.requests,
               'roles':state.roles,'assignment_posts':state.assignment_posts,'own_road_footprints':state.own,
               'failure':state.failure,'score':state.score}
    write_new(prefix.with_suffix('.transport.json'),transport)
    require(rc==0 and state.failure is None and prefix.with_suffix('.stderr').stat().st_size==0,'HTTP side failed; preserve evidence')
    require(state.day==case['days'] and len(state.actions)==case['days'],'incomplete lifecycle')
    require((len(state.assignment_posts)==1)==(case['role']=='native'),'native assignment not exercised exactly once')
    operational(replay,case,side)
    for i,action in enumerate(state.actions):
        require(action['state']['traffics']==traffic_for(case['setup'],i,state.own[:i],case['external_road_footprints']), 'traffic reconciliation')
    check=subprocess.run([str(ROOT/binary),'replay-check','--replay',str(replay)],capture_output=True,text=True,
                         timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    with prefix.with_suffix('.replay-check.txt').open('x',encoding='utf-8') as f:f.write(check.stdout+check.stderr)
    require(check.returncode==0 and f"summary days={case['days']} reconciled_transitions={case['days']-1}" in check.stdout,
            'replay-check did not reconcile complete lifecycle')
    write_new(prefix.with_suffix('.result.json'),{'kind':'case_complete','seed':case['seed'],'case_sha256':digest_case(case),
        'http_score':state.score,'actions':case['days'],'transitions':case['days']-1,'failure':None,
        'replay_sha256':digest(replay),'transport_sha256':digest(prefix.with_suffix('.transport.json')),
        'replay_check_sha256':digest(prefix.with_suffix('.replay-check.txt'))})
