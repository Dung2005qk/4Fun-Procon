# SEM-REFUEL-MIDMOVE-205

Parent: `690728a`. Read-only attribution; no production or test source change.
Probe: `research/probes/refuel_midmove_probe.cpp` (new research-only target
`udonshield_refuel_midmove_probe`).

Frozen manifest: `research/holdouts/SEM-REFUEL-MIDMOVE-205.csv` (all fifteen
archived BTC replays with SHA256 and sizes), SHA256
`E01CA37756C9CFD96FF6738BEFC3443A4738B6B6DB7048ABF3456D6C975E3962`.

## Gap

The canonical rendezvous primitive always charges the patrol a hardcoded
`WAIT(1)` at the meeting cell (`src/planner.cpp:3594`/`3619`), while the exact
simulator keeps a moving agent at its source cell until the move completes.
The existing immediate-departure regression asserts only `final fuel < limit`,
which cannot distinguish refuel-then-deduct from no-refuel, so the in-house
mid-move semantics were unpinned and real-server behavior was unverified.

## Phase (a): dual-engine micro-probes (`semantics` mode)

All cases require simulator/independent-validator agreement (all agreed):

- `A-immediate-departure`: patrol arrives at the rendezvous at boundary 2 and
  departs immediately with a two-step plain move; the waiting tanker refuels
  it **mid-move at boundary 3** (timeline `10/10/9/20/19`). Zero-dwell refuel
  is real in both engines. The old test message "must not refuel" describes
  intent, not behavior; the behavior is refuel-then-deduct.
- `B-midmove-interception`: a patrol one step into a three-step mountain move
  is refueled at boundary 1 by the tanker standing on its source cell.
- `C-road-one-step-departure`: a one-step smooth-road departure shares only
  one boundary with the tanker: no refuel (the boundary condition).
- `D-tanker-midmove-provision`: a tanker standing on its own move source
  still provides; the waiting patrol refuels at boundary 1.
- `E-arrival-onto-tanker`: arriving onto a waiting tanker refuels one
  boundary after arrival, not at arrival (timeline `10/10/9/9/8/20/20`).

## Phase (b): archived replay scan (`scan` mode)

Log: `research/evidence/SEM-REFUEL-MIDMOVE-205-scan.log`, SHA256
`C04B60651C542509B558B7793A86500BC39A017F1D1C51B5D16CC6D1AF186313`.

- 15/15 replays analyzed; every analyzed day's accepted production plan
  re-simulated with dual-engine agreement; probe fuel walk asserted equal to
  the simulator trace at every boundary (including the SEM-REFUEL-002
  terminal re-record).
- **555 mid-move refuel firings** occurred inside real accepted production
  plans across 12 of 15 replays; 111 days are distinguishing (final fuel
  differs from a stationary-only counterfactual).
- **4 fuel-critical days** (m-3878 x2, m-3879, m-3880): the accepted plan is
  fuel-feasible only if the server refuels mid-move; a stationary-only server
  must have rejected them with `E_NO_FUEL`. All were accepted and their
  next-day authoritative fuels match the mid-move simulation.
- Authoritative next-day agents matched the simulator on **109/110**
  checkable transitions.

Verdict on the primary question: **the BTC server implements mid-move refuel
exactly as both local engines do.** The semantics are pinned server-side.

## Side finding: cross-team refuel hypothesis (single mismatch)

The one mismatch is `m-3876` day 9, agent 1 (a patrol): predicted `1012@100`,
authoritative `1012@148` (+48, position exact). Single-refuel arithmetic pins
a refuel-to-full at the unique boundary where our simulated fuel was 152
(boundary 19, cells 461/493/524). The only opponent tanker in that match
(team1 agent 7) started the day at 811 and ended at 842, adjacent to our
patrol's mid-day corridor (810 -> 842 at boundaries 35-38) and
geometrically able to intercept earlier via roads. The official rule text
("Xe tiếp nhiên liệu ở cùng ô với xe tuần tra >= 1 bước sẽ nạp đầy") does not
restrict refueling to the same team, and the still-open `m-0922` anomaly
(+7 fuel with own tanker elsewhere) fits the same explanation. Status:
plausible, not proven (opponent mid-day trajectories are not public).
Impact if true: fuel only increases, so plan validity is never at risk;
daily reconciliation already adopts authoritative fuel; the only cost is
slightly pessimistic witnesses in rare co-location cases. No candidate is
warranted; recorded so future reconciliation mismatches are not
misattributed.

## Consequence

A production SCORE candidate (additive no-wait rendezvous variants that
reclaim the hardcoded WAIT(1) per refuel) is now unblocked: the required
server semantics are confirmed by 555 in-match firings, 4 fuel-critical
accepted plans and 109/110 exact transitions. Production already implicitly
relies on mid-move refuel; it just never exploits it deliberately.
