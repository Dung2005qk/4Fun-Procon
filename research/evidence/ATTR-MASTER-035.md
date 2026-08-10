# ATTR-MASTER-035 exact claim-mask attribution

Date: 2026-08-09
Parent: `afcd2da`
Verdict: accepted read-only attribution

## Frozen inputs

- BTC replay: `artifacts/btc/m-1285-score-discovery-live.jsonl`
  - SHA256: `D638BE3D0E136EF94542BBCE58AA6B086D6A7C872732EF27188392D5D19D2F77`
- Canonical parent day-10 plan scoring `6/60/321`:
  `research/evidence/m1285-day10-parent-321-plan.json`
  - SHA256: `787F2C1774D11D08125115FD3F75488A222572CB0CB79692F14A877F5EB81DA4`
- Exact bundle day-10 plan scoring `6/60/320`:
  `research/evidence/m1285-day10-exact-bundle-34-plan.json`
  - SHA256: `60CBC8BE30326307ED895B1111CCA39EB1E1BAA0B60EDC9CE6E726DCC9FE0832`

The probe was compiled against the frozen parent `udon_shield.lib`. It
reconstructed the ledger only from accepted replay actions and validated both
plans with `ExactStepSimulator` and `IndependentDayValidator`. No planner or
search function was called. No elapsed time was measured or used.

## Exact results

Both plans start day 10 from ledger `6/54/286`, preserve six daily brands and
are independently exact-valid. The exact bundle serves 34 for `6/60/320`; the
canonical witness serves 35 for `6/60/321`.

Agents 1 through 7 have byte-identical action sequences in both plans. Only
agent 0 changes:

| plan | agent-0 spot mask | claims | served | terminal |
|---|---:|---:|---:|---:|
| exact bundle 34 | `{0,3,4,7,8,11}` = `0x999` | 6 | 6 | cell 146, fuel 87 |
| canonical witness 35 | `{3,4,6,7,8,11}` = `0x9D8` | 6 | 6 | cell 406, fuel 100 |

The replacement drops spot 0 and adds spot 6. In the exact bundle, spot 0 has
four claims against stock two, so two claims are denied, while spot 6 has only
four claims against stock six. In the canonical witness, spot 0 has three
claims against stock two and spot 6 has five against stock six. Exactly one
denial is therefore removed and no other claim changes, proving the entire
one-serving gain is a stock-aware one-agent route substitution.

## Consequence

This closes the ambiguity about a distributed multi-agent trajectory: the
known 35 witness differs from the exact 34 bundle by one agent and one spot-bit
exchange. It does not yet prove whether exact enumeration omitted `0x9D8` or
whether exact coordination failed to choose it. The only admissible successor
is a read-only membership probe of the canonical agent-0 exact frontiers. No
route rank, cap or coordinator change is authorized by this result alone.
