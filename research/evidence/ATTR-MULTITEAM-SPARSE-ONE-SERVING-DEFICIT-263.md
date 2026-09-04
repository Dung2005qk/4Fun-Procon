# ATTR-MULTITEAM-SPARSE-ONE-SERVING-DEFICIT-263

Parent: canonical production `ab3d699`; signed Windows BTC binary SHA256
`4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`.

Consumed replay: `artifacts/btc/m-5501-ab3d699-series.jsonl`, SHA256
`6E1CC450CC80D87CAFA5756BD1B495CBDBB0FE2B3D3F9DA91347CBCBDBF7F92C`.

## Result

Hard three-bot `m-5501` used five days, a 24x24 map, 96 steps/day,
four agents, eighteen spots, six brands, high fuel and a public `10000 ms`
window. Production finished fourth at `6/30/127`; all three bots tied at
`6/30/128`. All five submissions were accepted and dual-valid, all four
transitions reconciled, response times were `5759..8397 ms`, and stderr was
empty.

## Attribution

The accepted 258 terminal path is live and causal. On day 5 it lifts the
complete checkpoint from 29 to 30 servings, evaluates 3198 valid continuation
plans, and records one terminal-marginal acceptance among 111 valid marginal
plans. There is no failure, rollback or deadline event.

The unchanged no-deadline terminal probe confirms a fixed point at `6/30/127`:
3264 sparse routes, 3069 generated/valid plans, 96 marginal routes, 30 valid
marginal plans and zero additional strict gain. There is no missing 31-serving
candidate in the canonical terminal mechanism.

On nonterminal day 4, the unchanged sparse closed-loop probe finds an exact
global substitution from `6/24/97` to `6/24/99` and retains the +2 serving
difference through its local suffix. Every witness changes terminal/fuel/
own-road state; protected same-state best remains `6/24/97` with
`protected_mask=0`. This is the known state-coupled sparse residual, not a new
safe continuation or terminal-supply gap.

## Verdict

Closed as `normal-bounded-tier3-variance-known-state-coupled-residual`. The
one-serving bot difference after production locks both higher official tiers is
not evidence of regression or a promotable mechanism. No source change or
holdout is authorized.

Reopen only from a fresh protected same-state gain or a genuinely new executable
whole-remaining-horizon certificate on unopened evidence. Do not tune from
`m-5501` or repeat 216--232.

Functionality-preservation answers: (1) this closure removes, disables, defers
or reduces no designed functionality; (2) nothing is deleted.
