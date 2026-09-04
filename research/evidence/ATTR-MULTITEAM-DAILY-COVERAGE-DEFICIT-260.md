# ATTR-MULTITEAM-DAILY-COVERAGE-DEFICIT-260

## Frozen evidence

- Consumed BTC replay: `artifacts/btc/m-5415-ab3d699-series.jsonl`
  - SHA256 `A0189CA5017F1B2252A754D4A38E3EE9DE96414674AA4C4B6EC7538CC2C2A939`
- Signed production BTC binary
  - SHA256 `4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`
- Unchanged sparse claim probe
  - SHA256 `0B7CA99D827DF216C3B44EACEBC35FA16B586F5E48660FCFA2BBD4376C240F7F`
- Persistent mask-32 session decision dump
  - SHA256 `2CA5B85AB4EC042B03D1CF2E9AA6418EED8BCDDC23C3E18D12253B33C746B302`

## Authoritative result

`m-5415` was hard difficulty with exactly three BTC bots, eight days, a 32x32
map, 96 steps/day, seven agents, twenty-four spots, eight brands, default fuel
and a public 10000-ms response window. Production finished third at `8/63/296`.
The two higher-ranked bots scored `8/64/271` and `8/64/251`. All eight actions
were accepted and exact-valid, independent validation agreed, all seven
transitions reconciled and stderr was empty.

Only day 4 missed coverage: `7/30`, missing brand 0. The live checkpoint was
`7/27`. Public continuation was authorized, had 3808 ms, generated and validated
158 plans, accepted one certified improvement and returned `7/30`; there was no
invalidity, failure or rollback.

## Earliest capability boundary

The live canonical decision itself selected `7/26`. Column generation reported
deadline reached, retained brand breadth `8,7,7,8,7,0,6`, and produced only two
simulator-valid complete master combinations before the master deadline. Thus
the missing brand was not removed by the public-continuation certificate: no
eight-brand plan reached that checkpoint.

Five independent `replay-solve --day 4 --response-ms 5000` runs each returned
`8/35`, with 7248--8787 combinations. This is a real availability witness but
not live-path equivalence because replay-solve constructs a fresh engine without
the live response ledger, cached post-ACK artifacts and prior own-footprint.
A persistent mask-32 session replay also reached `8/64`, but diverged from the
recorded earlier plans and ledger and therefore remains attribution only.

The unchanged recorded sparse closed-loop probe reconstructed an exact-valid
single-agent exchange at day 4 from cumulative `8/31/140` to `8/32/145`. Its
local suffix retained one extra daily unit through day 8. The protected search
found no strict same-terminal/fuel/road-footprint improvement: protected best
remained `8/31/140`. Therefore the witness changes future state and own traffic
and cannot satisfy the current nonterminal dominance certificate.

## Verdict

This is a fresh instance of the known state-coupled sparse/position residual.
The direct scheduling repairs are already falsified: experiment 179 restored
the staged legacy master around exact search without changing selected results;
240 and crossed 241 rejected the quality-order repair globally and in both
window classes. No new sound certificate or operation-equivalent speedup is
present, so no SCORE successor opens from this consumed replay. Production is
unchanged and the hard-three-bot series resumes at match 6.

Functionality preservation: no designed functionality was removed, disabled,
deferred or reduced; nothing was deleted.
