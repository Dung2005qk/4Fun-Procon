"""Terminal equivalence is exact; nonterminal endpoint/fuel cannot be quotiented."""
import itertools
from run_http_baseline_314 import ROOT, Bridge
import test_bounded_pair_338 as old


class Contract(old.Contract):
    def setUp(self):
        super().setUp()
        self.p.close();self.p.process.stdout.close();self.p.process.stderr.close()
        self.p=Bridge(ROOT/'artifacts/research/339/probe.exe')
        self.control=Bridge(ROOT/'artifacts/research/338/probe.exe')

    def tearDown(self):
        self.control.close();self.control.process.stdout.close();self.control.process.stderr.close()
        super().tearDown()

    def test_stable_complete_plan_equivalence_terminal_and_nonterminal(self):
        for day,stock,positions in itertools.product((3,4),(1,2,3),((17,18,7),(18,17,7))):
            q=self.q();q['state']['day']=day;q['plans']=q['plans'][:5-day]
            for a,p in zip(q['state']['agents'],positions):a['pos']=p
            for s in q['setup']['spots']:s['stocks']=stock
            before=self.control.request(q);after=self.p.request(q)
            self.assertEqual(before['completed'],1,before);self.assertEqual(after['completed'],1,after)
            self.assertEqual(before['score'],after['score']);self.assertEqual(before['plans'],after['plans'])
            self.assertTrue(after['originalUnchanged']);self.assertEqual(after['failures'],0)
            self.assertLess(after['transitions'],before['transitions'])

    def test_quotient_bookkeeping_cannot_escape_action_cap(self):
        q=self.q();q['state']['day']=4;q['plans']=q['plans'][:1]
        full=self.p.request(q);self.assertEqual(full['completed'],1,full)
        for cap in (1,full['actions']-1):
            limited=self.p.request({**q,'limit':'actions','value':cap})
            self.assertEqual(limited['completed'],0,limited);self.assertEqual(limited['exhausted'],1)
            self.assertTrue(limited['originalUnchanged']);self.assertEqual(limited['plans'],q['plans'])


if __name__=='__main__':
    import unittest
    unittest.main()
