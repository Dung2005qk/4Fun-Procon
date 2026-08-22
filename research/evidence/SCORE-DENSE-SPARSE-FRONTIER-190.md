# SCORE-DENSE-SPARSE-FRONTIER-190

## Registered scope

- Parent: `5c3aa7a`.
- Frozen manifest: `research/holdouts/SCORE-DENSE-SPARSE-FRONTIER-190.csv`.
- SHA256: `0391DE241480EAA0A2188CAE9714A03DE129941625147E9F8C47967A09FA698C`.
- Official comparison: lifetime distinct, then cumulative daily distinct, then
  servings.
- Internal compute cap: 5000 ms. Local elapsed time is not performance evidence.

## Consumed BTC counterexample

`m-3877` has a 32x32 map, 24 spots, 8 agents, 10 days and high fuel. All 24
spots are individually reachable by every patrol on every opened day, but the
dense exact representation is unsupported. The recorded current result is
`6/60/458`; the bot result and rank are excluded from promotion evidence.

A sparse Pareto frontier over unchanged legal transitions found a strict
single-agent exchange on every immutable replay state. The cumulative parent to
candidate serving changes for days 1 through 10 were:

`46->48, 91->93, 135->137, 183->186, 227->230, 273->275, 321->323,
365->368, 411->413, 458->460`.

Every candidate was checked by the exact simulator and independent validator.
With the registered 32-route cap, day 10 still selects agent 6 and reaches
`6/60/460`.

## Rejected integration

Adding sparse portfolios to the ordinary planner changed earlier closed-loop
choices and ended at `6/60/454`. This mechanism is rejected. Requiring the
nonterminal route to preserve the incumbent transition and road footprint found
no strict consumed gain. The main planner was restored to HEAD and has no diff.
The sparse enumerator is exposed only through an explicit API called by the
terminal sidecar.

## Terminal sidecar

The sidecar is eligible only on the final day when dense exact state is
unsupported. The parent remains immutable. Each one-agent replacement must be
valid and must strictly improve the official cumulative score. A deadline,
unsupported state, invalid route or absence of a strict improvement returns the
byte-identical parent.

Production-path replay probe on `m-3877`:

- parent `6/60/458`;
- selected `6/60/460`;
- witness agent 6;
- sparse routes evaluated 224;
- simulator/validator-valid 224;
- strict improvements 2;
- invalid 0.

Fresh 32x32, 24-spot, high-fuel fixed-role development seed `4713000`, using one
shared absolute 5000-ms solve-plus-sidecar cap:

- terminal parent `6/60/509`;
- terminal candidate `6/60/512`;
- strict improvements 2;
- invalid 0.

This second witness is independent of `m-3877`. It proves score value beyond the
consumed replay but does not provide target-host latency authority.

## Fresh development matrix

The 54-case development split ran with one shared absolute 5000-ms daily
solve-plus-sidecar cap. Protocol-impossible spot counts were excluded by the
published invariant `spotCount <= max(width,height)`; the manifest hash and all
valid registered axes remained unchanged.

- Overall: `25/29/0`, +92 servings, maximum gain +9.
- Easy: `0/12/0`, +0; all 12/14-spot exact-feasible controls tied.
- Medium: `4/8/0`, +18.
- Hard: `11/4/0`, +43.
- Very hard: `10/5/0`, +31; two dense cases exhausted local headroom and
  returned the parent.
- Fixed roles: 12 wins, +41; native roles: 13 wins, +51.
- Default fuel: 12 wins, +57; high fuel: 13 wins, +35.
- Spot counts: 12 => `0/17/0`; 14 => `7/10/0`; 18 => `11/0/0`;
  24 => `5/1/0`; 30 => `2/1/0`.
- All six generated families contain strict wins.
- Generated/validated sparse alternatives: 4,366/4,366.
- Invalid: 0. Loss: 0. Terminal-sidecar deadline returns: 2.

All gains are tier-3 gains after lifetime and cumulative daily distinct tie.
This is expected at the final-day dense coverage boundary and is reported by
official tier rather than a weighted sum. Development passes and permits the
sealed holdout to open. Local timing remains non-authoritative.

## Frozen holdout

The 108-case frozen holdout opened only after the candidate hashes below were
recorded. It used the same shared absolute 5000-ms daily solve-plus-sidecar cap
and the same published protocol-valid intersection as development.

- Overall: `53/55/0`, +220 servings, maximum gain +8.
- Easy: `0/24/0`, +0.
- Medium: `8/16/0`, +44.
- Hard: `22/8/0`, +94.
- Very hard: `23/7/0`, +82.
- Fixed roles: `26/27/0`, +104; native roles: `27/28/0`, +116.
- Default fuel: `27/27/0`, +130; high fuel: `26/28/0`, +90.
- Spot counts: 12 => `0/34/0`; 14 => `14/20/0`; 18 => `21/0/0`;
  24 => `13/0/0`; 30 => `5/1/0`.
- All six traffic families contain strict wins: balanced 6, fuel-tight 9,
  high-stock 10, overnight 9, rare-brand 12 and threshold-corridor 7.
- Generated/planned/validated sparse alternatives: `9,271/9,270/9,270`.
- Lifetime and cumulative daily deltas: 0. Invalid: 0. Emergency: 0.
  Failure: 0. Loss: 0.
- Two very-hard 30-spot cases exhausted their sidecar slack and returned the
  exact parent; neither exceeded the shared solver deadline or changed score.

The production diff, manifest and runner hashes remained unchanged through the
entire holdout. Holdout evidence therefore establishes broad, zero-downside
semantic value. It does not supply target-host latency authority; authenticated
BTC execution remains the final promotion gate.

## BTC target-host gate

Authenticated match `m-3896` used the intended dense boundary directly:
32x32, 10 days, 100 steps/day, 8 agents, 24 spots, 6 brands, high fuel and a
5000-ms response window. The production binary recorded all 10 ACKs as valid.

- maximum authoritative response: 2,821 ms;
- response sum: 28,024 ms;
- final-day sparse routes: 192;
- exact-simulated and independently validated plans: 192;
- strict improvements: 2;
- final-day parent `6/37`, selected `6/42`;
- cumulative virtual parent `6/60/402`, submitted result `6/60/407`;
- sparse failure: false; deadline reached: false;
- replay-check: 10/10 days valid, validator agreement 10/10, nine transitions
  reconciled, final `6/60/407`.

The bot rank and bot score are excluded. This match has authority only for
protocol, validity, lifecycle, target-host timing and proof that the terminal
sidecar takes over on its intended production path. Those gates pass.

## Verdict

Accepted. The candidate closes the consumed `m-3877` capability gap, improves
broad fresh development and frozen holdout with zero paired loss, preserves all
existing planner and experiment-187 behavior, and executes validly inside the
5000-ms hard cap on BTC. Experiment 185 remains an independent oracle gap and
continues unchanged.

Candidate frozen before holdout:

- production diff hash: `77e1d3c28fa96352e01f629925c631bccc409108`;
- research/test diff hash: `8d2d355c82e78df1ae7805c7f43fcf081a18d9da`;
- matrix-runner SHA256:
  `FC28678699A3729C3645F12120025992735142A193EFAAB53EB6C88AFC43EF4A`.

No production logic may be changed in response to the opened holdout.

## Functionality-preservation answers

1. The candidate removes, disables, defers or reduces no designed function.
2. Nothing is deleted, so no replacement-equivalence claim is needed.

The 187 nonterminal protected WAIT path and all existing planner paths remain
active and unchanged. The holdout was opened once after the candidate freeze
and was not used to tune production logic.
