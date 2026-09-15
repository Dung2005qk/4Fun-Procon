import unittest
from ack_suffix_consumption_333 import classify, gate, cache_has, plan_id


class Contract(unittest.TestCase):
    def row(self, **kw):
        return {"seed": 1, "family": "a", "next_certificate": [3,12,21], "witness_score": [3,12,22],
                "plan_matches": False, "current_floor_eligible": True, "cache_at_ack": True,
                "cache_before_next": True, "audit_record": {"disposition": "not-shortlisted"}, **kw}

    def test_alternative_not_loss(self):
        self.assertEqual(classify(self.row(next_certificate=[3,12,22])), "equal-or-better-certified-alternative")
        self.assertEqual(classify(self.row(next_certificate=[4,12,1])), "equal-or-better-certified-alternative")
        self.assertEqual(classify(self.row(next_certificate=[3,12,22], plan_matches=True)), "direct-consumption")

    def test_separate_boundaries(self):
        self.assertEqual(classify(self.row()), "absent-from-W1-shortlist")
        self.assertEqual(classify(self.row(current_floor_eligible=False)), "current-day-floor-conflict")
        self.assertEqual(classify(self.row(cache_at_ack=False)), "missing-ack-cache")
        self.assertEqual(classify(self.row(cache_before_next=False)), "lost-during-idle-cache")
        self.assertEqual(classify(self.row(audit_record=None)), "absent-from-post-F0-audit")
        self.assertEqual(classify(self.row(audit_record={"disposition": "certified-not-selected"})), "lower-selected-certificate")

    def test_breadth_and_floor(self):
        self.assertFalse(gate([self.row(), self.row(seed=2)]))
        self.assertFalse(gate([self.row(), self.row(seed=2, family="b", current_floor_eligible=False)]))
        self.assertFalse(gate([self.row(), self.row(seed=2, family="b", next_certificate=[3,12,22])]))
        self.assertTrue(gate([self.row(), self.row(seed=2, family="b")]))

    def test_full_suffix_not_first_plan_only(self):
        w = {"certified": True, "lowerBoundOnly": False, "witnessScore": {"totalServings": 20}, "futurePlans": [[[-4]], [[-3]]]}
        suffix = {"certified": True, "lowerBoundOnly": False, "score": w["witnessScore"], "futurePlans": w["futurePlans"]}
        c = {"plan": w["futurePlans"][0], "certifiedSuffix": suffix}
        self.assertTrue(cache_has({"cachedContingencies": [c]}, w))
        self.assertFalse(cache_has({"cachedContingencies": [{**c, "certifiedSuffix": {**suffix, "futurePlans": [[[-4]]]}}]}, w))
        self.assertEqual(plan_id([[1,-3],[-5]]), "[[1,-3],[-5]]")


if __name__ == "__main__": unittest.main()
