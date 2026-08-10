# ATTR-QUEUE-046 evidence

Date: 2026-08-09
Parent: `afcd2da` source with temporary same-binary queue switch
Runtime parity: explicit harvest mode `7`, future mode `7`
Verdict: accepted causal terminal attribution

One shallow-policy prefix through day 9 produced terminal state hash
`7024081118606904448` and ledger `6/54/312` on opened BTC-like low-fuel
threshold-corridor seed 957002. The identical submitted-decision history was
replayed into fresh engines with equally cold route caches. All day-10 plans
passed the exact simulator and independent validator.

| run | day-10 exact | cumulative | plan hash | exact seed/local | supported/settled |
|---|---:|---:|---:|---:|---:|
| forward shallow | `6/39` | `6/60/351` | `3450257480091618005` | `38/38` | `7/1343228` |
| forward cardinality | `6/39` | `6/60/351` | `108925725917910114` | `38/39` | `7/1777423` |
| reverse cardinality | `6/39` | `6/60/351` | `108925725917910114` | `38/39` | `7/1777423` |
| reverse shallow | `6/39` | `6/60/351` | `3450257480091618005` | `38/38` | `7/1343228` |

Each policy is byte-stable across execution order. Both are exact-valid, hit
the bounded search deadline, use three exact bundles and finish with zero
invalid/emergency. Cardinality-first changes the terminal plan and improves the
exact local coordinator from 38 to 39 servings, but the shallow full planner
already obtains 39 through another candidate path. The causal official delta on
this previously alleged negative state is therefore exactly zero.

Together with the archived production-mode `m-1285` day-10 result, where the
same capped cardinality mechanism directly raised exact seed/local and selected
score from `6/60/321` to `6/60/323`, this removes the only alleged material
downside but does not prove global strength. The old protected matrix used mode
6 and is invalid for this mechanism. A successor must start from restored parent
source, freeze a new diverse holdout wired to mode 7, and evaluate paired
same-state terminal outcomes. No opened SCORE-QUEUE-043 seed may be reused for
tuning or promotion.
