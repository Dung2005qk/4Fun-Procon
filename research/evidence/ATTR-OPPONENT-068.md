# ATTR-OPPONENT-068 — peer-favorable compute-ceiling counterexample

Date: 2026-08-11

## Frozen question

How much additional score headroom does the external peer expose on its
strongest already-opened win when it receives the long compute profile its own
implementation targets? This fixture cannot test whether long peer compute
reverses a UDON win, because UDON already lost the opened comparison.

- Fixture: `attr-opponent-066-low-btc-large-overnight-seed-72005`.
- Map: BTC-like `32x32`, 10 days, 100 steps/day, low fuel `120`.
- Lane: fixed one-tanker-last roles.
- Judge: common UDON exact simulator plus independent validator.
- UDON internal budget: `5000 ms` per day, unchanged.
- Peer daily budget: `45000 ms`.
- Peer profiles: production default, whole-match MLNS plus fuel-aware repair,
  and release-b-all. The best valid peer profile is an intentionally favorable
  oracle upper bound for this one opened fixture.
- Manifest: `research/holdouts/ATTR-OPPONENT-068.csv`, SHA256
  `627BA277755FF9D11D988E92C5C80220E9700985FD6ADACA41D67CA8EE12E72E`.

No solver source was changed. Only the external harness was parameterized to
pass separate UDON/peer budgets and published peer feature profiles.

## Result

| Solver/profile | Common-evaluator result | Valid | Diagnostic max local day |
|---|---:|---:|---:|
| UDON operational | `6/60/418` | yes | `2490 ms` |
| Peer production default | `6/60/462` | yes | `22451 ms` |
| Peer whole-match MLNS + fuel-aware | `6/60/462` | yes | `30280 ms` |
| Peer release-b-all | invalid on day 8 | no | `34332 ms` |

The peer oracle is therefore `6/60/462`, beating UDON by 44 servings at tier 3.
UDON does not win this particular counterexample on the peer's long-compute
field.

Whole-match MLNS completed a robust phase on all nine days that had a future
day (`9/9`) but emitted the same final match score as production default. The
broader release-b-all profile completed eight robust epochs, then the common
evaluator rejected day 8 with `SimulationErrorCode::InsufficientFuel`: agent 2,
step 47, patrol movement accepted without enough fuel. Invalid output is not
eligible for the oracle.

Both valid peer profiles disagreed with the common authoritative semantics on
all ten days. This did not invalidate their emitted actions, but confirms that
extra compute did not repair the private-simulator transition mismatch.

## Relation to the earlier 5000 ms observation

The prior ATTR-OPPONENT-067 raw row on this fixture was UDON `6/60/379` versus
peer `6/60/438`, a peer margin of 59 servings. The new long-profile run was UDON
`6/60/418` versus peer `6/60/462`, a margin of 44. The peer score is 24 servings
higher, but the UDON score also changed by 39 under a separate local run.
Consequently the cross-run delta cannot be attributed causally to compute; local
load, cutoff and wall-clock search variance remain confounded. The controlled
facts are narrower: the 45000 ms production-default and whole-match profiles tie
at `462`, and both beat the single contemporaneous UDON result at tier 3.

Local elapsed is reported only to prove the peer profiles received and stayed
within their requested 45000 ms window. It is not BTC target-host performance
evidence.

## Verdict

Accepted as external attribution evidence about one known peer-win lane. It
does not answer whether long peer compute can reverse a UDON win; that corrected
question is tested separately by ATTR-OPPONENT-069. More compute is not
sufficient to make the peer whole-match layer exceed its default result here,
and enabling the broadest experimental portfolio exposes the already-known
fuel-validity class.

This result does not reverse the diverse ATTR-OPPONENT-067 aggregate, does not
open a UDON optimization axis, does not authorize a source change or commit, and
cannot be generalized beyond this deliberately opened counterexample.
