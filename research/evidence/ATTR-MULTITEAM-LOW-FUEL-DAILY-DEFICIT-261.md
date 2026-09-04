# ATTR-MULTITEAM-LOW-FUEL-DAILY-DEFICIT-261

Parent: canonical production `ab3d699`; signed Windows BTC binary SHA256
`4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`.

Consumed BTC replay: `artifacts/btc/m-5422-ab3d699-series.jsonl`, SHA256
`1D8FDF6BF3BDB871471CF18AE65DB29D1D3953649A55FFCCDA4929ED7325C794`.

## Counterexample

Hard three-bot `m-5422` used ten days, a 32x32 map, 96 steps/day,
eight agents, thirty-two spots, eight brands, low fuel and a public
`10000 ms` response window. Production finished second at `8/78/247`; the
winner reached `8/80/327`. Days 7 and 9 each missed one daily brand. All ten
submissions were accepted and dual-valid, response times were `5373..8416 ms`,
and stderr was empty.

## Transition attribution

Replay-check reconciles eight of nine transitions. After day 2, patrol 3 is
predicted at cell 702 with fuel 21, while the day-3 authoritative state reports
the same cell with fuel 77. This is the same position-exact, fuel-only-richer
shape recorded by `SEM-REFUEL-MIDMOVE-205`: cross-team refuel-to-full is
plausible under the public rule text, opponent mid-day paths are not public,
and daily reconciliation safely adopts the richer authoritative state. The
discrepancy cannot invalidate the submitted plan and supplies no reason to
assume cross-team refuel while planning. It is not a new correctness candidate.

## Daily-deficit attribution

Fresh `replay-solve --response-ms 5000` reaches eight daily brands on both
target days (`8/20` on day 7 and `8/15` on day 9). That establishes geometric
feasibility only; a clean replay engine does not restore the live response
ledger, virtual/checkpoint divergence, caches or exact physical trajectory.

The live decision's virtual planning branch had an eight-brand candidate on
both days. Mandatory replay from the complete physical checkpoint reached only
seven brands, so production correctly preserved the executable checkpoint.
Public continuation remained healthy and dual-valid: day 7 generated 72 plans
and accepted none; day 9 generated 73 plans and accepted one servings-only
gain (`7/12 -> 7/15`). Neither continuation recovered the missing brand.

The unchanged recorded sparse closed-loop probe pins the remaining boundary:

- Day 7: global sparse substitution changes the root from `8/55/185` to
  `8/56/197`, but day 9 then loses one daily brand and both suffixes finish at
  `8/79`; servings improve by 20. Protected same-terminal/fuel/road-footprint
  best is unchanged (`protected_mask=0`).
- Day 9: global sparse substitution changes `8/70/216` to `8/71/228` and its
  local suffix finishes `8/79/243` versus `8/78/234`. Protected best is again
  unchanged (`protected_mask=0`).

Thus an exact current-day route can improve the consumed replay only by
changing the future physical/traffic state. This is the already-registered
nonterminal state-coupled sparse residual from 216/223, not a new protected
mechanism. The direct proof/ACK/protected consumers 229--232 already failed to
turn this residual into a safe general takeover. Repeating them or tuning a
dispatcher from `m-5422` would be circular and overfit.

## Verdict

Closed as `known-state-coupled-sparse-residual-no-safe-successor`. No source
change, holdout or SCORE successor is authorized. Reopen only with a genuinely
new executable whole-remaining-horizon dominance certificate or an
operation-equivalent capability improvement on fresh unopened evidence.

Functionality-preservation answers: (1) this closure removes, disables, defers
or reduces no designed functionality; (2) nothing is deleted, so no active
equivalent is required.
