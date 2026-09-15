"""Read-only descriptive audit of existing BTC wire timestamps; no budget change."""
from refinement_cost_353 import ROOT,sha,load,write,require
import json
import math
NAMES=['m-13627.jsonl','m-13560.jsonl','m-11679.jsonl','m-11670.jsonl',
    'm-11621.jsonl','m-10543.jsonl','m-10542.jsonl','m-10505.jsonl',
    'm-10368.jsonl','m-10335.jsonl','m-9973-ab3d699-series.jsonl','m-9921-ab3d699-series.jsonl']
def main():
    rows=[];files={};counts={};errors=[]
    for name in NAMES:
        p=ROOT/'artifacts/btc'/name;files[name]=sha(p)
        events=[json.loads(l)for l in p.read_text().splitlines()]
        counts[name]={'result_records':sum(e['kind']=='result'for e in events),'actions':sum(e['kind']=='actions'for e in events)}
        slack=None;send=None;retries=0
        for e in events:
            k=e['kind'];b=e['body']
            if k=='protected_slack':slack=e
            elif k in ('actions','actions_fallback','actions_recovery_wait'):send=e;retries=0
            elif k=='action_transport_retry':retries+=b['transportRetries']
            elif k=='action_result' and send:
                row={'file':name,'day':b.get('day'),'valid':b.get('valid'),
                    'http_status':e.get('status'),'action_to_ack_ms':e['atUnixMs']-send['atUnixMs'],'transport_retries':retries}
                if slack:
                    s=slack['body'];deadline=s.get('publicContinuationDeadlineMs')
                    row.update({'slack_record_to_action_ms':send['atUnixMs']-slack['atUnixMs'],
                        'reserve_ms':s['refinementReserveMs'],'public_deadline_reached':s.get('publicContinuationDeadlineReached'),
                        'checkpoint_deadline_reached':s['deadlineReached']})
                    if deadline:
                        row.update({'action_to_authoritative_deadline_ms':deadline+s['refinementReserveMs']-send['atUnixMs'],
                            'slack_record_after_compute_deadline_ms':slack['atUnixMs']-deadline})
                rows.append(row);send=None
            if k in ('actions_deadline_skip','actions_server_wait','actions_recovery_wait','transition_mismatch'):errors.append({'file':name,'kind':k})
    def stats(key):
        a=sorted(r[key]for r in rows if key in r)
        return {'count':len(a),'min':min(a),'median':a[(len(a)-1)//2],
            'p95_empirical':a[math.ceil(.95*len(a))-1],'max':max(a)}if a else None
    report={'source':'Existing user/agent BTC replay logs; exact build/network provenance not inferred.',
        'input_hashes':files,'file_counts':counts,'rows':rows,'errors':errors,
        'stats':{k:stats(k)for k in ('action_to_ack_ms','slack_record_to_action_ms','action_to_authoritative_deadline_ms','slack_record_after_compute_deadline_ms')},
        'retries':sum(r['transport_retries']for r in rows),
        'bad_ack':sum(r['http_status']!=200 or r['valid']is not True for r in rows),
        'limitations':['No timed final-certification-start marker. protected_slack is written AFTER certificate checks.',
            'Fast successful ACK samples do not bound slow-network/resend tails. No p99 guarantee or reserve-policy qualification.',
            '357 frozen budget unchanged. No experiment or matrix score was inspected by this audit.']}
    write(ROOT/'research/evidence/TRANSPORT-RESERVE-1100-357-audit.json',report)
    print(json.dumps({k:report[k]for k in ('stats','retries','bad_ack','errors')},indent=2))
if __name__=='__main__':main()
