import copy
import json
import subprocess
import unittest
from pool_boundary_value_327 import ROOT, singleton
from http_prefix_option_loss_321 import close_bridge, state
from run_http_baseline_314 import Bridge
from test_http_prefix_oracle_321 import fixture

class Contract(unittest.TestCase):
    def ask(self, binary, q):
        p = subprocess.run([str(ROOT/binary)], input=json.dumps(q)+'\n', text=True, capture_output=True, timeout=30)
        self.assertEqual(p.returncode, 0, p.stderr); self.assertFalse(p.stderr)
        return json.loads(p.stdout)

    def test_singleton_exact_action_and_suffix(self):
        for stock in (1, 2, 3):
            q = fixture(); q['state']['day'] = 2
            for s in q['setup']['spots']: s['stocks'] = stock
            plans = {'move': [[2, -5], [5, -5], [-7]], 'wait': [[-7], [-7], [-7]]}
            bridge = Bridge(ROOT/'artifacts/research/324/claims_bridge.exe')
            checked = {k: bridge.request({**q, 'op': 'step', 'plan': plan}) for k, plan in plans.items()}
            close_bridge(bridge)
            self.assertTrue(all(v['ok'] and v['agrees'] for v in checked.values()))
            q['portfolios'] = {k: singleton(plan, checked[k]) for k, plan in plans.items()}; q['certificates'] = []
            r = self.ask('artifacts/research/326/portfolio_oracle.exe', q)
            self.assertTrue(r['ok'], r)
            for k, plan in plans.items():
                p = r['portfolios'][k]; self.assertEqual(p['days'][0]['plan'], plan)
                self.assertEqual(p['admissible_combinations'], 1); self.assertEqual(p['unique_joint_roots'], 1)
                old = self.ask('artifacts/research/321/probe.exe', {'setup': q['setup'],
                    'state': state(checked[k]['agents'], 3), 'ledger': checked[k]['ledger']})
                self.assertEqual(p['score'], old['score'])
            bad = copy.deepcopy(q); bad['portfolios']['move'][1][0]['contingencyBundle'] = 1
            self.assertFalse(self.ask('artifacts/research/326/portfolio_oracle.exe', bad)['ok'])

    def test_bad_metadata_rejected(self):
        q = fixture(); bridge = Bridge(ROOT/'artifacts/research/324/claims_bridge.exe')
        plan = [[2, -4], [5, -4], [-6]]; actual = bridge.request({**q, 'op':'step', 'plan': plan}); close_bridge(bridge)
        q['portfolios'] = {'bad': singleton(plan, actual)}; q['certificates'] = []
        q['portfolios']['bad'][0][0]['terminalFuel'] += 1
        self.assertFalse(self.ask('artifacts/research/326/portfolio_oracle.exe', q)['ok'])

if __name__ == '__main__': unittest.main()
