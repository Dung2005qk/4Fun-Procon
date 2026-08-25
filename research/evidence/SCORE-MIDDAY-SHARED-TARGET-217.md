# SCORE-MIDDAY-SHARED-TARGET-217

## Frozen contract

- Parent: `177b588`.
- Manifest: `research/holdouts/SCORE-MIDDAY-SHARED-TARGET-217.csv`.
- Manifest SHA256:
  `B17826FE51D17D0FA217E60699A26E6F6DC42A98E70C5053942E621FF7316940`.
- Holdout status: sealed and unopened.
- Official score comparison is lexicographic. Local elapsed is not performance
  evidence.

## Structural gate

The combined sparse traversal was compared with the two standalone views on
eight consumed replay-days covering 24x24 and 32x32 maps, low/default fuel and
four/eight-agent configurations. All 40 patrol/day comparisons reproduced the
global and incumbent-terminal retained route pools exactly: eight completion
markers and zero parity failures.

## Rejected wiring build

Tournament binary SHA256:
`2DD7114D8BA5D6B3C2EC748D1F3A2CEEE1BF8FD02FD865F74FC53D16D34C6FAD`.

Development completed 60 paired cases with zero invalid, emergency or mid-day
failure. It failed decisively:

- W/T/L: `3/26/31`;
- net servings: `-528`;
- gains: three cases, total `+14`, range `+1..+10`;
- losses: 31 cases, total `-542`, range `-1..-75`;
- all 34 first differences were tier 3;
- long-window W/T/L: `0/4/26`;
- low-fuel W/T/L: `0/10/15`;
- target routes/plans/valid/acceptances on-side: `0/0/0/0`;
- shared-target diagnostic days on-side: `436`.

The cause is exact and source-local: combined enumeration was wired into
`refine_terminal_sparse`, but `refine_midday_chains` still used the global-only
enumerator. Its shared target phase therefore moved empty target vectors and
skipped the accepted 215 enumeration. This build does not test the registered
mechanism and cannot open holdout.

Preserved rejected logs:

- `SCORE-MIDDAY-SHARED-TARGET-217-development-miswired-off.log`, SHA256
  `1B6339301E8A79932E8BE92A7CAD58588B3472C42FD6743FEDF680FEFF3608E2`;
- `SCORE-MIDDAY-SHARED-TARGET-217-development-miswired-on.log`, SHA256
  `D6C02896C0A23D3FCA8C13EEA8E79B49DC3536A77E69F619414D0456EB0935B0`.

## Corrected source freeze

Tournament binary SHA256:
`F848AE893D5F9D38F30BC99EA68CD6AEC293151E9057B9A431A5EF44A66230A4`.

Claim-probe binary SHA256:
`1A93C307D3D6747C7CDF4F340165762FC11C429B2A7E0BB0C96DA691673DF0`.

The terminal-sparse path is restored to the parent enumerator. Combined
retention now exists only in the initial `refine_midday_chains` traversal. A
one-case target-host smoke produced 576 target routes, 318 generated and
dual-valid target plans, three target acceptances and zero safety failures.
The corrected development rerun may use the same development seeds; no holdout
row has been read and no score threshold or parameter was tuned.

## Corrected development result

The corrected 60-case paired gate passed clearly:

- W/T/L: `29/28/3`;
- net servings: `+246`;
- gains: 29 cases, including `+50` maximum;
- losses: three very-hard long-window cases at `-1`, `-10`, `-1`;
- all first differences were tier 3;
- fuel W/T/L: low `13/11/1`, default `11/7/1`, high `5/10/1`;
- role W/T/L: fixed `18/19/2`, native `11/9/1`;
- short-window W/T/L: `23/7/0`; long-window W/T/L: `6/21/3`;
- target acceptances: on `358`, off `239`;
- target-acceptance-conditioned W/T/L: `29/18/3`;
- invalid/emergency/mid-day failures: zero;
- runtime attribution only: mean `2443/2444 ms`, maximum `3024/3023 ms`
  off/on. BTC remains the performance authority.

Corrected logs:

- `SCORE-MIDDAY-SHARED-TARGET-217-development-causal-off.log`, SHA256
  `EEC042238E1E8B5952EA0D76E21F3C4C0113C2AF4CA8E179F5D17D73E5F3EA95`;
- `SCORE-MIDDAY-SHARED-TARGET-217-development-causal-on.log`, SHA256
  `4F7A70004FEE2F272E4D9D9495133BD0E17AFB7DAAF0687BAB697C20BFE3CA15`.

The gains are broad across fuel and role strata. The three bounded tier-3
losses are confined to the very-hard long-window stratum and are outweighed by
the paired global benefit, so the pre-registered development gate authorizes
one opening of the sealed holdout without any source or threshold change.

## Sealed holdout result

The frozen holdout was opened exactly once on the corrected binary and completed
all `108 + 108` causal pairs:

- W/T/L: `40/58/10`;
- net servings: `+455`, gross gain/loss `+491/-36`;
- gain/loss tail: `+75/-9`;
- all 50 first differences were tier 3;
- invalid, emergency, mid-day failure and terminal-sparse failure: zero;
- map/difficulty, spot-count, fuel and role strata were globally positive or
  tied, including spots 30 at `10/3/0`, `+136`;
- short protected windows were strongly positive at `35/14/5`, `+466`;
- long protected windows failed the stratified gate at `5/44/5`, `-11`;
- `fuel-tight` and `rare-brand` families were also net negative at `-4` and
  `-11` respectively.

The on-side increased target routes/plans/valid/acceptances from
`132671/106637/106637/319` to `213277/167679/167679/543`, and recorded 772
shared-target days. The mechanism is therefore active and highly valuable when
the second standalone traversal is time-starved, but unconditional shared
retention perturbs work allocation without paying rent in the long-window
stratum. This violates the pre-registered requirement that the sealed holdout
be clearly positive across strata; the aggregate gain cannot erase that
systematic negative lane.

Preserved holdout artifacts:

- off log SHA256
  `4E0E7448143D0BAF8C1731204F1B63F14EF677CF88E0519DCFB1DED5127B9502`;
- on log SHA256
  `6D692282B28E96583BAA5B0457E52CD89385699E2F6C76AC022D9E025604E0EE`;
- empty stderr SHA256
  `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.

## Verdict

`rejected-unconditional-window-regression`.

The unconditional production flag is not promoted. The result reopens the
deadline axis through a narrower structural successor: retain the byte-identical
5000-ms incumbent, and on an authoritative public response window longer than
five seconds spend only a bounded extra continuation interval in the already
accepted protected target-terminal refiner. A <=5000-ms match must remain
byte-identical. The successor must submit once, use the public deadline rather
than map/seed/family routing, and preserve the incumbent on deadline, invalidity
or lack of a strict exact certificate.
