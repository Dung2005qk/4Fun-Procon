# PROTECT-HORIZON-SCENARIO-084

Date: 2026-08-12

Manifest SHA256:
`A00548DF115C09CDED7530A7268153091354EFBDB38C57B89D9B130FE97BA1BE`.

External historical-harness binaries were frozen as parent
`7EF7143531B5280826709D7EE8018C5526B73E06277994A67F25FCD6A8415AF0`
and candidate
`D0DC94ECC238808DEBBB7A4EEB3436A45069A7B56DBCD645F176E20360A7A7E6`.
Local elapsed was never used as performance evidence.

Phase A, 36 new road-containing 8x8 fixtures per role lane:

- fixed all-patrol: candidate-vs-parent `4/30/2`, first-tier gain/loss sums
  `9/4`, tails `+3/-3`, zero invalid/emergency;
- native exhaustive: `3/31/2`, sums `13/2`, tails `+11/-1`, zero
  invalid/emergency, 8/36 role-mask changes;
- combined: `7/61/4`. No family showed systematic loss. Rare-brand seed
  `830017` lost in both lanes and remains an explicit tail.

Phase B low-fuel fixed, 12 BTC-like 32x32 horizon-10 fixtures:

- `4/8/0`, first-tier gain sum `18`, no loss, maximum gain `+8`;
- gains spanned rare-brand, overnight, threshold-corridor and high-stock;
- zero invalid/emergency.

Low-fuel native produced cross-binary W/T/L `3/1/8`, but it is not a causal
logic verdict. All 12 parent/candidate role masks were identical and every mask
contained a tanker. On such configs SCORE-HORIZON-072 returns the old bound;
because every map contains roads, SCORE-SCENARIO-073 returns the old manifest.
Thus both candidate mechanisms are semantically inactive while wall-clock search
cutoffs remain compile-layout dependent. The large tier-3 spread is evidence
that local cannot judge target-host performance, not evidence that the candidate
logic regressed. Both binaries still had zero invalid/emergency.

Verdict: logic protection remains positive; local performance is inconclusive.
Remaining default/high native rows were not opened because they cannot resolve
the same non-causal cross-binary cutoff. The unchanged candidate requires the
unit/semantic gate and BTC target-host 5000-ms telemetry before commit.

The direct semantic gate then passed. One theorem test proves that a roadless
configuration freezes exactly one `deterministic-no-road` scenario with weight
10000, no false pessimistic fallback and no survival signature. A second theorem
test isolates one fuel-zero patrol at a spot for four remaining days and proves
the no-tanker upper is exactly four additional daily-distinct/serving claims;
changing an isolated agent to tanker restores the established coarse relaxation.
The complete unit suite passed. This test-only build did not rebuild or change
the frozen BTC artifact, whose SHA256 remains
`784E0E8F5156E063AD572F9946BBDA7EB158C00D62D267EEB35812A434993032`.

The only remaining promotion gate is BTC target-host validity, lifecycle and
5000-ms hard-cap telemetry using that exact frozen artifact.

## BTC target-host gate

Fresh explicit-advanced match `m-1986` used hard difficulty, three bots, ten
days, a 32x32 map, 100 steps/day, 5000 ms response, eight agents, twelve spots,
six brands and low fuel 1x. The exact frozen artifact selected one-tanker mask
8. Setup receipt to accepted assignment record was 4623 ms, below the internal
cap.

All ten decisions and all ten action POSTs completed with HTTP 200 valid
acknowledgements. Decision `totalMs` ranged from 1882 to 3376 ms; end-to-end
server `response_ms` ranged from 2085 to 3458 ms. There were zero emergencies,
submission skips, server WAIT replacements, invalid actions, hard-cap breaches
or authoritative-state reconciliation mismatches. Independent `replay-check`
reported `valid=1` and `validator_agrees=1` on every day and reconciled all nine
inter-day transitions. Final exact score was `6/60/219`. Rank 1 over the three
bots is recorded only as an asymmetric failure check, not promotion evidence.

Replay SHA256:
`7B0B83411B21550F20C4691B0EC9077FCB6E553212722BA70A3E1F1EEF706F53`.
The executable SHA256 remained
`784E0E8F5156E063AD572F9946BBDA7EB158C00D62D267EEB35812A434993032`.

Verdict: accepted. The unchanged combined 072+073 candidate has passed the
paired global score gate, bounded-tail protected matrix, direct semantic gate
and BTC target-host hard-cap/lifecycle gate. It is eligible for a canonical
production commit; no further tuning on the consumed 083/084 fixtures is
permitted.
