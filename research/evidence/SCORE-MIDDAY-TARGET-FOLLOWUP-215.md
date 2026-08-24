# SCORE-MIDDAY-TARGET-FOLLOWUP-215

## Frozen identity

- Parent: `8b4ec50`.
- Manifest: `research/holdouts/SCORE-MIDDAY-TARGET-FOLLOWUP-215.csv`.
- Manifest SHA256: `908C08A5E13AEE08C764F48DD444C98D1568FB5FF8145D471DE1262EF0D1EC14`.
- Frozen Linux A/B binary SHA256: `36E33ACC621F54714D6934C0F83CA706F994AB144A4AB6050C47CA8B0421966B`.

## Development

The 60-pair same-binary development gate produced `31/27/2`, `+507`
servings, with every difference at tier 3. The target-terminal suffix fired in
29 cases and those cases produced `28/0/1`. There were zero invalid plans,
emergencies, validator disagreements or lane failures.

## Sealed holdout

The independently frozen 108-pair holdout was opened once. It produced
`57/47/4` and a net `+721` servings. Gross gains were `+740`, gross losses were
`-19`, maximum gain was `+63` and maximum loss was `-9`; every difference was
tier 3. The suffix recorded 303 target-terminal acceptances across 52 cases.
Conditioned on at least one target acceptance, the result was `51/0/1`, with
`+712/-4` causal gain/loss and a single bounded `-4` tail. The other three
losses (`-1/-5/-9`) had zero target acceptances and cannot be caused by an
accepted suffix plan. All fuel profiles, role modes and difficulty strata were
net positive. Invalid, emergency and lane-failure totals were zero.

- Off log SHA256: `5ED3FDDAEB53B1BD6D49F3BDFC57320C362C87EF2E66B4C64399853607880F2F`.
- On log SHA256: `6510224F407F5F007209226C907DF6E1411750A4DCA626FCA145905CFB65F3B7`.
- Empty runner stderr SHA256: `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.

## BTC target-host gate

The promoted runtime binary SHA256 is
`500A51610FF0103C3CA19801A844732350FEF87FE97ABB33BF8A45A35DDAD2E4`.
Three fresh explicit-advanced matches used the canonical `5000 ms` compute cap:

- `m-4108`: hard, 32x32, 10 days, 100 steps, 8 agents, 30 spots, 6
  brands, default fuel. Ten valid acknowledgements, 9/9 reconciled transitions,
  zero skip/WAIT/emergency. The main/global prefix exhausted the protected
  window, so the target suffix correctly did not start. Result `6/60/496`,
  rank 3. Replay SHA256
  `7923F3FCD3FFB58CAB1F790536C50D8EABC2F2032DAF58B55F48CA97722B6572`.
- `m-4109`: hard, 24x24, 7 days, 100 steps, 4 agents, 18 spots, 6
  brands, low fuel. Seven valid acknowledgements, 6/6 reconciled transitions,
  zero skip/WAIT/emergency. The target suffix executed 161 routes and produced
  148 dual-valid plans with zero failure; no strict takeover existed on this
  random map. Result `6/42/152`, rank 1. Replay SHA256
  `FB39C8F40260D1D30D67F17454BD8B3F098C894ABBF587C6BD123BD68EE55EED`.
- `m-4110`: the same 24x24 class with default fuel. Seven valid
  acknowledgements, 6/6 reconciled transitions and zero skip/WAIT/emergency.
  The prefix consumed the protected window before a target task could start.
  Result `6/42/163`, rank 4. Replay SHA256
  `A561B21894DBF721B754A5EFCEC7760D564DDC300D27974B03321DA008836BE0`.

BTC therefore proves target-path wiring, exact validity, lifecycle safety and
deadline behavior. Ranking is not promotion evidence. The rank-3/rank-4 results
remain independent serving-score counterexamples for the next research sweep;
they are not regressions attributable to 215.

## Verdict

Accepted and enabled in production. The direct-parent prefix remains protected;
the suffix is additive, exact-certified and globally positive with bounded,
non-systematic downside. Reopen only on a reproducible tier-1/tier-2 regression,
invalid plan, simulator-validator disagreement, hard-cap failure, or systematic
target-conditioned downside on new evidence.
