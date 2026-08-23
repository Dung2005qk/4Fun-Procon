# DEADLINE-VIRTUAL-PARENT-186

Date: 2026-08-22

Parent: `0f01d69`

Frozen consumed manifest:
`research/holdouts/DEADLINE-VIRTUAL-PARENT-186.csv`

SHA256:
`D7C730502AAE4BAD2DA38A08887DE6585DE57F177FDA325C9DD5EE6E84DE4543`

## Question

Experiment 184 rejected exact full-transition equality as safe but inert and
rejected paired scenario dominance because it admitted a reproducible
closed-loop regression. Experiment 186 tested the strictly broader exact
simulation relation composed from the accepted methods of experiments 175 and
178:

1. ordered agent kinds and terminal cells are equal;
2. every patrol has no less terminal fuel;
3. current own-plus-opponent traffic is equal after componentwise saturation at
   the public jam threshold;
4. the challenger lifetime mask is a superset;
5. cumulative daily distinct and servings are componentwise no lower; and
6. daily distinct or servings is already strictly higher, so a later lifetime
   catch-up cannot turn the resend into a response-time-only loss.

Under this relation a future action sequence legal from the protected parent is
also legal from the challenger. Daily stock is reconstructed from
`MatchConfig`, so it is not cross-day state. If a witness existed, the safe
integration would preserve one virtual parent state, solve each future day once
from that state, replay the same action sequence on the authoritative richer
state, and require simulator-validator agreement plus the same relation after
every day. Replanning from the richer state was explicitly forbidden because
the bounded heuristic is not monotone.

## Consumed gates

Only the already-opened 184 states `4310000` and `4411000` were used. The
protected engine received 5000 ms. A separate canonical exploration engine
received the remaining 10000 ms and was advanced only by the selected protected
history. Production, sealed holdouts and BTC were unchanged.

An initial run was invalidated before verdict because its temporary unclamp hit
`select_roles_until` while the fixed-role harness called a still-clamped
`solve_day`. The corrected executable asserted on every day that the parent
deadline total was exactly `5000 ms` and the challenger total was exactly
`10000 ms`.

The corrected selected Long sweep produced eight different plans on each
fixture, close to the nine previously observed in experiment 184. None was
liftable and invalid was zero. On `4411000`, days 5 and 8 gained one serving,
but their terminal/fuel or traffic relation failed; the gain therefore could
not be protected by virtual-parent continuation.

To avoid confusing selector choice with structural absence, the preregistered
existence sweep inspected every already exact-evaluated candidate in each
correctly budgeted Long pool:

| Seed | Days | Different pool candidates | Liftable | Invalid |
|---|---:|---:|---:|---:|
| 4310000 | 10 | 316 | 0 | 0 |
| 4411000 | 10 | 318 | 0 | 0 |
| Total | 20 | 634 | 0 | 0 |

Each pool exposed 31--32 plans different from the protected parent per day.
Every one was rerun through `ExactStepSimulator` and
`IndependentDayValidator` before the relation was evaluated. No candidate
passed all six conditions, so there was no virtual-parent continuation witness
to test.

Local elapsed time and selected-plan stability have no BTC performance
authority. The pool result is semantic existence evidence over the generated
candidate sets, not a latency claim.

## Verdict

`rejected-sound-but-inert`.

The fuel-monotone virtual-parent certificate is mathematically valid, but the
correctly budgeted canonical Long pools on both strongest consumed deadline
counterexamples contain zero usable witness. Adding the certificate to
production would add inactive complexity without improving an action or score.
Fresh development, sealed holdout and BTC remained unopened, and all
experiment-only source and harness instrumentation is removed.

Reopen only after a new independently generated, exact-valid candidate satisfies
the full relation with persistent score gain. Do not weaken traffic equality,
accept lifetime-only strictness, replan the future from the richer state, tune
by fixture metadata or rerun these consumed pools for a cutoff accident.

## Functionality-preservation check

1. No designed production functionality was removed, disabled, deferred or
   reduced.
2. No canonical implementation was deleted. Only rejected experiment-only
   observability and an isolated extended-budget research build were removed.
