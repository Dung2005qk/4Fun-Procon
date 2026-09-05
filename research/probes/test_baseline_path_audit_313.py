"""Guard tests for the source-only 313 audit; no solver or holdout execution."""
import copy
import json
from pathlib import Path
import unittest

from audit_three_patrol_baseline_path_313 import analyze, sha256


class BaselinePathAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads((root / "research/holdouts/ATTR-THREE-PATROL-BASELINE-PATH-313.json").read_text())
        cls.texts = {}
        for path, expected in manifest["inputs"].items():
            raw = (root / path).read_bytes()
            if sha256(raw) != expected:
                raise AssertionError(f"frozen input changed: {path}")
            cls.texts[path] = raw.decode("utf-8-sig").replace("\r\n", "\n")
        cls.summary = json.loads(next(value for key, value in cls.texts.items() if key.endswith(".summary.json")))

    def test_reviewed_snapshot_preserves_results_but_grants_no_score_authority(self):
        report = analyze(self.texts, self.summary)
        self.assertEqual(report["preserved_312_oracle_vs_engine"]["wtl"], {"wins": 6, "ties": 6, "losses": 0})
        self.assertFalse(report["baseline"]["complete_checkpoint_measured"])
        self.assertEqual(report["baseline"]["production_residual_gap"], "unmeasured")
        self.assertFalse(report["score_successor_authorized"])
        self.assertFalse(report["holdout_open_authorized"])

    def test_changed_current_floor_policy_requires_new_review(self):
        changed = dict(self.texts)
        path = "research/probes/multi_patrol_oracle.cpp"
        changed[path] = changed[path].replace("kHarvestMode,\n        false,\n        kFutureHarvestMode", "kHarvestMode,\n        true,\n        kFutureHarvestMode")
        with self.assertRaisesRegex(ValueError, "direct constructor changed"):
            analyze(changed, self.summary)

    def test_changed_http_calibration_requires_new_review(self):
        changed = dict(self.texts)
        path = "src/btc_main.cpp"
        changed[path] = changed[path].replace("networkFloor = std::chrono::milliseconds{1600}", "networkFloor = std::chrono::milliseconds{50}")
        with self.assertRaisesRegex(ValueError, "HTTP calibration changed"):
            analyze(changed, self.summary)

    def test_missing_refiner_must_not_silently_pass(self):
        changed = dict(self.texts)
        changed["src/btc_main.cpp"] = changed["src/btc_main.cpp"].replace("refine_midday_chains", "removed_midday_chains")
        with self.assertRaisesRegex(ValueError, "missing reviewed HTTP path"):
            analyze(changed, self.summary)

    def test_protected_runner_change_requires_new_scope_audit(self):
        changed = dict(self.texts)
        path = "research/probes/run_three_active_patrol_prevalence_312_vm.sh"
        changed[path] += "\n# --protected-head\n"
        with self.assertRaisesRegex(ValueError, "different path"):
            analyze(changed, self.summary)

    def test_incomplete_or_invalid_summary_is_rejected(self):
        for key, value in (("cases", 11), ("zero_invalid", False)):
            with self.subTest(key=key):
                summary = copy.deepcopy(self.summary)
                summary[key] = value
                with self.assertRaisesRegex(ValueError, "incomplete or invalid"):
                    analyze(self.texts, summary)


if __name__ == "__main__":
    unittest.main()
