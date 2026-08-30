# ATTR-MULTITEAM-ROOT-CAUSAL-244

Date: 2026-08-29

## Scope and frozen inputs

- Attribution only; no production source behavior changed.
- Consumed BTC counterexample: `m-4959`, replay SHA256
  `336CB23D6444668C85EFE4663F209FA5DD8561BD1EB57FD01904B93D1A6AD532`.
- Research-only MSVC claim-probe SHA256
  `1D41A69ED38D5C3AEFDD400DE5B4D6AD429A20F97DEFD7A6BE174847ADCE85F2`.
- Authoritative match shape: hard, three BTC bots, four days, 12x12,
  96 steps/day, 10000-ms public response, eight agents, twelve spots,
  eight brands and low fuel.
- Live result: rank 4 at `8/32/211`, behind `234/224/220`; 4/4 days
  agreed between exact simulator and independent validator and 3/3
  transitions reconciled.

## Exclusions before the causal probe

- Role identity is not causal. The live seven-patrol/one-tanker mask 128
  scores `8/32/210` in the checkpoint counterfactual; all-patrol regresses to
  `8/30/118`.
- The canonical master reaches its deadline on days 1--3 and exposes a
  feasible-pool serving upper six above its selected score on every one of
  those days. This is diagnostic only, not a proof that the upper is a
  complete feasible plan.
- The unchanged sparse exchange finds an exact-valid day-1 root at
  `8/8/48` versus recorded `8/8/44`. Its protected same-terminal result is
  only `8/8/44`, so it is outside the existing transition certificate.
- The exact sparse full plan is absent from all 16 day-1 canonical audit
  candidates. Its changed patrol route is also absent from every audit
  candidate. The earliest witnessed mismatch is therefore route supply before
  audit/F0, not F0 selection.

## Closed-loop causal result

Both arms use the identical research-only lifecycle: exact application of the
forced day-1 root, `record_applied_transition`, then the unchanged canonical
engine for days 2--4 against the replay's recorded external states. The only
difference is the root plan/state.

| Arm | Day 1 | Day 2 | Day 3 | Day 4 | Final |
|---|---:|---:|---:|---:|---:|
| recorded root | 8/8/44 | 8/16/94 | 8/24/144 | 8/32/211 | 8/32/211 |
| sparse root | 8/8/48 | 8/16/98 | 8/24/148 | 8/32/215 | 8/32/215 |

Official lexicographic delta: `0/0/+4`. Daily suffix contributions are exactly
`50/50/67` in both arms, so the root gain persists instead of being erased or
reversed by the subsequent canonical decisions.

## Verdict

Accepted attribution: `m-4959` exposes a real additive pre-audit route-supply
gap. It is not a role-selection gap, F0 ordering gap, or suffix/cutoff artifact.
The forced sparse route still does not close the observed gap to the strongest
BTC bot, and this consumed replay has no promotion authority.

A separate SCORE successor is permitted only if it supplies the same class of
route additively before F0, preserves every parent candidate and certificate,
is derived from public state rather than fixture identity, and is frozen before
fresh development plus a sealed holdout. Protected coverage must include
synthetic 8/9/10-team traffic lanes. Broad nonterminal exact takeover and the
post-ACK/proof mechanisms rejected by 176/179/180 and 229--232 must not be
repeated.

Functionality-preservation answers: (1) this attribution removes, disables,
defers or reduces no designed production functionality; (2) nothing is deleted.
