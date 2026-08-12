# ATTR-OPPONENT-067 — external action-output A/B

Date: 2026-08-11

## Frozen inputs

- UDON-SHIELD: `7ef36949e1b166c862a30f52ecb3bd9c9fb210ea`.
- External peer `thing-or-think/hexudon-procon`:
  `1f0d22efdf5cb8d74b9a263fecfb9fa5cf85bf4d`.
- Manifest: `research/holdouts/ATTR-OPPONENT-067.csv`, SHA256
  `EDA237490DC2C94239ECE93B85CE3DCE2B2EC459374FC68D34D6E776C1F8F47A`.
- The consumed semantic pilot seed `36000` is excluded. The scored suite has
  23 fresh structural fixtures and both fixed/native lanes: 46 paired matches.
- Every pair shares the official-valid map, common exogenous footprints,
  endogenous own two-day traffic, fuel, horizon and role lane. Fixed uses one
  tanker in the last slot. Native lets each unmodified solver select roles.
- Each role/day entry point receives `5000 ms`. Local elapsed is recorded only
  as diagnostic telemetry and has no BTC performance authority.
- Both outputs are scored only by the UDON exact simulator and independent
  validator. The peer's internal score is never substituted for the common
  official score.

## Comparative result

- Overall UDON W/T/L: **40/0/6** over all 46 pairs. A peer invalid counts as a
  hard pair loss.
- Both-valid score W/T/L: **33/0/6** over 39 pairs.
- Fixed lane: **19/0/4**. Native lane: **21/0/2**.
- UDON output: 0 invalid. Peer output: 7 invalid in the frozen run.
- First differing tier among both-valid pairs:
  - UDON won 6 pairs at tier 2 and 27 pairs at tier 3.
  - Peer won 6 pairs, all at tier 3. It never won lifetime or daily distinct.
- UDON tier-2 winning deltas: `n=6`, min `+1`, median `+1.5`, max `+3`,
  within-tier sum `+11`.
- UDON tier-3 winning deltas: `n=27`, min `+6`, median `+54`, max `+118`,
  within-tier sum `+1407`.
- Peer tier-3 winning deltas: `n=6`, min `+1`, median `+23.5`, max `+59`,
  within-tier sum `+148`. These within-tier sums are descriptive and are never
  combined across lexicographic tiers.

### By lane and fuel

| Slice | UDON W/T/L |
|---|---:|
| generated 8x8 fixture fuel | 10/0/0 |
| BTC-like low fuel | 8/0/4 |
| BTC-like default fuel | 10/0/2 |
| BTC-like high fuel | 12/0/0 |
| fixed roles | 19/0/4 |
| native roles | 21/0/2 |

### By family

| Family | UDON W/T/L |
|---|---:|
| balanced | 6/0/0 |
| fuel-tight | 8/0/0 |
| high-stock | 8/0/0 |
| rare-brand | 7/0/1 |
| threshold-corridor | 7/0/1 |
| overnight | 4/0/4 |

All six peer wins were serving-only: low-fuel rare fixed (`+29`), threshold
fixed (`+35`), overnight fixed/native (`+59/+18`), and default-fuel overnight
fixed/native (`+1/+6`). Their strength is therefore real but narrow: portions
on selected large low-fuel/overnight lanes, not general lexicographic strength.
Every high-fuel pair was won by UDON, commonly by `+65..+118` servings.

## Semantic and validity attribution

The preflight proved that action direction encoding and basic movement mapping
are compatible, but the peer's private simulator is not semantically equivalent:

- It credits a patrol already standing on a spot at day start; the two UDON
  engines credit a serving only after a completed move.
- It also disagrees on some final agent states and road footprints. The harness
  preserves the peer's private planner state but feeds the authoritative UDON
  final state into the next day, matching the live server lifecycle.
- Peer self-semantic parity failed in 45/46 runs, spanning 338 mismatch-days.

The frozen full run observed 7 peer-invalid lanes. Targeted attribution
reproduced 6/7 as `SimulationErrorCode::InsufficientFuel` (`code=8`): the peer
accepted a patrol move after its private refuel/fuel transition had diverged
from the common evaluator. The high-fuel threshold fixed invalid did not recur;
its rerun was valid at `6/60/343`, so that seventh sample is classified as
cutoff-sensitive/transient local behavior, not a deterministic correctness
claim. The six reproducible invalids remain a confirmed semantic failure class.

Local elapsed crossed 5000 ms in 20/46 peer runs and 0/46 UDON runs. Per the
research contract this is only local-load/cancellation telemetry; it is not a
target-host performance verdict and is not used in W/T/L promotion logic.

## Verdict

The comparison is accepted as external comparative evidence: UDON-SHIELD is
the comprehensive winner against this frozen peer under the common official
evaluator. The peer is capable of isolated tier-3 wins, especially overnight,
but does not threaten lifetime/daily tiers, loses broadly outside those lanes,
and has a reproducible fuel-validity defect.

This experiment changes no production source, opens no optimization axis and
authorizes no commit. The peer's narrow winning lanes are opponent-level
context only; future UDON research must still originate from independently
measured UDON telemetry/counterexamples and pass the protected matrix.

## Evidence files

- `research/evidence/ATTR-OPPONENT-067-raw.csv` — 92 solver rows, SHA256
  `A2095F903458D53048159F5E76A9FCCE29636AB06FF15F842353B7D8EBA1FDBC`.
- `research/evidence/ATTR-OPPONENT-067-pairs.csv` — 46 paired rows, SHA256
  `A24A04C95D0D0227B3A1BE8190E56FBFD068EBFC30B87EFA6A0BA6A2E2F7BC7A`.
- `research/evidence/ATTR-OPPONENT-067-invalid-diagnostics.csv` — 7 targeted
  rows, SHA256
  `2EAF559DDDC6D590911477278C41D6F1654EF2563DD92A023A792F3B530E2182`.
