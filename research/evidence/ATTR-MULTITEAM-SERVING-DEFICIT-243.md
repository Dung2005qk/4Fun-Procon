# ATTR-MULTITEAM-SERVING-DEFICIT-243

Parent behavior: canonical production checkpoint `18ecdd3`, branch HEAD
`f7c017884fe50afaa3b033ea552d8cda4a505569`.

This is attribution only. It has no source candidate, holdout or promotion
authority.

## Fresh counterexample

`m-4955` is hard with three bots, ten days, 24x24, 80 steps/day, 10000-ms
public response, eight agents, 24 spots, eight brands and high fuel. All four
teams reached `8/80`; UDON-SHIELD ranked fourth with 449 servings behind
500/475/453. Replay SHA256:
`63D361462D000C21F62EF72442084D1F08443AE871B9667A97F4C7C48919EBBD`.

The replay is exact-valid on 10/10 days, the independent validator agrees on
10/10, and all nine transitions reconcile. Per-day servings are
`43,45,45,41,45,45,45,43,43,54`. Public continuation was authorized on every
day and generated valid plans, but accepted zero replacements. Terminal-day
refinement lifted the checkpoint from 45 to 54.

## Completed attribution

Role identity is not the cause. The production role assignment is seven patrols
and one tanker with mask 64. A closed-loop replay counterfactual scored
`8/80/441`; moving the sole tanker to the bots' slot (mask 128) scored
`8/80/445`. The `+4` identity effect is far smaller than the gap to
`8/80/500`, and the existing role rollout still ranks the production mask first.
This does not reopen the rejected role-count, role-identity or role-beam axes.

The main route portfolio is the binding representational boundary. On days
1--9, the complete main/master serving upper is approximately `44--45` per day;
the 24-spot configuration reports zero supported/completed exact-orienteering
agents. Running the unchanged 216/223 sparse attribution probe on the consumed
replay finds a changed-terminal strict improvement on every nonterminal day:
per-day deltas `+3,+3,+3,+4,+3,+3,+3,+3,+3`, naively `+28`. It finds **zero**
transition-dominating protected improvement on all nine days. Probe binary
SHA256 is
`98E9D60D31E7D4C0AF2D4E4CA2A4AF8944923C6D2701404FD986F2325655FCC2`;
the compact exact output is
`ATTR-MULTITEAM-SERVING-DEFICIT-243-sparse.log`, SHA256
`BC1FF2FDAA11E485C92A70D5EDEE7401CF289B5620C29DAB7082EF4221054EB0`.

This exactly reproduces the post-219 classification from ATTR-223: candidate
supply exists, time is not the blocker, but every gain crosses the future-state
certificate domain. Experiments 229--232 already falsified the available
cache/proof/ACK consumers for that residual. Injecting these plans before F0 is
not a new safe mechanism: it changes the authoritative next-day state and is
the rejected direct-W0/frozen-scenario class unless a new whole-horizon
state-coupled certificate is supplied.

The terminal day is consistent with the same boundary and needs no new logic:
there is no future state, so accepted 190/191/207 legitimately raises the day-10
checkpoint from 45 to 54.

## Verdict

Closed as a known certificate-blocked residual, not a fresh actionable SCORE
gap. The loss is a valid opponent counterexample but does not justify repeating
216--232 or tuning from `m-4955`. Resume the 30-match screen. Reopen only if a
new state-coupled whole-remaining-horizon certificate becomes executable within
the authoritative public window, or a fresh 8/9/10-team traffic-specific
counterexample proves a different causal mismatch.

Do not repeat the rejected role beam, quality-order, timed-dedup, proof-witness,
ACK-rebase, or transposition mechanisms. BTC three-bot evidence is only a
minimum multi-team proxy; any traffic-sensitive successor must pass synthetic
8/9/10-team protected lanes.

## Invariants

- No production behavior change in 243.
- Preserve official lexicographic score, exact simulator, independent validator,
  all traffic safety, terminal state and closed-loop capability.
- Main/role/complete checkpoint remains capped at 5000 ms.
- No map, seed, fuel, role identity, opponent, match-ID or wall-clock dispatcher.
- `m-4955` is consumed attribution only and cannot promote a successor.
- No designed functionality is removed, disabled, deferred or reduced; nothing
  is deleted.
