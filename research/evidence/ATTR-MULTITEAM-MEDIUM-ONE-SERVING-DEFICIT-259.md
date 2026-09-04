# ATTR-MULTITEAM-MEDIUM-ONE-SERVING-DEFICIT-259

Date: 2026-08-31

Parent production commit: `ab3d699`

Final Windows BTC binary SHA256:
`4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`

Consumed replay SHA256:
`C496ECDD163EE3A38AA412352AA0F6A8AF86F580C7699DE0113558F94CE09344`

This is read-only attribution. It changes no production source and has no
promotion or holdout authority.

## Runtime result

Fresh hard three-bot `m-5413` used six days, a 16x16 map, 48 steps/day,
five agents, fourteen spots, six brands, medium fuel and a public 10000-ms
response window. Canonical production ranked second at `6/36/126`, one serving
behind the winner at `6/36/127`. All 6/6 actions were exact-valid, both
validators agreed, all 5/5 transitions reconciled and no deadline, invalid,
emergency, continuation or rollback failure occurred. The terminal marginal
reservoir had zero routes, plans or accepted gains.

## Role attribution

All structurally valid role masks were replayed at the canonical 5000-ms
checkpoint in forward and reverse execution order. The final scores were:

- mask 0 `PPPPP`: `6/34/104` forward;
- mask 1 `TPPPP`: `6/36/131` in both orders;
- mask 2 `PTPPP`: `6/36/125` in both orders;
- mask 4 `PPTPP`: `6/36/125` in both orders;
- mask 8 `PPPTP`: `6/36/105` in both orders;
- mask 16 `PPPPT`: `6/36/126` in both orders;
- mask 20 `PPTPT`: `6/36/102` forward and `6/36/101` reverse;
- mask 24 `PPPTT`: `6/36/101` forward and `6/36/103` reverse.

The live assignment was mask 2. The timed role evaluator remained incomplete
for every retained mask and ranked the one-tanker identities within only two
rollout servings of one another; a fresh diagnostic run placed masks 16/4/2 at
`80/79/79` and mask 1 at `6/35/78`. It therefore does not resolve the exact
trajectory ordering.

Complete decision dumps for masks 1 and 2 have SHA256
`B4E132C0D75A9DD347EF02BA9A1B76C61E7A0F86BCC577CE6DF374FB368A2FE0` and
`3B2CE80FD593E024E5ED2540201CC194FE5D2848D8F6595D56AC0EE08055ED6E`.
Both score `6/6/24` on day 1, but their exact terminal state, selected plan and
own-road footprint already differ. Mask 1 then advances by daily servings
`24,23,21,19,19,25`; mask 2 advances by `24,20,20,20,17,24`.

## Authority boundary and verdict

The counterfactual engine replaces this team's role/state trajectory while
retaining the server-recorded road statuses. It does not recompute the complete
multi-team traffic that the alternate own footprint would have caused. Thus the
order-stable `131` is a real role-resolution witness, but not an authoritative
closed-loop score or a state-coupled dominance certificate.

This reproduces the already documented role-identity residual from 237--239.
The two-trajectory mechanism in 238 had same-count identity losses down to
`-29`; deterministic equal-depth coarse evaluation in SCORE-ROLE-032 made the
wrong evaluator stable; centrality, marginal-patrol, terminal-fuel, wider/longer
rollout and deadline allocation variants were also falsified. Repeating one of
those mechanisms, choosing mask 1, or routing by this fixture would be circular
and overfit.

Verdict: closed known role-identity residual, no safe SCORE successor. Resume
the unopened hard-three-bot screen. Reopen role identity only when a fresh
counterexample exposes a new public-state, state-coupled discriminator that
bounds same-count identity downside before assignment; do not reuse this replay
to tune such a discriminator.

Functionality-preservation answers: (1) this attribution removes, disables,
defers or reduces no designed production functionality; (2) nothing is deleted.
