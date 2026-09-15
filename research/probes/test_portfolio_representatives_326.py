import copy
import json
import subprocess
import unittest
from test_http_prefix_oracle_321 import fixture
from portfolio_representatives_326 import ROOT, representatives, dominates, strict
from run_http_baseline_314 import Bridge
from http_prefix_option_loss_321 import close_bridge
from build_portfolio_oracle_326 import transform

PROBE=ROOT/"artifacts/research/326/portfolio_oracle.exe"

class Contract(unittest.TestCase):
    def ask(self,q):
        p=subprocess.run([str(PROBE)],input=json.dumps(q)+'\n',capture_output=True,text=True,timeout=20)
        self.assertEqual(p.returncode,0,p.stderr);self.assertFalse(p.stderr)
        return json.loads(p.stdout)

    def make(self,stock=2):
        q=fixture()
        for s in q['setup']['spots']:s['stocks']=stock
        bridge=Bridge(ROOT/'artifacts/research/324/claims_bridge.exe')
        def col(agent,actions,id):
            plan=[[-6],[-6],[-6]];plan[agent]=actions
            r=bridge.request({**q,'op':'step','plan':plan});self.assertTrue(r['ok'],r)
            return {'agent':agent,'columnId':id,'contingencyBundle':-1,'plan':[actions],
                'terminalCell':r['agents'][agent]['pos'],'terminalFuel':r['agents'][agent]['fuel'],
                'priority':100-id,'estimatedServings':sum(c['agent']==agent for c in r['claims']),
                'firstVisits':[{'spot':c['spot'],'claimed':True,'brandIndex':c['spot']} for c in r['claims'] if c['agent']==agent]}
        try:
            original=[[col(0,[2,5,2],1)],[col(1,[5,2,5],2)],[col(2,[-6],3)]]
            raw=[original[0]+[col(0,[2,-4],4)],original[1]+[col(1,[5,-4],5)],original[2]]
        finally:close_bridge(bridge)
        rep,cert,stats=representatives(original,raw)
        q.update(portfolios={'legacy':original,'merged':original,'raw-legacy':raw,'raw-expanded':raw,'representatives':rep},certificates=cert)
        return q,stats

    def test_complete_stock_fuel_and_mapping_contract(self):
        for stock in (1,2,3):
            q,stats=self.make(stock);r=self.ask(q);self.assertTrue(r['ok'],r)
            p=r['portfolios'];self.assertGreaterEqual(tuple(p['representatives']['score']),tuple(p['legacy']['score']))
            self.assertEqual(p['raw-legacy']['score'],p['representatives']['score'])
            self.assertEqual(r['certificates_checked'],3)
            self.assertEqual([s['replaced'] for s in stats],[1,1,0])
            self.assertTrue(all(len(v['days'])==4 for v in p.values()))

    def test_safe_wait_bundle_and_invalid_certificate(self):
        q,_=self.make();col=q['portfolios']['legacy'][0][0]
        altered=copy.deepcopy(col);altered['terminalCell']+=1
        self.assertFalse(dominates(altered,col))
        altered=copy.deepcopy(col);altered['terminalFuel']-=1
        self.assertFalse(dominates(altered,col))
        altered=copy.deepcopy(col);altered['firstVisits']=[]
        self.assertFalse(dominates(altered,col))
        altered=copy.deepcopy(col);altered['contingencyBundle']=7
        rep,cert,_=representatives([[altered]],[[col]])
        self.assertTrue(cert[0]['unchanged']);self.assertEqual(rep,[[altered]])
        self.assertFalse(strict(col,col))
        q['certificates'][0]['representative']['terminalFuel']+=1
        self.assertFalse(self.ask(q)['ok'])

    def test_scope_and_no_partial_bundle_unlock(self):
        q,_=self.make()
        for pools in q['portfolios'].values():
            for c in pools[1]:c['contingencyBundle']=1
            for c in pools[2]:c['contingencyBundle']=1
        q['certificates']=[]
        self.assertFalse(self.ask(q)['ok'])
        q,_=self.make();q['state']['agents'][0]['kind']=1;self.assertFalse(self.ask(q)['ok'])

    def test_mechanical_solver_reuse(self):
        self.assertEqual(transform(),(ROOT/'artifacts/research/326/portfolio_oracle_326.cpp').read_text())

if __name__=='__main__':unittest.main()
