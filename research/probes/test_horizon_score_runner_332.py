import copy
import unittest
from run_horizon_score_332 import check_stats, gate, FIELDS, LIMITS


class Contract(unittest.TestCase):
    def stats(self): return dict.fromkeys(FIELDS, 0)
    def rows(self):
        return [{"family": "f"+str(i%2), "comparison": "win", "activated_before_divergence": True,
                 "delta": [0,0,2]} for i in range(4)]
    def test_zero_and_completed_counters(self):
        check_stats(self.stats())
        d = self.stats(); d.update(calls=2, supported=2, completed=1, exhausted=1, improvements=1)
        d.update(LIMITS); check_stats(d)
    def test_missing_or_failed_or_excessive_work_is_blocked(self):
        d = self.stats(); d.pop("failures")
        with self.assertRaises(ValueError): check_stats(d)
        d = self.stats(); d.update(calls=1, supported=1, failures=1)
        with self.assertRaises(ValueError): check_stats(d)
        for k in LIMITS:
            d = self.stats(); d.update(calls=1, supported=1, exhausted=1); d[k] = LIMITS[k]+1
            with self.assertRaises(ValueError): check_stats(d)
    def test_bounded_loss_is_not_mechanically_rejected(self):
        rows = self.rows(); rows.append({"family":"f0", "comparison":"loss", "activated_before_divergence":True, "delta":[0,0,-1]})
        self.assertTrue(gate(rows))
    def test_first_tier_and_unbounded_loss_block(self):
        for delta in ([-1,100,100], [0,-1,100], [0,0,-2]):
            rows=self.rows(); rows.append({"family":"f0","comparison":"loss","activated_before_divergence":True,"delta":delta})
            self.assertFalse(gate(rows))
    def test_inactive_breadth_or_family_loss_blocks(self):
        rows=self.rows(); rows[0]["activated_before_divergence"]=False; self.assertFalse(gate(rows))
        rows=self.rows()
        for r in rows:r["family"]="f0"
        self.assertFalse(gate(rows))
        rows=self.rows(); rows.append({"family":"f2","comparison":"loss","activated_before_divergence":True,"delta":[0,0,-1]})
        self.assertFalse(gate(rows))


if __name__ == "__main__": unittest.main()
