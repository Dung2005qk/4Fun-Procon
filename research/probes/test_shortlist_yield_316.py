import copy
import json
import unittest
from run_http_baseline_314 import ROOT, Bridge
from run_shortlist_yield_316 import validate_row


class ShortlistTests(unittest.TestCase):
    def test_current_terminal_mismatch_blocks(self):
        candidate = {"scoreAfterToday": {"lifetimeDistinct": 1, "totalDailyDistinct": 1, "totalServings": 1},
                     "terminalCells": [17], "terminalFuel": [9]}
        output = {"ok": True, "certified": True, "dual_valid_days": 3, "current_score": [1, 1, 1],
                  "current_agents": [{"pos": 18, "fuel": 9}]}
        with self.assertRaisesRegex(ValueError, "terminal mismatch"):
            validate_row(output, candidate)

    def test_false_certificate_blocks(self):
        with self.assertRaisesRegex(ValueError, "incomplete"):
            validate_row({"ok": False}, {})

    def test_isolated_wait_and_invalid_input_contract(self):
        fixture = json.loads((ROOT/"research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json").read_text())["setups"][0]
        request = {"setup": fixture, "state": {"day": 1, "endsAt": 0,
            "agents": [{"kind": 0, "pos": p, "fuel": fixture["fuelLimits"]} for p in fixture["agents"]],
            "others": [], "traffics": []}, "ledger": {"brands": [], "totalDailyDistinct": 0, "totalServings": 0},
            "plan": [[-fixture["daySteps"][0]]]*3, "upper": [5, 20, 60]}
        bridge = Bridge(ROOT/"artifacts/research/316/probe.exe")
        try:
            good = bridge.request(request)
            self.assertTrue(good["ok"], good)
            self.assertTrue(good["certified"])
            self.assertEqual(len(good["future_plans"]), 3)
            self.assertEqual(good["dual_valid_days"], 3)
            self.assertEqual(good["current_score"], [0, 0, 0])
            bad = copy.deepcopy(request)
            bad["plan"][0] = [0, -15]
            self.assertFalse(bridge.request(bad)["ok"])
            bad = copy.deepcopy(request)
            bad["state"]["day"] = 2
            self.assertFalse(bridge.request(bad)["ok"])
        finally:
            bridge.close()
            bridge.process.stdout.close()
            bridge.process.stderr.close()


if __name__ == "__main__":
    unittest.main()
