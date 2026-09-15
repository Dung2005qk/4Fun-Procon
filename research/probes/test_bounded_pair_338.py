"""Joint response correctness, resource faults, identity and old-caller parity."""
import copy
import itertools
import unittest
from run_http_baseline_314 import ROOT, Bridge
from http_prefix_option_loss_321 import load, close_bridge
import test_bounded_horizon_332 as single_tests
import test_pair_resource_response_336 as pair_tests


class Contract(single_tests.Contract):
    def setUp(self):
        self.p=Bridge(ROOT/'artifacts/research/338/probe.exe')
        self.parent=Bridge(ROOT/'artifacts/research/332/parent-probe.exe')
        self.judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
        self.claims=Bridge(ROOT/'artifacts/research/324/claims_bridge.exe')

    def test_whole_horizon_against_independent_walks_and_stock(self):
        for stock in (1,2,3):
            q=pair_tests.Contract.q(self);q.update(op='price',expired=False,corrupt=False)
            for spot in q['setup']['spots']:spot['stocks']=stock
            out=self.p.request(q)
            self.assertTrue(out['ok'],out);self.assertEqual(out['completed'],1,out)
            self.assertEqual(out['failures'],0);self.assertTrue(out['originalUnchanged'])
            self.assertEqual(out['score'],max(pair_tests.Contract.brute(self,q,pair) for pair in itertools.combinations(range(3),2)))

    def test_complete_consumed_control_capability_matches330(self):
        # Consumed capability examples only. Incomplete must retain original,
        # never pretend to have matched the independent336 conditional optimum.
        m=load(ROOT/'research/holdouts/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.json')
        for c in m['cases']:
            old=load(ROOT/('research/evidence/ATTR-W1-PAIR-RESOURCE-RESPONSE-336/'+c['id']+'.result.json'))
            q={**c['request'],'op':'price','expired':False,'corrupt':False}
            out=self.p.request(q);self.assertTrue(out['ok'],out)
            self.assertEqual(out['failures'],0,out);self.assertTrue(out['originalUnchanged'])
            if out['completed']:
                self.assertEqual(out['score'],max(r['score'] for r in old['output']['responses']))
            else:
                self.assertEqual(out['exhausted'],1,out);self.assertEqual(out['plans'],q['plans'])

    def test_every_shared_cap_is_fail_closed_and_cannot_be_raised(self):
        q=self.q()
        for field in ('settled','created','actions','memo','transitions','queries'):
            out=self.p.request({**q,'limit':field,'value':0})
            self.assertTrue(out['ok'],(field,out));self.assertEqual(out['exhausted'],1,(field,out))
            self.assertEqual(out['completed'],0);self.assertEqual(out['improvements'],0)
            self.assertEqual(out['plans'],q['plans']);self.assertTrue(out['originalUnchanged'])
            out=self.p.request({**q,'limit':field,'value':10**9})
            self.assertTrue(out['ok']);self.assertEqual(out['completed'],1)
            self.assertLessEqual(out[field],{'settled':32768,'created':196609,'actions':262144,'memo':8192,'transitions':262144,'queries':1024}[field])

    def test_whole_physical_identity_not_sorted_away(self):
        q=pair_tests.Contract.q(self);q.update(op='price',expired=False,corrupt=False)
        before=self.p.request(q)
        q['state']['agents'].reverse()
        for plan in q['plans']:plan.reverse()
        after=self.p.request(q)
        self.assertEqual(after['score'],before['score']);self.assertEqual(after['completed'],1)
        state=q['state'];ledger=q['ledger']
        for plan in after['plans']:
            r=self.judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':plan})
            self.assertTrue(r['ok'] and r['agrees']);self.assertEqual(r['agents'][0],q['state']['agents'][0])
            state={**state,'day':state['day']+1,'agents':r['agents']};ledger=r['ledger']
        self.assertEqual(r['score'],after['score'])

    def test_frontier_matches_complete_walk_resource_envelope(self):
        # Independently enumerate all action sequences with unit WAIT / source-
        # cost moves in a horizontal Plain corridor, including initial pickup.
        for steps in (1,2,3,4):
            for pos in (16,17,19):
                q=self.q();q['setup']['daySteps']=[steps]*4
                q['state']['agents'][0].update(pos=pos,fuel=2)
                q.update(op='frontier',settled=32768,created=196609,actions=262144)
                out=self.p.request(q);self.assertTrue(out['complete'],out)
                expected={}
                for n in range(steps+1):
                    for seq in itertools.product((-1,2,5),repeat=n):
                        used=sum(1 if a<0 else 2 for a in seq)
                        if used>steps:continue
                        plan=[list(seq)+([-(steps-used)] if used<steps else []),[-steps],[-steps]]
                        r=self.claims.request({'op':'step','setup':q['setup'],'state':q['state'],'ledger':q['ledger'],'plan':plan})
                        if not r.get('ok'):continue
                        self.assertTrue(r['agrees']);mask=0
                        for claim in r['claims']:
                            if claim['agent']==0:mask|=1<<claim['spot']
                        key=(r['agents'][0]['pos'],mask);fuel=2-r['agents'][0]['fuel']
                        expected[key]=min(expected.get(key,100),fuel)
                got={}
                for r in out['routes']:
                    key=(r['cell'],r['mask']);got[key]=min(got.get(key,100),r['fuel'])
                self.assertEqual(got,expected,(steps,pos))


if __name__=='__main__': unittest.main()
