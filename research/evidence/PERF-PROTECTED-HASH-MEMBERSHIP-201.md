# PERF-PROTECTED-HASH-MEMBERSHIP-201

Parent: `690728a`

Frozen manifest SHA256:
`6A073DE108F32E616C56E1A35F32B77E11F0378C45BCBD9F7ADD9B82D5FAD086`

## Mechanism

Only the two `uint64_t` membership containers inside protected WAIT detours and
terminal sparse coordinate ascent change from ordered to unordered storage.
Neither container is iterated or exposed. Candidate generation order, plan
hashing, exact simulation, independent validation, official comparison,
incumbent protection and the 5000-ms hard cap are unchanged.

## Direct equivalence gate

Parent binary SHA256:
`97D3397399302A09FF6C5601DCAD1744088900DEE2005BE1C06FC95A8BAB16C6`

Candidate binary SHA256:
`84112720C1D918339B05A17D8FA57B2A5A08AA44EEFA84E467092AD50AA22AF2`

Both binaries received the same deterministic authoritative state, ledger and
WAIT incumbent. The WAIT lane produced 11 generated and 11 valid candidates in
both binaries, with identical result plan and final-state hashes. The terminal
sparse lane produced 3,584 route visits, 1,547 generated/valid candidates, 16
strict improvements and seven coordinate rounds in both binaries. Both returned
`6/6/83`, result plan hash `4572210825862042076` and state hash
`5125308409693543102`, with no deadline or failure.

Equivalence log SHA256:
`3AAD2418BDCF2AFAAE7DE6D14DAA9CA7DEC5A419E0BF181CA8122CE0846E8900`

An earlier full-trajectory comparison was discarded because independent timed
main solves supplied different terminal incumbents. Its `643` versus `642`
outcome is not a container comparison and has no verdict authority.

The GCC unit binary stops at the same unrelated harvest-source-rank assertion
for both parent and candidate. It is recorded as a parent/toolchain baseline
failure, not counted as a candidate pass. The authoritative Windows unit gate
remains required before any promotion.

## Development decision

Fresh paired development completed all 60 pairs at `7/46/7`, serving gain/loss
`+68/-63`, tail `-22/+38`, with zero role mismatch, invalid, emergency or
refiner failure. Fixed-role was `4/22/4`, `+48/-44`, tail `-22/+38`; native was
`3/24/3`, `+20/-19`. Low fuel lost `37` while gaining `22`, and very-hard
finished `5/7/6`, `+61/-56`. The candidate completed 27 more valid wait plans,
512 more valid sparse plans and two more sparse rounds, but this did not protect
the incumbent trajectory: every gain category also contains losses.

Experiment 201 is rejected before holdout and BTC. The container replacement is
semantics-equivalent on a fixed complete input, but under the hard deadline it
only changes how much refinement work finishes and yields a balanced,
nonmonotonic score perturbation. There is no target-host claim to validate after
the development score gate fails.

Development log SHA256:
`23B16C01ED85A5BD4C9CD9F8B97E15CF390ACF814A8C6C72B8DCA78A9948282E`
