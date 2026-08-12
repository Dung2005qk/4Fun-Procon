# ATTR-OPPONENT-069 — can long peer compute reverse a prior UDON win?

Date: 2026-08-11

## Frozen question

ATTR-OPPONENT-068 replayed a fixture the peer already won. That established a
peer strength but could not answer the requested reversal question. This
experiment instead selects the closest both-valid BTC-like UDON win from
ATTR-OPPONENT-067 and asks whether the unchanged peer can overturn it when
given its long compute profile.

- Fixture: `attr-opponent-066-low-btc-large-high-stock-seed-72004`.
- Map: BTC-like `32x32`, 10 days, 100 steps/day, low fuel `120`.
- Lane: fixed one-tanker-last roles.
- Prior opened score: UDON `6/60/416`, peer `6/60/404`; UDON +12 at tier 3.
- UDON budget: `5000 ms` per day.
- Peer budget: `45000 ms` per day.
- Peer profiles: production default, whole-match MLNS plus fuel-aware repair,
  and release-b-all.
- Judge: common UDON exact simulator plus independent validator.
- Manifest: `research/holdouts/ATTR-OPPONENT-069.csv`, SHA256
  `B469189A5B192772A352A48C54F47183D956FA51721AC04ED390D52D02A8740D`.

Neither solver source was changed. The peer profiles ran sequentially to avoid
cross-profile CPU contention.

## Result

| Solver/profile | Common-evaluator result | Valid | Diagnostic max local day |
|---|---:|---:|---:|
| UDON operational | `6/60/489` | yes | `2515 ms` |
| Peer production default | `6/60/448` | yes | `21670 ms` |
| Peer whole-match MLNS + fuel-aware | `6/60/448` | yes | `27216 ms` |
| Peer release-b-all | `6/60/414` | yes | `31574 ms` |

The best valid peer score is `6/60/448`. UDON wins the contemporaneous
comparison by 41 servings at tier 3. Therefore the prior UDON win survives all
three frozen 45000 ms peer profiles on this fixture.

Whole-match MLNS did not improve the peer default score. It reported robust
completion on 7 of the 9 days with a future horizon. The release-b-all bundle
remained valid here but reduced the peer result by another 34 servings. Every
peer profile stayed within its requested local 45000 ms window.

All peer profiles disagreed with the common authoritative transition semantics
on 9 of 10 days. Their emitted actions were nevertheless valid under the common
judge; private scores were not substituted for official scores.

## Scope and causality

The prior opened row was UDON `416` versus peer `404`; the new run was UDON
`489` versus best peer `448`. Both scores changed across separate wall-clock
runs, so the changes cannot be causally attributed to additional peer compute.
The controlled conclusion is only the within-run one: under the frozen long
profiles, the best valid peer action sequence still loses to the contemporaneous
UDON sequence by 41 servings.

Local elapsed confirms that the requested profile was exercised and stayed
inside 45000 ms. It is not BTC performance evidence. One selected fixture does
not establish general dominance, open a UDON optimization axis, authorize a
source change or authorize a commit.

## Verdict

Accepted as corrected external attribution evidence. On the selected closest
BTC-like prior UDON win, granting the peer its long compute window does not
reverse the result. ATTR-OPPONENT-068 remains evidence about a known peer-win
lane only; it is not evidence for this reversal question.
