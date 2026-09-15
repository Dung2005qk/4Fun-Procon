"""Phase-specialization and complete-development equivalence, no holdout input."""
import copy
import re
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import run_pair_score_341 as dev
import run_pair_holdout_341 as held


class HoldoutContract(unittest.TestCase):
    def test_only_preregistered_phase_count_and_gate_specialization(self):
        source = Path(dev.__file__).read_text(encoding='utf-8')
        expected = source.replace(
            '"""Frozen actual HTTP paired screen; no partial-score interpretation or holdout run."""',
            '"""One-time frozen341 holdout; same development runtime, comparator and safety."""')
        expected = expected.replace('f"research/holdouts/{ID}-execution.json"',
                                    'f"research/holdouts/{ID}-holdout-execution.json"')
        expected = expected.replace('development', 'holdout')
        for a, b in ((24, 54), (48, 108), (192, 432), (144, 324)):
            expected = re.sub(r'\b' + str(a) + r'\b', str(b), expected)
        expected = expected.replace('def gate(rows, minimum_wins=4, minimum_families=2):',
                                    'def gate(rows, minimum_wins=6, minimum_families=3):')
        self.assertEqual(Path(held.__file__).read_text(encoding='utf-8'), expected)

    def rows(self):
        return [dict(family='f'+str(i%3), comparison='win',
                     activated_before_divergence=True, delta=[0, 0, 2]) for i in range(6)]

    def test_six_activated_wins_three_families_required(self):
        rows = self.rows()
        self.assertTrue(held.gate(rows))
        self.assertFalse(held.gate(rows[:-1]))
        for r in rows: r['family'] = 'f'+str(len(r['family'])%2)
        self.assertFalse(held.gate(rows))

    def test_bounded_loss_kept_but_not_higher_tier_or_family_loss(self):
        for delta, family, accepted in (([0,0,-1], 'f0', True), ([0,0,-2], 'f0', False),
                ([-1,100,100], 'f0', False), ([0,-1,100], 'f0', False), ([0,0,-1], 'new', False)):
            rows = self.rows()+[dict(family=family, comparison='loss',
                                     activated_before_divergence=True, delta=delta)]
            self.assertEqual(held.gate(rows), accepted)
            self.assertEqual(held.gate(rows), dev.gate(rows, 6, 3))

    def test_inactive_and_gain_loss_balance(self):
        rows = self.rows(); rows[0]['activated_before_divergence'] = False
        self.assertFalse(held.gate(rows))
        rows = self.rows()
        for r in rows: r['delta'][2] = 1
        rows += [dict(family='f'+str(i%3), comparison='loss',
                      activated_before_divergence=True, delta=[0,0,-1]) for i in range(4)]
        self.assertFalse(held.gate(rows))

    def test_missing_or_partial_run_blocks_before_side_validation(self):
        with tempfile.TemporaryDirectory(prefix='udon341-holdout-contract-') as folder:
            directory = Path(folder)
            cases = [{'spec': {'seed': i}} for i in range(54)]
            def fake_load(path):
                if path == held.M: return {'holdout': 'unit-cases'}
                if path.name == 'unit-cases': return {'cases': cases}
                if path.name == 'run_complete.json': return {'execution_sha256':'unit', 'pairs':53}
                raise AssertionError('must not read side evidence')
            with patch.object(held, 'D', directory), patch.object(held, 'load', fake_load), \
                    patch.object(held, 'verify'), patch.object(held, 'digest', return_value='unit'), \
                    patch.object(held, 'validate_side') as validate:
                with self.assertRaisesRegex(ValueError, 'completion identity'): held.summarize()
                validate.assert_not_called()

    def test_completed_development_measurement_helpers_and_gate_identical(self):
        m = dev.load(dev.M); dev.verify(m)
        report = dev.load(dev.S); rows = report['rows']
        self.assertTrue(report['complete'] and report['gate_passed'])
        self.assertEqual(held.gate(rows), dev.gate(rows, 6, 3))
        cases = dev.load(dev.ROOT/m['development'])['cases']
        for c in cases:
            for side in ('parent', 'candidate'):
                self.assertEqual(held.validate_side(c, side, dev.D), dev.validate_side(c, side, dev.D))
            prefix = dev.D/'candidate'/str(c['spec']['seed'])
            replay = prefix.with_suffix('.replay.jsonl')
            self.assertEqual(held.decisions(replay), dev.decisions(replay))
            plans = [d['plan'] for d in dev.load(prefix.with_suffix('.transport.json'))['actions']]
            for i, day in enumerate(dev.decisions(replay)):
                following = plans[i+1] if i < 3 else None
                self.assertEqual(held.measured_day(day, following), dev.measured_day(day, following))


if __name__ == '__main__': unittest.main()
