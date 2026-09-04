# ATTR-MULTITEAM-TIER3-TWO-SERVING-DEFICIT-262

Parent: canonical production `ab3d699`; signed Windows BTC binary SHA256
`4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`.

Consumed replay: `artifacts/btc/m-5477-ab3d699-series.jsonl`, SHA256
`0AF34AB081E7A512376BD9798493085DC9C5BED4AE48B1F613C8D88EB45D8705`.

## Result

Hard three-bot `m-5477` used nine days, a 24x24 map, 72 steps/day,
six agents, twenty spots, seven brands, medium fuel and a public `10000 ms`
window. Production finished second at `7/63/268`, two servings behind the
winner at `7/63/270`; lifetime and daily tiers are tied at their fixture maxima.

All 9 submissions were accepted and dual-valid, all 8 transitions reconciled,
response times were `6305..8529 ms`, and stderr was empty. There is no protocol,
network, state reconciliation, deadline or safety failure.

## Attribution

The complete physical checkpoint totals 263 servings. Protected refinement and
public continuation add five servings and submit 268: one gain on day 3, one on
day 5 and three on day 7. Across the nine days the continuation dual-validates
161, 154, 283, 133, 226, 159, 415, 159 and 9983 plans. It has two benign
deadline-reached observations on days 4 and 5 but no failure or rollback.

On terminal day 9, the accepted 258 reservoir is not starved. The live path
evaluates 9983 regular plans plus 109 valid marginal plans and accepts no gain.
The no-deadline attribution probe reconstructs the same fixed point: 10560
sparse routes, 9874 generated/valid plans, 160 marginal routes, 109 valid
marginal plans, zero strict gain and exact score `7/63/268`.

Fresh main-only replay is weaker than the complete live pipeline and supplies
no operation-equivalent gain. The lowest-serving nonterminal day (day 8) has an
unchanged sparse global witness from `7/56/231` to `7/56/234`, but every such
witness changes the future terminal/fuel/own-road state; protected best remains
exactly `7/56/231` with `protected_mask=0`. This is the already-known
state-coupled sparse residual, not a new safe candidate.

## Verdict

Closed as `normal-bounded-tier3-variance-known-state-coupled-residual`. A
two-serving bot difference after production locks both higher official tiers is
an asymmetric failure observation, not evidence that the parent regressed or
that a SCORE successor exists. No source change or holdout is authorized.

Reopen only from a fresh protected same-state gain or a genuinely new
whole-remaining-horizon certificate on unopened evidence. Do not tune from
`m-5477` or repeat the rejected 216--232 mechanisms.

Functionality-preservation answers: (1) this closure removes, disables, defers
or reduces no designed functionality; (2) nothing is deleted.
