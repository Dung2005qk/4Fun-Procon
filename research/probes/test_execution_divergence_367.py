"""Synthetic forensic-checker tests only; no solver execution."""
import copy
from pathlib import Path
import tempfile
import unittest
from audit_execution_divergence_367 import verify_files, sha
from explain_execution_divergence_367 import singleton_rank_check


def score(a, b, c):
    return dict(zip(("lifetimeDistinct", "totalDailyDistinct", "totalServings"), (a, b, c)))


def row(identity, value, today, selected=False):
    return {"stableId": identity, "certified": True, "selected": selected,
            "finalQuantile50": score(*value), "finalCertifiedLowerBound": score(*value),
            "scoreAfterToday": score(*today)}


def decision(rows):
    return {"manifest": {"scenarios": [{"weight": 10000}]},
            "riskPolicy": {"safetySlack": 1},
            "audit": {"candidates": rows, "selectionReason": "certified-undominated-current-floor"}}


class Checks(unittest.TestCase):
    def test_correct_rank(self):
        d = decision([row("a", (6, 44, 291), (6, 6, 58), True), row("b", (6, 44, 289), (6, 6, 58))])
        self.assertEqual(singleton_rank_check(d)["status"], "no-strict-rank-inversion")

    def test_inverted_rank_detected(self):
        d = decision([row("a", (6, 44, 289), (6, 6, 58), True), row("b", (6, 44, 291), (6, 6, 58))])
        self.assertEqual(singleton_rank_check(d)["status"], "strict-rank-inversion")

    def test_official_tiers_not_serving_sum(self):
        d = decision([row("a", (6, 44, 1), (6, 6, 58), True), row("b", (6, 43, 999), (6, 6, 58))])
        self.assertEqual(singleton_rank_check(d)["status"], "no-strict-rank-inversion")

    def test_current_floor_exclusion(self):
        d = decision([row("a", (6, 44, 289), (6, 6, 58), True), row("b", (6, 50, 999), (6, 6, 57))])
        self.assertEqual(singleton_rank_check(d)["status"], "no-strict-rank-inversion")

    def test_tie_not_false_defect(self):
        d = decision([row("a", (6, 44, 291), (6, 6, 58), True), row("b", (6, 44, 291), (6, 6, 58))])
        self.assertEqual(singleton_rank_check(d)["status"], "no-strict-rank-inversion")

    def test_multiple_scenarios_not_guessed(self):
        d = decision([])
        d["manifest"]["scenarios"] *= 2
        self.assertEqual(singleton_rank_check(d)["status"], "multiple-scenarios-not-reconstructible")

    def test_missing_selection_rejected(self):
        with self.assertRaises(AssertionError):
            singleton_rank_check(decision([row("a", (6, 44, 291), (6, 6, 58))]))

    def test_singleton_inconsistency_rejected(self):
        d = decision([row("a", (6, 44, 291), (6, 6, 58), True)])
        d["audit"]["candidates"][0]["finalQuantile50"] = score(6, 44, 292)
        with self.assertRaises(AssertionError):
            singleton_rank_check(d)

    def test_hash_and_path_guards(self):
        with tempfile.TemporaryDirectory(prefix="udon367-check-") as name:
            directory = Path(name)
            p = directory / "fixture"
            p.write_bytes(b"synthetic fixture")
            verify_files(directory, {"fixture": sha(p)})
            with self.assertRaises(ValueError):
                verify_files(directory, {"fixture": "0" * 64})
            with self.assertRaises(ValueError):
                verify_files(directory, {"../outside": "0" * 64})


if __name__ == "__main__":
    unittest.main()
