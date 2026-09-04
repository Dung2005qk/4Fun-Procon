# BTC 30-match operational screen — 2026-09-03 closure

## Scope and authority

This screen used canonical accepted production experiment 258 at commit
`ab3d6999d60a7ca290a366bf235154178c6d721f`, with the canonical main checkpoint
fixed at `5000 ms`. Every qualifying BTC match used hard difficulty, exactly
three bot opponents and a public response window of `10000 ms`.

BTC rank is an asymmetric operational/counterexample gate. A clean gameplay
loss can expose a gap, while a rank-1 result does not prove promotion-quality
improvement over the direct parent or protected lane champion.

## Aggregate verdict

- Qualifying matches: **30**.
- Strict official-score wins: **14**.
- Exact official-score ties: **1** (row 13; BTC displayed rank 2 only by
  cumulative response-time tie-break).
- Gameplay-score losses: **15**.
- First differing tier among losses: **3 tier-2 losses**, **12 tier-3 losses**.
- Invalid operational attempts excluded: `m-8614` and `m-9598`.

The result is concerning as counterexample evidence: canonical production has
material domain gaps. It is not evidence of a global protocol/runtime collapse,
because the screen also contains clean full-coverage wins over small, medium and
large configurations. The BTC bots remain a minimum failure gate, not a quality
metric or promotion opponent.

## Exact qualifying rows

| Row | Match | Production | Best bot | Classification |
|---:|---|---:|---:|---|
| 1 | m-5400 | 3/12/35 | 3/12/27 | strict win |
| 2 | m-5412 | 5/25/109 | 5/25/74 | strict win |
| 3 | m-5413 | 6/36/126 | 6/36/127 | tier-3 loss -1 |
| 4 | m-5414 | 6/42/167 | 6/39/115 | tier-2 win |
| 5 | m-5415 | 8/63/296 | 8/64/271 | tier-2 loss -1 |
| 6 | m-5422 | 8/78/247 | 8/80/327 | tier-2 loss -2 |
| 7 | m-5477 | 7/63/268 | 7/63/270 | tier-3 loss -2 |
| 8 | m-5478 | 5/20/52 | 5/20/31 | strict win |
| 9 | m-5486 | 6/42/159 | 6/42/151 | strict win |
| 10 | m-5500 | 8/80/241 | 8/76/184 | tier-2 win |
| 11 | m-5501 | 6/30/127 | 6/30/128 | tier-3 loss -1 |
| 12 | m-5502 | 8/64/195 | 8/64/197 | tier-3 loss -2 |
| 13 | m-5836 | 8/48/184 | 8/48/184 | exact official-score tie |
| 14 | m-5839 | 6/30/109 | 6/27/76 | tier-2 win |
| 15 | m-5845 | 6/42/138 | 6/42/132 | strict win |
| 16 | m-5846 | 8/32/161 | 8/32/190 | tier-3 loss -29 |
| 17 | m-5964 | 6/60/130 | 6/60/92 | strict win |
| 18 | m-5968 | 8/64/335 | 8/64/300 | strict win |
| 19 | m-6134 | 7/63/351 | 7/63/413 | tier-3 loss -62 |
| 20 | m-6213 | 8/80/647 | 8/80/867 | tier-3 loss -220 |
| 21 | m-8130 | 8/64/100 | 8/63/77 | tier-2 win |
| 22 | m-8135 | 3/15/182 | 3/15/320 | tier-3 loss -138 |
| 23 | m-8385 | 5/40/299 | 5/40/402 | tier-3 loss -103 |
| 24 | m-8615 | 5/30/174 | 5/30/182 | tier-3 loss -8 |
| 25 | m-9515 | 8/72/389 | 8/72/424 | tier-3 loss -35 |
| 26 | m-9592 | 10/100/460 | 10/100/615 | tier-3 loss -155 |
| 27 | m-9593 | 6/42/225 | 6/42/169 | strict win |
| 28 | m-9594 | 6/29/105 | 6/30/138 | tier-2 loss -1 |
| 29 | m-9920 | 8/32/128 | 8/32/123 | strict win |
| 30 | m-9921 | 10/100/396 | 10/98/334 | tier-2 win |

## Final qualifying row

Row 30 `m-9921` used ten days, 32x32, 128 steps/day, eight agents,
32 Spots, ten brands and low fuel 128. Production finished rank 1 at
`10/100/396`; the best bot reached `10/98/334`. All 10 actions were HTTP 200
and exact-valid, all nine transitions reconciled, stderr was empty, maximum
response was `6861 ms`, maximum main time was `3376 ms`, and there was no
emergency. Replay SHA256 is
`13E9077A0B35DA49F53F48C0CEF3C4514C4183E44739C7736EF4DD3300AFAD11`.

## Closure

The screen is complete. It does not authorize signing production as a final
general champion: known state-coupled prior-trajectory, high-fuel long-horizon,
candidate-supply and certificate boundaries remain material. No source change
or new production commit is justified by rank-1 rows alone.

Functionality-preservation answers: (1) this evidence report removes, disables,
defers or reduces no designed functionality; (2) nothing is deleted, so no
active-equivalent claim is required.
