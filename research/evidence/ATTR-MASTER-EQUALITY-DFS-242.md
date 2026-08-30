# ATTR-MASTER-EQUALITY-DFS-242

Parent behavior: canonical production checkpoint `18ecdd3`, branch HEAD
`f7c017884fe50afaa3b033ea552d8cda4a505569`.

This is attribution only. It has no source-candidate, holdout or promotion
authority.

## Fresh counterexample

The preregistered 30-match hard BTC screen stopped at match 4, `m-4952`:
hard, one bot, five days, 12x12, 40 steps/day, 10000-ms public response,
four agents, twelve spots, four brands and default fuel. Both teams finished
the first three official tiers at `4/20/90`; UDON-SHIELD ranked second only on
the fourth official response-time tiebreak, `11899 ms` versus `2493 ms`.

The replay is exact-valid on 5/5 days, the independent validator agrees on
5/5, and all 4 transitions reconcile. Replay SHA256:
`7EC89340983C01D8A7F49BB03E631D4127F2E67A4690EDF0F0EC21A59BC03C07`.
The canonical binary SHA256 is
`43ED5815DA0880652819BF589787C11CAFDC92F4D7D313899C3256E37D570389`.

Days 1--3 consumed `3375/3375/2552 ms`. Their master DFS visited
`27503/23453/36045` leaves and reached the deadline on all three days. Duplicate
complete plans skipped were `338/603/10674`; day 1 DFS alone consumed about
`1.55 s` while population maintenance consumed only `58 us`. Day 1 already
matched its exact portfolio score upper at `4/4/26`, but the horizon envelope
remained open and equal-score plans had distinct terminal position, fuel and
traffic states. Therefore stopping when the current-day score reaches its upper
would remove designed closed-loop search capability and is forbidden.

The source-frozen day-1 replay-solve evidence is
`research/evidence/ATTR-MASTER-DEADLINE-STALL-242-baseline-day1.jsonl`, SHA256
`6917E4002372B31F29CE66E0B30D178098BEF101612D54F0D4E6FDE83DE17BCF`.

## Attribution question

Measure whether the deadline is spent on exact representationally redundant
master subtrees or on genuinely distinct terminal-state outcomes. The first
probe may add research-only counters for per-depth child-state equivalence and
must not prune, reorder or alter production work. A child state is equivalent
only when every future-relevant field agrees: action bytes, active contingency
bundle, escort/refuel synchronization metadata, exact first-visit events,
selected claim counts/brands, stock-bound state and exact column footprint.

If a substantial duplicate-subtree fraction exists, a separate PERF experiment
may test one canonical transposition elimination whose complete-search output is
byte-identical. If the measured states are genuinely distinct, 242 closes the
transposition hypothesis and the next axis must derive a sound terminal-state
bound; it may not introduce an early stop, quality-order dispatcher, reduced
population, second solver or response-window-specific logic.

## Invariants

- No production source behavior changes in 242.
- Official lexicographic tiers, exact simulator, independent validator,
  terminal slack, traffic safety, stable-ID order and diversity retention are
  all preserved.
- Canonical main/role/complete checkpoint remains capped at 5000 ms; the public
  10000-ms response window does not enlarge the main solver.
- `m-4952` is consumed attribution evidence only and cannot promote or tune a
  successor.
- No map, seed, fuel, bot, match-ID or opponent dispatcher is permitted.
- No designed functionality is removed, disabled, deferred or reduced; nothing
  is deleted.

## Result and closure

The no-prune MSVC probe on day 1 measured portfolio columns
`29,31,31,32`, unique action sequences `28,30,30,32`, and conservative complete
master-child signatures `29,31,31,32`. Thus there was no exact
future-state-equivalent child redundancy to eliminate at this boundary. The
small action-only duplication carried different master metadata and was not
safe to merge.

The user then made three opponents mandatory for every future BTC screen match.
`m-4952` used one opponent and is therefore preserved only as consumed
out-of-scope attribution; it is not a blocker and cannot justify solver tuning.
Verdict: closed negative/out-of-scope. Reopen only from a fresh three-opponent
counterexample with a new invariant; do not repeat this transposition probe.
