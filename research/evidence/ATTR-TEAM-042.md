# ATTR-TEAM-042 complete resource team DP

Date: 2026-08-09
Parent: `afcd2da`
Verdict: accepted multi-agent feasibility proof

Complete exact resource frontiers were enumerated without a wall deadline for
all six patrols in frozen `m-1285` day 10:

- agents 0, 1, 2, 4 and 7: `complete=1`, 3,148,570 settled states, 78
  inclusion-maximal routes each;
- agent 3: `complete=1`, 2,831,326 settled states, 76 inclusion-maximal routes.

A deterministic count-state DP capped every per-spot count at public stock and
used official daily brands then servings. It found a target-36 state after 38
nodes and seven memo states. The reconstructed plan retained the frozen tanker
actions and used these patrol masks:

- agent 3: `0xB59`;
- agents 0, 1, 2 and 4: `0x9D9`;
- agent 7: `0x1EC`.

The exact simulator and independent validator agreed on `6/60/322`. Therefore
the production guidance value 322 is feasible, but it requires a coordinated
multi-patrol route change; the negative one-exchange proofs do not make the
bound loose.

The capped production anytime frontier for representative agent 0 retained 24
six-spot and eight five-spot masks and omitted the seven-spot `0x9D9`. The full
proof frontier contains 76--78 maximal routes. The actionable mismatch is thus
the anytime exploration order: the queue is ordered by shallow used steps/fuel
while retention is ordered by route cardinality first. A candidate may align
those objectives under the unchanged 1,250,000-state cap, but may not add these
mask bytes, raise the cap or route on this replay.
