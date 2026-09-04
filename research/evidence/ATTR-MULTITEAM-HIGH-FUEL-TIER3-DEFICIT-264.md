# ATTR-MULTITEAM-HIGH-FUEL-TIER3-DEFICIT-264

## Authority and observed loss

Fresh hard three-bot match `m-5502` used eight days, a 32x32 map, 64
steps/day, six agents, twenty-four spots, eight brands, high fuel and a public
`10000 ms` response window. Signed production finished second at `8/64/195`,
two servings behind `8/64/197`. Replay SHA256 is
`A0099166241F92DB6360D8974E56286B4A7F5968F5FB0B6AEAF42ABA86879055`.
All 8 actions are exact-valid with independent-validator agreement, all 7
transitions reconcile, response times are `3534..7290 ms`, and stderr is empty.
The replay is consumed attribution evidence and has no promotion authority.

Days 7 and 8 each deliver only 21 servings despite full lifetime and daily
coverage. On both days tanker 0 waits from cell 912 while patrol 1 starts on a
spot at cell 107 with fuel 2, takes that serving and waits. The live master hit
its deadline on both days with optimistic upper bounds exactly one serving above
the selected score: `8/56/174` versus `8/56/175`, then `8/64/195` versus
`8/64/196`. Four fresh same-binary solves per day remained at 21 servings, so
this is not a one-off rerun recovery.

## Exact loss boundary

The unchanged production-width route portfolio retains `12/5/12/12/12/12`
columns and no complete rendezvous pair. A research-only no-deadline portfolio
with the same generator and cap 64 retains `45/5/64/64/64/64` plus four
complete pairs. On both days, groups 16--18 replace only tanker 0 and patrol 1,
refuel at cell `107@36`, and dual-validate at exactly +1 serving:

- day 7: `8/56/174 -> 8/56/175`;
- day 8: `8/64/195 -> 8/64/196`.

The cap-64 day-7 witness group 18 ends patrol 1 at cell 178 with fuel 183 and
tanker 0 at cell 109; the same group is valid on day 8. Cap 12 retains neither
half as a complete atomic group. The observed generator creates both halves
before `prune_columns`, then each agent is pruned independently. This is a real
portfolio-integrity gap, not opponent variance and not the known changed-state
sparse residual.

Exact logs:

- day7 cap12 SHA256 `69C25A475C1C28EBD772A88D595D513FD76E85CD696442A2EC2DDE83D21944D1`;
- day7 cap64 SHA256 `CE4635B27C1AC8C231B1C35E3C80E60EAA28B735985C849AD49F5B068CBF6E0E`;
- day8 cap12 SHA256 `E1A88A04704F08A5D9338D851B0887BE65CD81AEE18CC4F80583BC28499319EC`;
- day8 cap64 SHA256 `4CB02864A098F7096CA2D4B5EB3E09BBB9D7AEC44CC61673F1ED8BD83157EAF8`.

Research probe source SHA256 is
`84C00EF1CF7914F823C65E852614E4F764797A66781391BF6A161CCDA39E8666`;
binary SHA256 is
`9F011263DB7A3929A2EF4B8E2666F3F7C2EEDAC58DA44D97B15B7AF8AEC8A4D0`.

## Classification and successor boundary

SCORE-ROLE-005 previously observed the same structural risk in pre-match role
rollout and explicitly allowed reopening for a separate portfolio-integrity
audit. It was inert on its role-ranking counterexample, so its result is not
promotion evidence here. SCORE-REFUEL-NOWAIT-206 proves that blindly growing the
deadline-sampled escort pool is unsafe. Therefore the authorized successor may
not raise the per-agent cap, add every pair to the master, or route on this
fixture.

Experiment 264 closes as accepted attribution and opens
SCORE-ORPHAN-RENDEZVOUS-PAIR-SIDECAR-265 on wholly fresh evidence. The successor
must leave the ordinary pruned portfolio and complete parent search unchanged,
carry only bounded complete groups outside the master pool, exact-evaluate
atomic substitutions against already-produced parent candidates, retain the
parent, and let the existing future-profile/certification path decide among the
resulting candidates.

Functionality-preservation answers: (1) this attribution removes, disables,
defers or reduces no designed functionality; (2) nothing is deleted.

