import copy
import json
import unittest
from selected_horizon_pricing_331 import ROOT, load, Bridge, close_bridge, selected_context, validate_response


class Contract(unittest.TestCase):
    def setUp(self):
        m = load(ROOT/"research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json")
        index = next(i for i, c in enumerate(m["cases"]) if c["seed"] == 10700100)
        self.setup = m["setups"][index]
        p = ROOT/"research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314/10700100.replay.jsonl"
        self.body = next(e["body"] for e in map(json.loads, p.read_text().splitlines()) if e["kind"] == "decision")
        self.bridge = Bridge(ROOT/"artifacts/research/314/bridge.exe")
        self.probe = Bridge(ROOT/"artifacts/research/330/probe.exe")

    def tearDown(self):
        close_bridge(self.bridge); close_bridge(self.probe)

    def test_actual_selected_not_regenerated_and_full_dual_validation(self):
        before = copy.deepcopy(self.body)
        c = selected_context(self.body, self.setup, self.bridge)
        self.assertEqual(c["baseline"], [4, 16, 36])
        self.assertEqual(c["request"]["plans"], self.body["decision"]["profile"]["outcomes"][0]["futurePlans"])
        out = self.probe.request(c["request"])
        self.assertEqual(validate_response(out, c, self.bridge), 9)
        self.assertEqual(self.body, before)

    def test_reject_incomplete_root_and_forged_certificate(self):
        bad = copy.deepcopy(self.body); bad["decision"]["profile"]["outcomes"][0]["certified"] = False
        with self.assertRaisesRegex(ValueError, "selected complete"):
            selected_context(bad, self.setup, self.bridge)
        bad = copy.deepcopy(self.body); bad["decision"]["profile"]["outcomes"][0]["futurePlans"].pop()
        with self.assertRaisesRegex(ValueError, "selected complete"):
            selected_context(bad, self.setup, self.bridge)
        bad = copy.deepcopy(self.body); bad["decision"]["profile"]["outcomes"][0]["witnessScore"]["totalServings"] += 1
        with self.assertRaisesRegex(ValueError, "original score"):
            selected_context(bad, self.setup, self.bridge)

    def test_reject_ceiling_coverage_other_plan_and_ledger_mutation(self):
        c = selected_context(self.body, self.setup, self.bridge); out = self.probe.request(c["request"])
        bad = copy.deepcopy(out); bad["responses"][0]["score"][0] = c["valid_upper_bound"][0]+1
        with self.assertRaisesRegex(ValueError, "score/ceiling"): validate_response(bad, c, self.bridge)
        bad = copy.deepcopy(out); bad["responses"].pop()
        with self.assertRaisesRegex(ValueError, "coverage"): validate_response(bad, c, self.bridge)
        bad = copy.deepcopy(out); bad["responses"][0]["days"][0]["plan"][1] = [-9999]
        with self.assertRaisesRegex(ValueError, "other plan"): validate_response(bad, c, self.bridge)
        bad = copy.deepcopy(out); bad["responses"][0]["days"][0]["ledger"]["totalServings"] += 1
        with self.assertRaisesRegex(ValueError, "dual response"): validate_response(bad, c, self.bridge)


if __name__ == "__main__":
    unittest.main()
