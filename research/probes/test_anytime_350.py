import copy
import unittest
from run_http_baseline_314 import ROOT, Bridge
import test_pair_resource_response_336 as fixtures


class Contract(unittest.TestCase):
    def setUp(self):
        self.p=Bridge(ROOT/'artifacts/research/350/probe.exe')
        self.control=Bridge(ROOT/'artifacts/research/347/build/udonshield_pricing_contract_344.exe')
        self.judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')

    def tearDown(self):
        for b in (self.p,self.control,self.judge):
            b.close();b.process.stdout.close();b.process.stderr.close()

    def q(self):
        return {**fixtures.Contract.q(self),'op':'price','expired':False,'corrupt':False}

    def exact(self,q,a):
        self.assertTrue(a['ok'],a);self.assertTrue(a['originalUnchanged'])
        self.assertEqual(a['failures'],0)
        self.assertEqual(a['completed']+a['exhausted'],1)
        self.assertLessEqual(a['anytimeReturns'],a['exhausted'])
        self.assertLessEqual(a['improvements'],a['completed']+a['anytimeReturns'])
        self.assertGreaterEqual(a['certifiedIncumbents'],a['improvements'])
        self.assertGreaterEqual(a['score'],a['baseline'])
        fixed=[i for i in range(len(q['setup']['agents'])) if all(x[i]==y[i] for x,y in zip(a['plans'],q['plans'],strict=True))]
        self.assertGreaterEqual(len(fixed),len(q['setup']['agents'])-2)
        if not a['improvements']:self.assertEqual(a['plans'],q['plans'])
        s,l=copy.deepcopy(q['state']),copy.deepcopy(q['ledger'])
        os,ol=copy.deepcopy(s),copy.deepcopy(l)
        for plan,original in zip(a['plans'],q['plans'],strict=True):
            r=self.judge.request({'op':'step','setup':q['setup'],'state':s,'ledger':l,'plan':plan})
            o=self.judge.request({'op':'step','setup':q['setup'],'state':os,'ledger':ol,'plan':original})
            self.assertTrue(r['ok'] and r['agrees']);self.assertTrue(o['ok'] and o['agrees'])
            for i in fixed:self.assertEqual(r['agents'][i],o['agents'][i])
            s={**s,'day':s['day']+1,'agents':r['agents']};l=r['ledger']
            os={**os,'day':os['day']+1,'agents':o['agents']};ol=o['ledger']
        self.assertEqual(r['score'],a['score'])

    def test_complete_parity(self):
        for stock in (1,2,3):
            q=self.q()
            for s in q['setup']['spots']:s['stocks']=stock
            a=self.p.request(q);b=self.control.request(q);self.exact(q,a)
            self.assertEqual(a['completed'],1);self.assertEqual(b['completed'],1)
            self.assertEqual(a['score'],b['score']);self.assertEqual(a['plans'],b['plans'])

    def test_early_expiry_and_every_zero_cap(self):
        for name in ('settled','created','actions','memo','transitions','queries'):
            q={**self.q(),'limit':name,'value':0};a=self.p.request(q)
            self.exact(q,a);self.assertEqual(a['exhausted'],1)
            self.assertEqual(a['anytimeReturns'],0);self.assertEqual(a['certifiedIncumbents'],0)
        q={**self.q(),'expired':True};a=self.p.request(q);self.exact(q,a)
        self.assertEqual(a['anytimeReturns'],0)

    def test_corrupt_is_not_usable_certificate(self):
        a=self.p.request({**self.q(),'corrupt':True})
        self.assertTrue(a['ok']);self.assertEqual(a['failures'],1)
        self.assertEqual(a['anytimeReturns'],0);self.assertEqual(a['certifiedIncumbents'],0)
        self.assertTrue(a['originalUnchanged'])

    def check_boundaries(self,terminal=False):
        q=self.q();q['state']['agents'][0]['pos']=16;q['state']['agents'][1]['pos']=19
        if terminal:q['state']['day']=4;q['plans']=q['plans'][:1]
        full=self.p.request(q);hits=0;self.exact(q,full)
        for n in range(full['transitions']+1):
            a=self.p.request({**q,'limit':'transitions','value':n});self.exact(q,a)
            self.assertLessEqual(a['transitions'],n)
            if a['anytimeReturns']:
                hits+=1;self.assertEqual(a['exhausted'],1)
                self.assertGreater(a['score'],a['baseline'])
        self.assertGreater(hits,0)

    def test_nonterminal_every_transition_boundary(self):self.check_boundaries()
    def test_terminal_every_transition_boundary(self):self.check_boundaries(True)

    def test_four_physical_agents(self):
        q=self.q()
        q['setup']['agents'].append(copy.deepcopy(q['setup']['agents'][-1]))
        q['setup']['map']['cells'][7][0]=0
        q['setup']['agents'][-1]=56
        q['state']['agents'].append(copy.deepcopy(q['state']['agents'][-1]))
        q['state']['agents'][-1]['pos']=56
        for p in q['plans']:p.append(copy.deepcopy(p[-1]))
        self.exact(q,self.p.request(q))


if __name__=='__main__':unittest.main()
