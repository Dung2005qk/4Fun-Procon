# ATTR-QUEUE-044 evidence

Date: 2026-08-09
Parent: frozen `afcd2da` historical harness
Candidate: frozen rejected `SCORE-QUEUE-043` historical harness
Verdict: accepted read-only localization; causal source remains unresolved

The single preregistered paired fixture was
`btc-lowfuel / threshold-corridor / seed 957002`, fixed role mask `128`, with
the solver and role budgets configured at the 5000 ms hard cap. Local elapsed
columns are deliberately excluded from every conclusion.

Both runs were exact-valid with zero invalid and zero emergency days. Final
official scores reproduced the frozen tail:

- parent: `6/60/346`;
- candidate: `6/60/274`;
- first differing official tier: tier 3, delta `-72`.

The day localization is not terminal-only:

| day | parent exact | candidate exact | cumulative delta candidate-parent | plan hash equal |
|---:|---:|---:|---:|---:|
| 1 | `6/36` | `6/36` | `0` | no |
| 2 | `6/38` | `6/36` | `-2` | no |
| 3 | `6/34` | `6/21` | `-15` | no |
| 4 | `6/37` | `6/25` | `-27` | no |
| 5 | `6/33` | `6/24` | `-36` | no |
| 6 | `6/36` | `6/26` | `-46` | no |
| 7 | `6/32` | `6/24` | `-54` | no |
| 8 | `6/36` | `6/27` | `-63` | no |
| 9 | `6/30` | `6/25` | `-68` | no |
| 10 | `6/34` | `6/30` | `-72` | no |

The first action-byte difference is day 1 while the first official-score
difference is day 2. The large collapse begins on day 3 and then persists on
every remaining day. All cumulative lifetime and daily-distinct tiers remain
equal.

Current runtime tracing shows the production current-day
`enableAnytimeFuelConstrainedHarvestOrienteering` guard is enabled only when
`state.dayNumber == config.day_count()`. Therefore the cross-binary day-1
divergence cannot by itself be attributed to the intended terminal anytime
queue mechanism. Separate deadline-bounded binaries can diverge through code
layout/cutoff effects, as already demonstrated by the role experiments. The
frozen `-72` remains a real paired observed downside of that binary, but it is
not yet a causal proof that cardinality-first terminal exploration destroys
shallow route evidence.

The next admissible attribution is a same-binary, same-terminal-state queue
switch. It must freeze one day-1--9 prefix, replay identical submitted decisions
into fresh engines, and compare only day 10 under the two queue policies. No
mixed priority, weight, cap or opened-fixture tuning is authorized.
