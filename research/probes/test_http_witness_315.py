import json
from pathlib import Path
import unittest

from run_http_witness_315 import parse_case, verify_inputs
from run_http_baseline_314 import ROOT, Bridge, EMPTY_LEDGER
from summarize_http_witness_315 import (analyze_case, audit_classification,
                                       candidates_matching, state_key)

SPEC = {"seed": 10700000, "family": "three-balanced", "players": 8, "oracle_score": [5, 20, 38]}


def fixture_log():
    header = ("case,seed=10700000,family=three-balanced,fuel=low,players=8,agents=3,horizon=4,"
              "oracle=5/20/38,head=5/20/36,oracle_valid=1,head_valid=1\n")
    traces = "".join(f"trace,seed=10700000,day={day},oracle_score=5/20/38,"
                     "oracle_id=[[-17],[-17],[-17]],head_id=[[-17],[-17],[-17]]\n"
                     for day in range(1, 5))
    return header + traces + "summary,cases=1,invalid=0,head_wins=0\n"


class WitnessTests(unittest.TestCase):
    def test_parse_all_four_days(self):
        self.assertEqual([row["day"] for row in parse_case(fixture_log(), SPEC)], [1, 2, 3, 4])

    def test_duplicate_or_missing_day_rejected(self):
        for invalid in (fixture_log().replace("day=4", "day=3"),
                        "\n".join(line for line in fixture_log().splitlines() if "day=4" not in line)):
            with self.assertRaisesRegex(ValueError, "day traces"):
                parse_case(invalid, SPEC)

    def test_wrong_final_score_or_invalid_case_rejected(self):
        for invalid in (fixture_log().replace("oracle=5/20/38", "oracle=5/20/39"),
                        fixture_log().replace("oracle_valid=1", "oracle_valid=0")):
            with self.assertRaises(ValueError):
                parse_case(invalid, SPEC)

    def test_malformed_plan_rejected(self):
        with self.assertRaisesRegex(ValueError, "malformed oracle plan"):
            parse_case(fixture_log().replace("oracle_id=[[-17],[-17],[-17]]", 'oracle_id=[["wait"],[],[]]'), SPEC)

    def test_exact_brand_identity_not_cardinality(self):
        a = {"brands": [1, 3], "totalDailyDistinct": 2, "totalServings": 4}
        b = dict(a, brands=[1, 4])
        self.assertNotEqual(state_key([], a), state_key([], b))
        self.assertEqual(state_key([], a), state_key([], dict(a, brands=[3, 1])))

    def test_permutation_is_separate_from_physical_identity(self):
        agents = [{"kind": 0, "pos": 16, "fuel": 4}, {"kind": 0, "pos": 20, "fuel": 3}]
        self.assertNotEqual(state_key(agents, EMPTY_LEDGER), state_key(agents[::-1], EMPTY_LEDGER))
        self.assertEqual(state_key(agents, EMPTY_LEDGER, True), state_key(agents[::-1], EMPTY_LEDGER, True))

    def test_audit_classification_boundaries(self):
        self.assertEqual(audit_classification([]), "absent-from-recorded-audit")
        row = {"selected": False, "certified": False, "disposition": "not-shortlisted"}
        self.assertEqual(audit_classification([row]), "present-but-not-shortlisted")
        self.assertEqual(audit_classification([dict(row, disposition="uncertified")]), "shortlisted-but-uncertified")
        self.assertEqual(audit_classification([dict(row, certified=True)]), "certified-but-unselected")
        self.assertEqual(audit_classification([dict(row, selected=True, certified=True)]), "selected-before-later-divergence")

    def test_wrong_deployed_hash_blocks_before_oracle(self):
        manifest = {"experiment": "ATTR-THREE-PATROL-HTTP-WITNESS-315",
            "cases": [{"seed": seed} for seed in (10700000,10700001,10700100,10700101,10700500,10700501)],
            "deployment_hashes": {"test_http_witness_315.py": "0"*64}}
        with self.assertRaisesRegex(ValueError, "deployed hash mismatch"):
            verify_inputs(manifest, Path(__file__).parent)

    def test_audited_equal_count_wrong_brands_not_equivalent(self):
        agents = [{"kind": 0, "pos": 16, "fuel": 1}]
        ledger = {"brands": [1], "totalDailyDistinct": 1, "totalServings": 1}
        target = {"ok": True, "agrees": True, "agents": agents, "ledger": ledger, "score": [1, 1, 1]}
        class FakeBridge:
            def request(self, request):
                return dict(target, ledger=dict(ledger, brands=[2]))
        candidate = {"scoreAfterToday": {"lifetimeDistinct": 1, "totalDailyDistinct": 1, "totalServings": 1},
            "terminalCells": [16], "terminalFuel": [1], "stableId": "[[-1]]"}
        self.assertEqual(candidates_matching(FakeBridge(), {}, {"agents": agents}, ledger, target, [candidate]), ([], []))

    def test_real_recorded_four_day_http_path_revalidates(self):
        directory = ROOT / "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"
        transport = json.loads((directory / "10700000.transport.json").read_text())
        traces = [{"day": action["wire_day"]+1, "plan": action["plan"], "score": action["validated"]["score"]}
                  for action in transport["actions"]]
        spec = dict(SPEC, oracle_score=transport["score"], http_score=transport["score"])
        bridge = Bridge(ROOT / "artifacts/research/314/bridge.exe")
        try:
            # The same four plans must reach the exact same states/ledgers.
            # An allegedly winning witness with no divergence is rejected.
            with self.assertRaisesRegex(ValueError, "no divergence"):
                analyze_case(spec, transport["setup"], traces, directory / "10700000.replay.jsonl",
                             directory / "10700000.transport.json", bridge)
        finally:
            bridge.close()
            bridge.process.stdout.close()
            bridge.process.stderr.close()


if __name__ == "__main__":
    unittest.main()
