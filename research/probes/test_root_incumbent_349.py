import copy
import unittest
from run_http_baseline_314 import ROOT, Bridge
import test_pair_resource_response_336 as fixtures


class Contract(unittest.TestCase):
    def setUp(self):
        self.p=Bridge(ROOT/'artifacts/research/349/probe.exe')
        self.control=Bridge(ROOT/'artifacts/research/340/probe.exe')
        self.judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')

    def tearDown(self):
        for b in (self.p,self.control,self.judge):
            b.close();b.process.stdout.close();b.process.stderr.close()

    def q(self):
        return {**fixtures.Contract.q(self),'op':'price','expired':False,'corrupt':False}

    def test_complete_parity(self):
        for stock in (1,2,3):
            q=self.q()
            for s in q['setup']['spots']:s['stocks']=stock
            a=self.p.request(q);b=self.control.request(q)
            self.assertTrue(a['ok'],a);self.assertEqual(a['completed'],1)
            self.assertEqual(a['score'],b['score']);self.assertEqual(a['plans'],b['plans'])
            self.assertEqual(a['observation']['completed_pairs'],3)
            self.assertIsNone(a['observation']['score']);self.assertTrue(a['originalUnchanged'])

    def test_early_expiry_and_every_zero_cap(self):
        for name in ('settled','created','actions','memo','transitions','queries'):
            a=self.p.request({**self.q(),'limit':name,'value':0})
            self.assertTrue(a['ok']);self.assertEqual(a['exhausted'],1)
            self.assertIsNone(a['observation']['score']);self.assertEqual(a['plans'],self.q()['plans'])
        a=self.p.request({**self.q(),'expired':True})
        self.assertTrue(a['ok']);self.assertEqual(a['exhausted'],1)
        self.assertIsNone(a['observation']['score'])

    def test_corrupt_is_not_usable_certificate(self):
        a=self.p.request({**self.q(),'corrupt':True})
        self.assertTrue(a['ok']);self.assertEqual(a['failures'],1)
        self.assertIsNone(a['observation']['score']);self.assertTrue(a['originalUnchanged'])

    def test_exhaustion_after_complete_pair_does_not_change_return(self):
        q=self.q()
        q['state']['agents'][0]['pos']=16
        q['state']['agents'][1]['pos']=19
        full=self.p.request(q);hits=0;root_only=0
        # Exhaustive tiny-fixture fault boundaries, not production cap tuning.
        for n in range(full['transitions']+1):
            a=self.p.request({**q,'limit':'transitions','value':n})
            self.assertTrue(a['ok']);self.assertTrue(a['originalUnchanged'])
            obs=a['observation']
            if obs['score'] is None:continue
            hits+=1
            root_only+=obs['completed_pairs']==0
            self.assertEqual(a['exhausted'],1);self.assertEqual(a['plans'],q['plans'])
            self.assertGreater(obs['score'],a['baseline']);self.assertGreaterEqual(obs['completed_pairs'],0)
            state,ledger=copy.deepcopy(q['state']),copy.deepcopy(q['ledger'])
            for plan in obs['plans']:
                r=self.judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':plan})
                self.assertTrue(r['ok'] and r['agrees'])
                state={**state,'day':state['day']+1,'agents':r['agents']};ledger=r['ledger']
            self.assertEqual(r['score'],obs['score'])
        self.assertGreater(hits,0,'control did not reach the lifetime boundary')
        self.assertGreater(root_only,0,'test only observed completed-pair retention')

    def test_terminal_root_reconstruction(self):
        q=self.q();q['state']['day']=4;q['plans']=q['plans'][:1]
        q['state']['agents'][0]['pos']=16;q['state']['agents'][1]['pos']=19
        full=self.p.request(q);hits=0
        for n in range(full['transitions']+1):
            a=self.p.request({**q,'limit':'transitions','value':n});obs=a['observation']
            self.assertTrue(a['ok'])
            if obs['score'] is None:continue
            hits+=1
            self.assertEqual(len(obs['plans']),1)
            r=self.judge.request({'op':'step','setup':q['setup'],'state':q['state'],'ledger':q['ledger'],'plan':obs['plans'][0]})
            self.assertTrue(r['ok'] and r['agrees']);self.assertEqual(r['score'],obs['score'])
        self.assertGreater(hits,0)


if __name__=='__main__':unittest.main()
