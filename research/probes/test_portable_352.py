import unittest
from pathlib import Path
import prepare_portable_352 as p
from process_runtime_352 import dispatch

class Contract(unittest.TestCase):
    def test_complete_policy_preserved(self):
        shim=(p.ROOT/'research/probes/process_rpc_352.hpp').read_text()
        roots=[p.ROOT/'src/btc_main.cpp',p.ROOT/'artifacts/research/350/source/src/btc_main.cpp']
        if (p.ROOT/'input352.json').exists():roots=[p.ROOT/s/'src/btc_main.original352.txt' for s in ('parent','candidate')]
        for root in roots:
            text=root.read_text(encoding='utf-8');out=p.portable(text,shim)
            self.assertIn(p.policy(text),out)
            self.assertEqual(out.count('void run_http('),1)
            for k in ('acknowledge_submitted','precompute_until','prove_until','publicContinuationAuthorized','post_until_deadline','checkpointLedger','virtualLedger'):
                self.assertIn(k,p.policy(text))
    def test_rpc_bytes_and_endpoint(self):
        class Case:
            def post(self,e,b):self.got=(e,b);return 200,{'valid':True,'day':2}
        case=Case();q={'transportRPC':352,'id':1,'method':'POST','path':'/api/v1/matches/m-1/actions','hasBody':True,'bodyRaw':'[[-6],[-6],[-6]]','ioTimeoutMs':750}
        r=dispatch(case,q,'m-1');self.assertEqual(case.got,('actions',[[-6],[-6],[-6]]))
        self.assertEqual(r,{'id':1,'status':200,'bodyRaw':'{"valid":true,"day":2}'})
        with self.assertRaises(ValueError):dispatch(case,{**q,'path':'/api/v1/matches/m-2/actions'},'m-1')
    def test_rpc_get_status_and_no_body(self):
        class Case:
            def get(self,e):return 425,{'reason':e}
        q={'transportRPC':352,'id':7,'method':'GET','path':'/api/v1/matches/m-1/state','hasBody':False,'bodyRaw':'','ioTimeoutMs':0}
        self.assertEqual(dispatch(Case(),q,'m-1')['status'],425)
        with self.assertRaises(ValueError):dispatch(Case(),{**q,'hasBody':True},'m-1')
    def test_no_new_policy_knobs(self):
        s=(p.ROOT/'research/probes/process_rpc_352.hpp').read_text()
        for k in ('solve_day','precompute_until','daySeconds','networkFloor','HEXUDON_TOKEN'):
            self.assertNotIn(k,s)
        self.assertIn('synthetic-loopback-only-no-credential',s)

if __name__=='__main__':unittest.main()
