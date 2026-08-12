# PERF-MASTER-BUNDLE-092 BTC target gate

Frozen candidate executable SHA256:
`247C143CBB6FE09AF4479DD23A472F4A3DFF5161CF5C3EEE4B828C584871CB7D`.
No source or binary changed between the six matches.

| Match | Size | Fuel | Final score | Max solver ms | Initial master avg/max ms | DFS us sum | Early bundle rejects |
|---|---:|---:|---:|---:|---:|---:|---:|
| m-2077 | 8x8 | 1x | 6/60/410 | 2456 | 1550.7/1625 | 14403088 | 449384 |
| m-2078 | 32x32 | 1x | 6/60/258 | 2828 | 670.5/1124 | 4114028 | 15987 |
| m-2081 | 8x8 | 2x | 6/60/310 | 3065 | 1339.6/1467 | 11172763 | 70933 |
| m-2082 | 8x8 | 3x | 6/60/430 | 3376 | 1625.7/1642 | 15078975 | 41273 |
| m-2084 | 32x32 | 2x | 6/60/347 | 3279 | 961.2/1310 | 7385181 | 23857 |
| m-2085 | 32x32 | 3x | 6/60/450 | 3383 | 745.2/1127 | 5005316 | 9088 |

All six matches used 10 days, 5000 ms internal response budget, eight agents,
12 requested spots, six brands and three BTC bots. Easy used 64 steps; hard used
100. Every generated setup contained roads (8 on easy, 63 on hard), so the
accepted 088 roadless branch was inactive and did not confound the performance
mechanism.

Across 60 days there were 60 submitted actions, 60 HTTP 2xx acknowledgements,
zero emergency plans, zero hard-cap breach and zero replay validator mismatch.
Every replay reconciled all nine authoritative transitions. Maximum target-host
solver time was 3383 ms, leaving 1617 ms inside the 5000-ms hard cap. All six
final scores retained lifetime 6 and daily 60. Bot rank is ignored.

`bundlePrunes` has exactly one increment site. Under 092 it counts route columns
rejected before `BranchColumnRank` construction and `stable_sort`; the frozen
source proof establishes that the parent rejects the identical set only after
performing that work. The target-host total is 610522 avoided rank/sort entries.
This is direct operation-count evidence on BTC, independent of random-map timing.
Absolute timing remains a safety gate, not a causal A/B latency estimate.

Replay SHA256:

- m-2077: `EB06B3F9D43748126B4351677C842FB4430044383B5FFA146F270621AA8CFF75`
- m-2078: `6096EB09CE58889969DB86880695F18A0B8DB1BB270154BF90B53081E039D8B5`
- m-2081: `61FDAC18DC8D4F74465D011B9C25B74C3A9E9DDCA01F9E42FD454967E35EFEFD`
- m-2082: `4962FA4919A853C5FAF3ABD6EFED690E018FA53EBB4BBC8C4D3F11B4BF62809F`
- m-2084: `0B552E7E9EC92C82281C4FAB0F60F8B4808E2FEE42B11A2CB6517AD955475B07`
- m-2085: `3E9FFB06B7E88C77931B56B9DC3DEB42C5C2D40BCDBF701D5F37A4AD64BE01C4`

Verdict: accepted as a semantics-preserving mathematical performance improvement.
It removes no capability, search stage, route, bound or budget. Reopen only on a
bundle-compatibility contradiction, semantic regression, hard-cap failure or
source/binary drift.
